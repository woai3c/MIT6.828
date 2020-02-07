// buggy program - faults with a write to location zero

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	*(unsigned*)0 = 0;
}

