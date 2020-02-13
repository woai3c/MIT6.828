// Demonstrate lack of fairness in IPC.
// Start three instances of this program as envs 1, 2, and 3.
// (user/idle is env 0).

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t who, id;

	id = sys_getenvid();

	if (thisenv == &envs[1]) {
		while (1) {
			ipc_recv(&who, 0, 0);
			cprintf("%x recv from %x\n", id, who);
		}
	} else {
		cprintf("%x loop sending to %x\n", id, envs[1].env_id);
		while (1)
			ipc_send(envs[1].env_id, 0, 0, 0);
	}
}

