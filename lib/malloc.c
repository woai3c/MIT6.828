
#include <inc/lib.h>

/*
 * Simple malloc/free.
 *
 * Uses the address space to do most of the hard work.
 * The address space from mbegin to mend is scanned
 * in order.  Pages are allocated, used to fill successive
 * malloc requests, and then left alone.  Free decrements
 * a ref count maintained in the page; the page is freed
 * when the ref count hits zero.
 *
 * If we need to allocate a large amount (more than a page)
 * we can't put a ref count at the end of each page,
 * so we mark the pte entry with the bit PTE_CONTINUED.
 */
enum
{
	MAXMALLOC = 1024*1024	/* max size of one allocated chunk */
};

#define PTE_CONTINUED 0x200

static uint8_t *mbegin = (uint8_t*) 0x08000000;
static uint8_t *mend   = (uint8_t*) 0x10000000;
static uint8_t *mptr;

static int
isfree(void *v, size_t n)
{
	uintptr_t va, end_va = (uintptr_t) v + n;

	for (va = (uintptr_t) v; va < end_va; va += PGSIZE)
		if (va >= (uintptr_t) mend
		    || ((uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_P)))
			return 0;
	return 1;
}

void*
malloc(size_t n)
{
	int i, cont;
	int nwrap;
	uint32_t *ref;
	void *v;

	if (mptr == 0)
		mptr = mbegin;

	n = ROUNDUP(n, 4);

	if (n >= MAXMALLOC)
		return 0;

	if ((uintptr_t) mptr % PGSIZE){
		/*
		 * we're in the middle of a partially
		 * allocated page - can we add this chunk?
		 * the +4 below is for the ref count.
		 */
		ref = (uint32_t*) (ROUNDUP(mptr, PGSIZE) - 4);
		if ((uintptr_t) mptr / PGSIZE == (uintptr_t) (mptr + n - 1 + 4) / PGSIZE) {
			(*ref)++;
			v = mptr;
			mptr += n;
			return v;
		}
		/*
		 * stop working on this page and move on.
		 */
		free(mptr);	/* drop reference to this page */
		mptr = ROUNDDOWN(mptr + PGSIZE, PGSIZE);
	}

	/*
	 * now we need to find some address space for this chunk.
	 * if it's less than a page we leave it open for allocation.
	 * runs of more than a page can't have ref counts so we
	 * flag the PTE entries instead.
	 */
	nwrap = 0;
	while (1) {
		if (isfree(mptr, n + 4))
			break;
		mptr += PGSIZE;
		if (mptr == mend) {
			mptr = mbegin;
			if (++nwrap == 2)
				return 0;	/* out of address space */
		}
	}

	/*
	 * allocate at mptr - the +4 makes sure we allocate a ref count.
	 */
	for (i = 0; i < n + 4; i += PGSIZE){
		cont = (i + PGSIZE < n + 4) ? PTE_CONTINUED : 0;
		if (sys_page_alloc(0, mptr + i, PTE_P|PTE_U|PTE_W|cont) < 0){
			for (; i >= 0; i -= PGSIZE)
				sys_page_unmap(0, mptr + i);
			return 0;	/* out of physical memory */
		}
	}

	ref = (uint32_t*) (mptr + i - 4);
	*ref = 2;	/* reference for mptr, reference for returned block */
	v = mptr;
	mptr += n;
	return v;
}

void
free(void *v)
{
	uint8_t *c;
	uint32_t *ref;

	if (v == 0)
		return;
	assert(mbegin <= (uint8_t*) v && (uint8_t*) v < mend);

	c = ROUNDDOWN(v, PGSIZE);

	while (uvpt[PGNUM(c)] & PTE_CONTINUED) {
		sys_page_unmap(0, c);
		c += PGSIZE;
		assert(mbegin <= c && c < mend);
	}

	/*
	 * c is just a piece of this page, so dec the ref count
	 * and maybe free the page.
	 */
	ref = (uint32_t*) (c + PGSIZE - 4);
	if (--(*ref) == 0)
		sys_page_unmap(0, c);
}

