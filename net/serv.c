/*
 * Network server main loop -
 * serves IPC requests from other environments.
 */

#include <inc/x86.h>
#include <inc/string.h>
#include <inc/env.h>
#include <inc/ns.h>
#include <inc/lib.h>

#include <arch/perror.h>
#include <arch/thread.h>
#include <lwip/sockets.h>
#include <lwip/netif.h>
#include <lwip/stats.h>
#include <lwip/sys.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/dhcp.h>
#include <lwip/tcpip.h>
#include <lwip/stats.h>
#include <lwip/netbuf.h>
#include <netif/etharp.h>
#include <jif/jif.h>

#include "ns.h"

/* errno to make lwIP happy */
int errno;

struct netif nif;

#define debug 0

struct timer_thread {
	uint32_t msec;
	void (*func)(void);
	const char *name;
};

static struct timer_thread t_arp;
static struct timer_thread t_tcpf;
static struct timer_thread t_tcps;

static envid_t timer_envid;
static envid_t input_envid;
static envid_t output_envid;

static bool buse[QUEUE_SIZE];
static int next_i(int i) { return (i+1) % QUEUE_SIZE; }
static int prev_i(int i) { return (i ? i-1 : QUEUE_SIZE-1); }

static void *
get_buffer(void) {
	void *va;

	int i;
	for (i = 0; i < QUEUE_SIZE; i++)
		if (!buse[i]) break;

	if (i == QUEUE_SIZE) {
		panic("NS: buffer overflow");
		return 0;
	}

	va = (void *)(REQVA + i * PGSIZE);
	buse[i] = 1;

	return va;
}

static void
put_buffer(void *va) {
	int i = ((uint32_t)va - REQVA) / PGSIZE;
	buse[i] = 0;
}

static void
lwip_init(struct netif *nif, void *if_state,
	  uint32_t init_addr, uint32_t init_mask, uint32_t init_gw)
{
	struct ip_addr ipaddr, netmask, gateway;
	ipaddr.addr  = init_addr;
	netmask.addr = init_mask;
	gateway.addr = init_gw;

	if (0 == netif_add(nif, &ipaddr, &netmask, &gateway,
			   if_state,
			   jif_init,
			   ip_input))
		panic("lwip_init: error in netif_add\n");

	netif_set_default(nif);
	netif_set_up(nif);
}

static void __attribute__((noreturn))
net_timer(uint32_t arg)
{
	struct timer_thread *t = (struct timer_thread *) arg;

	for (;;) {
		uint32_t cur = sys_time_msec();

		lwip_core_lock();
		t->func();
		lwip_core_unlock();

		thread_wait(0, 0, cur + t->msec);
	}
}

static void
start_timer(struct timer_thread *t, void (*func)(void), const char *name, int msec)
{
	t->msec = msec;
	t->func = func;
	t->name = name;
	int r = thread_create(0, name, &net_timer, (uint32_t)t);
	if (r < 0)
		panic("cannot create timer thread: %s", e2s(r));
}

static void
tcpip_init_done(void *arg)
{
	uint32_t *done = arg;
	*done = 1;
	thread_wakeup(done);
}

void
serve_init(uint32_t ipaddr, uint32_t netmask, uint32_t gw)
{
	int r;
	lwip_core_lock();

	uint32_t done = 0;
	tcpip_init(&tcpip_init_done, &done);
	lwip_core_unlock();
	thread_wait(&done, 0, (uint32_t)~0);
	lwip_core_lock();

	lwip_init(&nif, &output_envid, ipaddr, netmask, gw);

	start_timer(&t_arp, &etharp_tmr, "arp timer", ARP_TMR_INTERVAL);
	start_timer(&t_tcpf, &tcp_fasttmr, "tcp f timer", TCP_FAST_INTERVAL);
	start_timer(&t_tcps, &tcp_slowtmr, "tcp s timer", TCP_SLOW_INTERVAL);

	struct in_addr ia = {ipaddr};
	cprintf("ns: %02x:%02x:%02x:%02x:%02x:%02x"
		" bound to static IP %s\n",
		nif.hwaddr[0], nif.hwaddr[1], nif.hwaddr[2],
		nif.hwaddr[3], nif.hwaddr[4], nif.hwaddr[5],
		inet_ntoa(ia));

	lwip_core_unlock();

	cprintf("NS: TCP/IP initialized.\n");
}

static void
process_timer(envid_t envid) {
	uint32_t start, now, to;

	if (envid != timer_envid) {
		cprintf("NS: received timer interrupt from envid %x not timer env\n", envid);
		return;
	}

	start = sys_time_msec();
	thread_yield();
	now = sys_time_msec();

	to = TIMER_INTERVAL - (now - start);
	ipc_send(envid, to, 0, 0);
}

struct st_args {
	int32_t reqno;
	uint32_t whom;
	union Nsipc *req;
};

static void
serve_thread(uint32_t a) {
	struct st_args *args = (struct st_args *)a;
	union Nsipc *req = args->req;
	int r;

	switch (args->reqno) {
	case NSREQ_ACCEPT:
	{
		struct Nsret_accept ret;
		ret.ret_addrlen = req->accept.req_addrlen;
		r = lwip_accept(req->accept.req_s, &ret.ret_addr,
				&ret.ret_addrlen);
		memmove(req, &ret, sizeof ret);
		break;
	}
	case NSREQ_BIND:
		r = lwip_bind(req->bind.req_s, &req->bind.req_name,
			      req->bind.req_namelen);
		break;
	case NSREQ_SHUTDOWN:
		r = lwip_shutdown(req->shutdown.req_s, req->shutdown.req_how);
		break;
	case NSREQ_CLOSE:
		r = lwip_close(req->close.req_s);
		break;
	case NSREQ_CONNECT:
		r = lwip_connect(req->connect.req_s, &req->connect.req_name,
				 req->connect.req_namelen);
		break;
	case NSREQ_LISTEN:
		r = lwip_listen(req->listen.req_s, req->listen.req_backlog);
		break;
	case NSREQ_RECV:
		// Note that we read the request fields before we
		// overwrite it with the response data.
		r = lwip_recv(req->recv.req_s, req->recvRet.ret_buf,
			      req->recv.req_len, req->recv.req_flags);
		break;
	case NSREQ_SEND:
		r = lwip_send(req->send.req_s, &req->send.req_buf,
			      req->send.req_size, req->send.req_flags);
		break;
	case NSREQ_SOCKET:
		r = lwip_socket(req->socket.req_domain, req->socket.req_type,
				req->socket.req_protocol);
		break;
	case NSREQ_INPUT:
		jif_input(&nif, (void *)&req->pkt);
		r = 0;
		break;
	default:
		cprintf("Invalid request code %d from %08x\n", args->whom, args->req);
		r = -E_INVAL;
		break;
	}

	if (r == -1) {
		char buf[100];
		snprintf(buf, sizeof buf, "ns req type %d", args->reqno);
		perror(buf);
	}

	if (args->reqno != NSREQ_INPUT)
		ipc_send(args->whom, r, 0, 0);

	put_buffer(args->req);
	sys_page_unmap(0, (void*) args->req);
	free(args);
}

void
serve(void) {
	int32_t reqno;
	uint32_t whom;
	int i, perm;
	void *va;

	while (1) {
		// ipc_recv will block the entire process, so we flush
		// all pending work from other threads.  We limit the
		// number of yields in case there's a rogue thread.
		for (i = 0; thread_wakeups_pending() && i < 32; ++i)
			thread_yield();

		perm = 0;
		va = get_buffer();
		reqno = ipc_recv((int32_t *) &whom, (void *) va, &perm);
		if (debug) {
			cprintf("ns req %d from %08x\n", reqno, whom);
		}

		// first take care of requests that do not contain an argument page
		if (reqno == NSREQ_TIMER) {
			process_timer(whom);
			put_buffer(va);
			continue;
		}

		// All remaining requests must contain an argument page
		if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument page\n", whom);
			continue; // just leave it hanging...
		}

		// Since some lwIP socket calls will block, create a thread and
		// process the rest of the request in the thread.
		struct st_args *args = malloc(sizeof(struct st_args));
		if (!args)
			panic("could not allocate thread args structure");

		args->reqno = reqno;
		args->whom = whom;
		args->req = va;

		thread_create(0, "serve_thread", serve_thread, (uint32_t)args);
		thread_yield(); // let the thread created run
	}
}

static void
tmain(uint32_t arg) {
	serve_init(inet_addr(IP),
		   inet_addr(MASK),
		   inet_addr(DEFAULT));
	serve();
}

void
umain(int argc, char **argv)
{
	envid_t ns_envid = sys_getenvid();

	binaryname = "ns";

	// fork off the timer thread which will send us periodic messages
	timer_envid = fork();
	if (timer_envid < 0)
		panic("error forking");
	else if (timer_envid == 0) {
		timer(ns_envid, TIMER_INTERVAL);
		return;
	}

	// fork off the input thread which will poll the NIC driver for input
	// packets
	input_envid = fork();
	if (input_envid < 0)
		panic("error forking");
	else if (input_envid == 0) {
		input(ns_envid);
		return;
	}

	// fork off the output thread that will send the packets to the NIC
	// driver
	output_envid = fork();
	if (output_envid < 0)
		panic("error forking");
	else if (output_envid == 0) {
		output(ns_envid);
		return;
	}

	// lwIP requires a user threading library; start the library and jump
	// into a thread to continue initialization.
	thread_init();
	thread_create(0, "main", tmain, 0);
	thread_yield();
	// never coming here!
}
