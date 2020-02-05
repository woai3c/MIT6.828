/* See COPYRIGHT for copyright information. */

/* Support for reading the NVRAM from the real-time clock. */

#include <inc/x86.h>

#include <kern/kclock.h>


unsigned
mc146818_read(unsigned reg)
{
	outb(IO_RTC, reg);
	return inb(IO_RTC+1);
}

void
mc146818_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC, reg);
	outb(IO_RTC+1, datum);
}
