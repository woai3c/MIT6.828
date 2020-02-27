
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int p[2], r, i;
	struct Fd *fd;
	const volatile struct Env *kid;

	cprintf("testing for pipeisclosed race...\n");
	if ((r = pipe(p)) < 0)
		panic("pipe: %e", r);
	if ((r = fork()) < 0)
		panic("fork: %e", r);
	if (r == 0) {
		// child just dups and closes repeatedly,
		// yielding so the parent can see
		// the fd state between the two.
		close(p[1]);
		for (i = 0; i < 200; i++) {
			if (i % 10 == 0)
				cprintf("%d.", i);
			// dup, then close.  yield so that other guy will
			// see us while we're between them.
			dup(p[0], 10);
			sys_yield();
			close(10);
			sys_yield();
		}
		exit();
	}

	// We hold both p[0] and p[1] open, so pipeisclosed should
	// never return false.
	//
	// Now the ref count for p[0] will toggle between 2 and 3
	// as the child dups and closes it.
	// The ref count for p[1] is 1.
	// Thus the ref count for the underlying pipe structure
	// will toggle between 3 and 4.
	//
	// If pipeisclosed checks pageref(p[0]) and gets 3, and
	// then the child closes, and then pipeisclosed checks
	// pageref(pipe structure) and gets 3, then it will return true
	// when it shouldn't.
	//
	// If pipeisclosed checks pageref(pipe structure) and gets 3,
	// and then the child dups, and then pipeisclosed checks
	// pageref(p[0]) and gets 3, then it will return true when
	// it shouldn't.
	//
	// So either way, pipeisclosed is going give a wrong answer.
	//
	kid = &envs[ENVX(r)];
	while (kid->env_status == ENV_RUNNABLE)
		if (pipeisclosed(p[0]) != 0) {
			cprintf("\nRACE: pipe appears closed\n");
			sys_env_destroy(r);
			exit();
		}
	cprintf("child done with loop\n");
	if (pipeisclosed(p[0]))
		panic("somehow the other end of p[0] got closed!");
	if ((r = fd_lookup(p[0], &fd)) < 0)
		panic("cannot look up p[0]: %e", r);
	(void) fd2data(fd);
	cprintf("race didn't happen\n");
}
