#include <inc/lib.h>

#define VA ((char *) 0xA0000000)
const char *msg = "hello, world\n";
const char *msg2 = "goodbye, world\n";

void childofspawn(void);

void
umain(int argc, char **argv)
{
	int r;

	if (argc != 0)
		childofspawn();

	if ((r = sys_page_alloc(0, VA, PTE_P|PTE_W|PTE_U|PTE_SHARE)) < 0)
		panic("sys_page_alloc: %e", r);

	// check fork
	if ((r = fork()) < 0)
		panic("fork: %e", r);
	if (r == 0) {
		strcpy(VA, msg);
		exit();
	}
	wait(r);
	cprintf("fork handles PTE_SHARE %s\n", strcmp(VA, msg) == 0 ? "right" : "wrong");

	// check spawn
	if ((r = spawnl("/testptelibrary", "testptelibrary", "arg", 0)) < 0)
		panic("spawn: %e", r);
	wait(r);
	cprintf("spawn handles PTE_SHARE %s\n", strcmp(VA, msg2) == 0 ? "right" : "wrong");
}

void
childofspawn(void)
{
	strcpy(VA, msg2);
	exit();
}
