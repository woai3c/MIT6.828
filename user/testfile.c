#include <inc/lib.h>

const char *msg = "This is the NEW message of the day!\n\n";

#define FVA ((struct Fd*)0xCCCCC000)

static int
xopen(const char *path, int mode)
{
	extern union Fsipc fsipcbuf;
	envid_t fsenv;
	
	strcpy(fsipcbuf.open.req_path, path);
	fsipcbuf.open.req_omode = mode;

	fsenv = ipc_find_env(ENV_TYPE_FS);
	ipc_send(fsenv, FSREQ_OPEN, &fsipcbuf, PTE_P | PTE_W | PTE_U);
	return ipc_recv(NULL, FVA, NULL);
}

void
umain(int argc, char **argv)
{
	int r, f, i;
	struct Fd *fd;
	struct Fd fdcopy;
	struct Stat st;
	char buf[512];

	// We open files manually first, to avoid the FD layer
	if ((r = xopen("/not-found", O_RDONLY)) < 0 && r != -E_NOT_FOUND)
		panic("serve_open /not-found: %e", r);
	else if (r >= 0)
		panic("serve_open /not-found succeeded!");

	if ((r = xopen("/newmotd", O_RDONLY)) < 0)
		panic("serve_open /newmotd: %e", r);
	if (FVA->fd_dev_id != 'f' || FVA->fd_offset != 0 || FVA->fd_omode != O_RDONLY)
		panic("serve_open did not fill struct Fd correctly\n");
	cprintf("serve_open is good\n");

	if ((r = devfile.dev_stat(FVA, &st)) < 0)
		panic("file_stat: %e", r);
	if (strlen(msg) != st.st_size)
		panic("file_stat returned size %d wanted %d\n", st.st_size, strlen(msg));
	cprintf("file_stat is good\n");

	memset(buf, 0, sizeof buf);
	if ((r = devfile.dev_read(FVA, buf, sizeof buf)) < 0)
		panic("file_read: %e", r);
	if (strcmp(buf, msg) != 0)
		panic("file_read returned wrong data");
	cprintf("file_read is good\n");

	if ((r = devfile.dev_close(FVA)) < 0)
		panic("file_close: %e", r);
	cprintf("file_close is good\n");

	// We're about to unmap the FD, but still need a way to get
	// the stale filenum to serve_read, so we make a local copy.
	// The file server won't think it's stale until we unmap the
	// FD page.
	fdcopy = *FVA;
	sys_page_unmap(0, FVA);

	if ((r = devfile.dev_read(&fdcopy, buf, sizeof buf)) != -E_INVAL)
		panic("serve_read does not handle stale fileids correctly: %e", r);
	cprintf("stale fileid is good\n");

	// Try writing
	if ((r = xopen("/new-file", O_RDWR|O_CREAT)) < 0)
		panic("serve_open /new-file: %e", r);

	if ((r = devfile.dev_write(FVA, msg, strlen(msg))) != strlen(msg))
		panic("file_write: %e", r);
	cprintf("file_write is good\n");

	FVA->fd_offset = 0;
	memset(buf, 0, sizeof buf);
	if ((r = devfile.dev_read(FVA, buf, sizeof buf)) < 0)
		panic("file_read after file_write: %e", r);
	if (r != strlen(msg))
		panic("file_read after file_write returned wrong length: %d", r);
	if (strcmp(buf, msg) != 0)
		panic("file_read after file_write returned wrong data");
	cprintf("file_read after file_write is good\n");

	// Now we'll try out open
	if ((r = open("/not-found", O_RDONLY)) < 0 && r != -E_NOT_FOUND)
		panic("open /not-found: %e", r);
	else if (r >= 0)
		panic("open /not-found succeeded!");

	if ((r = open("/newmotd", O_RDONLY)) < 0)
		panic("open /newmotd: %e", r);
	fd = (struct Fd*) (0xD0000000 + r*PGSIZE);
	if (fd->fd_dev_id != 'f' || fd->fd_offset != 0 || fd->fd_omode != O_RDONLY)
		panic("open did not fill struct Fd correctly\n");
	cprintf("open is good\n");

	// Try files with indirect blocks
	if ((f = open("/big", O_WRONLY|O_CREAT)) < 0)
		panic("creat /big: %e", f);
	memset(buf, 0, sizeof(buf));
	for (i = 0; i < (NDIRECT*3)*BLKSIZE; i += sizeof(buf)) {
		*(int*)buf = i;
		if ((r = write(f, buf, sizeof(buf))) < 0)
			panic("write /big@%d: %e", i, r);
	}
	close(f);

	if ((f = open("/big", O_RDONLY)) < 0)
		panic("open /big: %e", f);
	for (i = 0; i < (NDIRECT*3)*BLKSIZE; i += sizeof(buf)) {
		*(int*)buf = i;
		if ((r = readn(f, buf, sizeof(buf))) < 0)
			panic("read /big@%d: %e", i, r);
		if (r != sizeof(buf))
			panic("read /big from %d returned %d < %d bytes",
			      i, r, sizeof(buf));
		if (*(int*)buf != i)
			panic("read /big from %d returned bad data %d",
			      i, *(int*)buf);
	}
	close(f);
	cprintf("large file is good\n");
}

