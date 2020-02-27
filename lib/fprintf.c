#include <inc/lib.h>

// Collect up to 256 characters into a buffer
// and perform ONE system call to print all of them,
// in order to make the lines output to the console atomic
// and prevent interrupts from causing context switches
// in the middle of a console output line and such.
struct printbuf {
	int fd;		// file descriptor
	int idx;	// current buffer index
	ssize_t result;	// accumulated results from write
	int error;	// first error that occurred
	char buf[256];
};


static void
writebuf(struct printbuf *b)
{
	if (b->error > 0) {
		ssize_t result = write(b->fd, b->buf, b->idx);
		if (result > 0)
			b->result += result;
		if (result != b->idx) // error, or wrote less than supplied
			b->error = (result < 0 ? result : 0);
	}
}

static void
putch(int ch, void *thunk)
{
	struct printbuf *b = (struct printbuf *) thunk;
	b->buf[b->idx++] = ch;
	if (b->idx == 256) {
		writebuf(b);
		b->idx = 0;
	}
}

int
vfprintf(int fd, const char *fmt, va_list ap)
{
	struct printbuf b;

	b.fd = fd;
	b.idx = 0;
	b.result = 0;
	b.error = 1;
	vprintfmt(putch, &b, fmt, ap);
	if (b.idx > 0)
		writebuf(&b);

	return (b.result ? b.result : b.error);
}

int
fprintf(int fd, const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vfprintf(fd, fmt, ap);
	va_end(ap);

	return cnt;
}

int
printf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vfprintf(1, fmt, ap);
	va_end(ap);

	return cnt;
}

