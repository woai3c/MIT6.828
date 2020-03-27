#ifndef JOS_MACHINE_SETJMP_H
#define JOS_MACHINE_SETJMP_H

#include <inc/types.h>

#define JOS_LONGJMP_GCCATTR	regparm(2)

struct jos_jmp_buf {
    uint32_t jb_eip;
    uint32_t jb_esp;
    uint32_t jb_ebp;
    uint32_t jb_ebx;
    uint32_t jb_esi;
    uint32_t jb_edi;
};

#endif
