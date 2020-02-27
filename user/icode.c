#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int fd, n, r;
	char buf[512+1];

	binaryname = "icode";

	cprintf("icode startup\n");

	cprintf("icode: open /motd\n");
	if ((fd = open("/motd", O_RDONLY)) < 0)
		panic("icode: open /motd: %e", fd);

	cprintf("icode: read /motd\n");
	while ((n = read(fd, buf, sizeof buf-1)) > 0)
		sys_cputs(buf, n);

	cprintf("icode: close /motd\n");
	close(fd);

	cprintf("icode: spawn /init\n");
	if ((r = spawnl("/init", "init", "initarg1", "initarg2", (char*)0)) < 0)
		panic("icode: spawn /init: %e", r);

	cprintf("icode: exiting\n");
}
