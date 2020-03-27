#include <inc/lib.h>

#include <lwip/sys.h>
#include <arch/thread.h>
#include <arch/cc.h>
#include <arch/sys_arch.h>
#include <arch/perror.h>
#include <arch/queue.h>

#define debug 0

#define NSEM		256
#define NMBOX		128
#define MBOXSLOTS	32

struct sys_sem_entry {
    int freed;
    int gen;
    union {
	uint32_t v;
	struct {
	    uint16_t counter;
	    uint16_t waiters;
	};
    };
    LIST_ENTRY(sys_sem_entry) link;
};
static struct sys_sem_entry sems[NSEM];
static LIST_HEAD(sem_list, sys_sem_entry) sem_free;

struct sys_mbox_entry {
    int freed;
    int head, nextq;
    void *msg[MBOXSLOTS];
    sys_sem_t queued_msg;
    sys_sem_t free_msg;
    LIST_ENTRY(sys_mbox_entry) link;
};
static struct sys_mbox_entry mboxes[NMBOX];
static LIST_HEAD(mbox_list, sys_mbox_entry) mbox_free;

struct sys_thread {
    thread_id_t tid;
    struct sys_timeouts tmo;
    LIST_ENTRY(sys_thread) link;
};

enum { thread_hash_size = 257 };
static LIST_HEAD(thread_list, sys_thread) threads[thread_hash_size];

void
sys_init(void)
{
    int i = 0;
    for (i = 0; i < NSEM; i++) {
	sems[i].freed = 1;
	LIST_INSERT_HEAD(&sem_free, &sems[i], link);
    }

    for (i = 0; i < NMBOX; i++) {
	mboxes[i].freed = 1;
	LIST_INSERT_HEAD(&mbox_free, &mboxes[i], link);
    }
}

sys_mbox_t
sys_mbox_new(int size)
{
    assert(size < MBOXSLOTS);
    struct sys_mbox_entry *mbe = LIST_FIRST(&mbox_free);
    if (!mbe) {
	cprintf("lwip: sys_mbox_new: out of mailboxes\n");
	return SYS_MBOX_NULL;
    }
    LIST_REMOVE(mbe, link);
    assert(mbe->freed);
    mbe->freed = 0;

    int i = mbe - &mboxes[0];
    mbe->head = -1;
    mbe->nextq = 0;
    mbe->queued_msg = sys_sem_new(0);
    mbe->free_msg = sys_sem_new(MBOXSLOTS);

    if (mbe->queued_msg == SYS_SEM_NULL ||
	mbe->free_msg == SYS_SEM_NULL)
    {
	sys_mbox_free(i);
	cprintf("lwip: sys_mbox_new: can't get semaphore\n");
	return SYS_MBOX_NULL;
    }
    return i;
}

void
sys_mbox_free(sys_mbox_t mbox)
{
    assert(!mboxes[mbox].freed);
    sys_sem_free(mboxes[mbox].queued_msg);
    sys_sem_free(mboxes[mbox].free_msg);
    LIST_INSERT_HEAD(&mbox_free, &mboxes[mbox], link);
    mboxes[mbox].freed = 1;
}

void
sys_mbox_post(sys_mbox_t mbox, void *msg)
{
    assert(sys_mbox_trypost(mbox, msg) == ERR_OK);
}

err_t 
sys_mbox_trypost(sys_mbox_t mbox, void *msg)
{
    assert(!mboxes[mbox].freed);

    sys_arch_sem_wait(mboxes[mbox].free_msg, 0);
    if (mboxes[mbox].nextq == mboxes[mbox].head)
	return ERR_MEM;

    int slot = mboxes[mbox].nextq;
    mboxes[mbox].nextq = (slot + 1) % MBOXSLOTS;
    mboxes[mbox].msg[slot] = msg;

    if (mboxes[mbox].head == -1)
	mboxes[mbox].head = slot;

    sys_sem_signal(mboxes[mbox].queued_msg);

    return ERR_OK;
}

sys_sem_t
sys_sem_new(u8_t count)
{
    struct sys_sem_entry *se = LIST_FIRST(&sem_free);
    if (!se) {
	cprintf("lwip: sys_sem_new: out of semaphores\n");
	return SYS_SEM_NULL;
    }
    LIST_REMOVE(se, link);
    assert(se->freed);
    se->freed = 0;

    se->counter = count;
    se->gen++;
    return se - &sems[0];
}

void
sys_sem_free(sys_sem_t sem)
{
    assert(!sems[sem].freed);
    sems[sem].freed = 1;
    sems[sem].gen++;
    LIST_INSERT_HEAD(&sem_free, &sems[sem], link);
}

void
sys_sem_signal(sys_sem_t sem)
{
    assert(!sems[sem].freed);
    sems[sem].counter++;
    if (sems[sem].waiters) {
	sems[sem].waiters = 0;
	thread_wakeup(&sems[sem].v);
    }
}

u32_t
sys_arch_sem_wait(sys_sem_t sem, u32_t tm_msec)
{
    assert(!sems[sem].freed);
    u32_t waited = 0;

    int gen = sems[sem].gen;

    while (tm_msec == 0 || waited < tm_msec) {
	if (sems[sem].counter > 0) {
	    sems[sem].counter--;
	    return waited;
 	} else if (tm_msec == SYS_ARCH_NOWAIT) {
	    return SYS_ARCH_TIMEOUT;
	} else {
	    uint32_t a = sys_time_msec();
	    uint32_t sleep_until = tm_msec ? a + (tm_msec - waited) : ~0;
	    sems[sem].waiters = 1;
	    uint32_t cur_v = sems[sem].v;
	    lwip_core_unlock();
	    thread_wait(&sems[sem].v, cur_v, sleep_until);
	    lwip_core_lock();
	    if (gen != sems[sem].gen) {
		cprintf("sys_arch_sem_wait: sem freed under waiter!\n");
		return SYS_ARCH_TIMEOUT;
	    }
	    uint32_t b = sys_time_msec();
	    waited += (b - a);
	}
    }

    return SYS_ARCH_TIMEOUT;
}

u32_t
sys_arch_mbox_fetch(sys_mbox_t mbox, void **msg, u32_t tm_msec)
{
    assert(!mboxes[mbox].freed);

    u32_t waited = sys_arch_sem_wait(mboxes[mbox].queued_msg, tm_msec);
    if (waited == SYS_ARCH_TIMEOUT)
	return waited;

    int slot = mboxes[mbox].head;
    if (slot == -1)
	panic("lwip: sys_arch_mbox_fetch: no message");
    if (msg)
	*msg = mboxes[mbox].msg[slot];

    mboxes[mbox].head = (slot + 1) % MBOXSLOTS;
    if (mboxes[mbox].head == mboxes[mbox].nextq)
	mboxes[mbox].head = -1;

    sys_sem_signal(mboxes[mbox].free_msg);
    return waited;
}

u32_t 
sys_arch_mbox_tryfetch(sys_mbox_t mbox, void **msg)
{
    return sys_arch_mbox_fetch(mbox, msg, SYS_ARCH_NOWAIT);
}

struct lwip_thread {
    void (*func)(void *arg);
    void *arg;
};

static void
lwip_thread_entry(uint32_t arg)
{
    struct lwip_thread *lt = (struct lwip_thread *)arg;
    lwip_core_lock();
    lt->func(lt->arg);
    lwip_core_unlock();
    free(lt);
}

sys_thread_t
sys_thread_new(char *name, void (* thread)(void *arg), void *arg, 
	       int stacksize, int prio)
{
    struct lwip_thread *lt = malloc(sizeof(*lt));
    if (lt == 0)
	panic("sys_thread_new: cannot allocate thread struct");

    if (stacksize > PGSIZE)
	panic("large stack %d", stacksize);

    lt->func = thread;
    lt->arg = arg;

    thread_id_t tid;
    int r = thread_create(&tid, name, lwip_thread_entry, (uint32_t)lt);

    if (r < 0)
	panic("lwip: sys_thread_new: cannot create: %s\n", e2s(r));

    return tid;
}

static void
timeout_cleanup(thread_id_t tid)
{
    lwip_core_lock();

    struct sys_thread *t;
    LIST_FOREACH(t, &threads[tid % thread_hash_size], link)
	if (t->tid == tid) {
	    LIST_REMOVE(t, link);
	    free(t);
	    goto done;
	}

    if (debug) cprintf("timeout_cleanup: bogus tid %ld\n", tid);
 done:
    lwip_core_unlock();
}

struct sys_timeouts *
sys_arch_timeouts(void)
{
    thread_id_t tid = thread_id();

    struct sys_thread *t;
    LIST_FOREACH(t, &threads[tid % thread_hash_size], link)
	if (t->tid == tid)
	    goto out;

    t = malloc(sizeof(*t));
    if (t == 0)
	panic("sys_arch_timeouts: cannot malloc");

    int r = thread_onhalt(timeout_cleanup);
    if (r < 0)
	panic("thread_onhalt failed: %s", e2s(r));

    t->tid = tid;
    memset(&t->tmo, 0, sizeof(t->tmo));
    LIST_INSERT_HEAD(&threads[tid % thread_hash_size], t, link);

out:
    return &t->tmo;
}

void
lwip_core_lock(void)
{
}

void
lwip_core_unlock(void)
{
}
