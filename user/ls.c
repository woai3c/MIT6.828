#include <inc/lib.h>

int flag[256];

void lsdir(const char*, const char*);
void ls1(const char*, bool, off_t, const char*);

void
ls(const char *path, const char *prefix)
{
	int r;
	struct Stat st;

	if ((r = stat(path, &st)) < 0)
		panic("stat %s: %e", path, r);
	if (st.st_isdir && !flag['d'])
		lsdir(path, prefix);
	else
		ls1(0, st.st_isdir, st.st_size, path);
}

void
lsdir(const char *path, const char *prefix)
{
	int fd, n;
	struct File f;

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %e", path, fd);
	while ((n = readn(fd, &f, sizeof f)) == sizeof f)
		if (f.f_name[0])
			ls1(prefix, f.f_type==FTYPE_DIR, f.f_size, f.f_name);
	if (n > 0)
		panic("short read in directory %s", path);
	if (n < 0)
		panic("error reading directory %s: %e", path, n);
}

void
ls1(const char *prefix, bool isdir, off_t size, const char *name)
{
	const char *sep;

	if(flag['l'])
		printf("%11d %c ", size, isdir ? 'd' : '-');
	if(prefix) {
		if (prefix[0] && prefix[strlen(prefix)-1] != '/')
			sep = "/";
		else
			sep = "";
		printf("%s%s", prefix, sep);
	}
	printf("%s", name);
	if(flag['F'] && isdir)
		printf("/");
	printf("\n");
}

void
usage(void)
{
	printf("usage: ls [-dFl] [file...]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int i;
	struct Argstate args;

	argstart(&argc, argv, &args);
	while ((i = argnext(&args)) >= 0)
		switch (i) {
		case 'd':
		case 'F':
		case 'l':
			flag[i]++;
			break;
		default:
			usage();
		}

	if (argc == 1)
		ls("/", "");
	else {
		for (i = 1; i < argc; i++)
			ls(argv[i], argv[i]);
	}
}

