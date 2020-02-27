#include <inc/lib.h>

// Waits until 'envid' exits.
void
wait(envid_t envid)
{
	const volatile struct Env *e;

	assert(envid != 0);
	e = &envs[ENVX(envid)];
	while (e->env_id == envid && e->env_status != ENV_FREE)
		sys_yield();
}
