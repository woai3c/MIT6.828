/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_ENV_H
#define JOS_KERN_ENV_H

#include <inc/env.h>

extern struct Env *envs;		// All environments
extern struct Env *curenv;		// Current environment
extern struct Segdesc gdt[];

void	env_init(void);
void	env_init_percpu(void);
int	env_alloc(struct Env **e, envid_t parent_id);
void	env_free(struct Env *e);
void	env_create(uint8_t *binary, enum EnvType type);
void	env_destroy(struct Env *e);	// Does not return if e == curenv

int	envid2env(envid_t envid, struct Env **env_store, bool checkperm);
// The following two functions do not return
void	env_run(struct Env *e) __attribute__((noreturn));
void	env_pop_tf(struct Trapframe *tf) __attribute__((noreturn));

// Without this extra macro, we couldn't pass macros like TEST to
// ENV_CREATE because of the C pre-processor's argument prescan rule.
#define ENV_PASTE3(x, y, z) x ## y ## z

#define ENV_CREATE(x, type)						\
	do {								\
		extern uint8_t ENV_PASTE3(_binary_obj_, x, _start)[];	\
		env_create(ENV_PASTE3(_binary_obj_, x, _start),		\
			   type);					\
	} while (0)

#endif // !JOS_KERN_ENV_H
