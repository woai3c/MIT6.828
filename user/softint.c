// buggy program - causes an illegal software interrupt

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	asm volatile("int $14");	// page fault
}

