// Ping-pong a counter between two shared-memory processes.
// Only need to start one of these -- splits into two with sfork.

#include <inc/lib.h>

uint32_t val;

void
umain(int argc, char **argv)
{
	envid_t who;
	uint32_t i;

	i = 0;
	if ((who = sfork()) != 0) {
		cprintf("i am %08x; thisenv is %p\n", sys_getenvid(), thisenv);
		// get the ball rolling
		cprintf("send 0 from %x to %x\n", sys_getenvid(), who);
		ipc_send(who, 0, 0, 0);
	}

	while (1) {
		ipc_recv(&who, 0, 0);
		cprintf("%x got %d from %x (thisenv is %p %x)\n", sys_getenvid(), val, who, thisenv, thisenv->env_id);
		if (val == 10)
			return;
		++val;
		ipc_send(who, 0, 0, 0);
		if (val == 10)
			return;
	}

}
