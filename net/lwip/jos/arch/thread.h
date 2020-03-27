#ifndef LWIP_ARCH_THREAD_H
#define LWIP_ARCH_THREAD_H

#include <inc/types.h>

typedef uint32_t thread_id_t;

void thread_init(void);
thread_id_t thread_id(void);
void thread_wakeup(volatile uint32_t *addr);
void thread_wait(volatile uint32_t *addr, uint32_t val, uint32_t msec);
int thread_wakeups_pending(void);
int thread_onhalt(void (*fun)(thread_id_t));
int thread_create(thread_id_t *tid, const char *name, 
		void (*entry)(uint32_t), uint32_t arg);
void thread_yield(void);
void thread_halt(void);

#endif
