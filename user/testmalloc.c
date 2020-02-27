#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	char *buf;
	int n;
	void *v;

	while (1) {
		buf = readline("> ");
		if (buf == 0)
			exit();
		if (memcmp(buf, "free ", 5) == 0) {
			v = (void*) strtol(buf + 5, 0, 0);
			free(v);
		} else if (memcmp(buf, "malloc ", 7) == 0) {
			n = strtol(buf + 7, 0, 0);
			v = malloc(n);
			printf("\t0x%x\n", (uintptr_t) v);
		} else
			printf("?unknown command\n");
	}
}
