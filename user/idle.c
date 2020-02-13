// idle loop

#include <inc/x86.h>
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	binaryname = "idle";

	// Loop forever, simply trying to yield to a different environment.
	// Instead of busy-waiting like this,
	// a better way would be to use the processor's HLT instruction
	// to cause the processor to stop executing until the next interrupt -
	// doing so allows the processor to conserve power more effectively.
	while (1) {
		sys_yield();
	}
}

