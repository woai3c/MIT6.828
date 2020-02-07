
#include <inc/string.h>
#include <inc/lib.h>

void
cputchar(int ch)
{
	char c = ch;

	// Unlike standard Unix's putchar,
	// the cputchar function _always_ outputs to the system console.
	sys_cputs(&c, 1);
}

int
getchar(void)
{
	int r;
	// sys_cgetc does not block, but getchar should.
	while ((r = sys_cgetc()) == 0)
		;
	return r;
}


