#include <inc/lib.h>
#include <lwip/sockets.h>

static ssize_t devsock_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devsock_write(struct Fd *fd, const void *buf, size_t n);
static int devsock_close(struct Fd *fd);
static int devsock_stat(struct Fd *fd, struct Stat *stat);

struct Dev devsock =
{
	.dev_id =	's',
	.dev_name =	"sock",
	.dev_read =	devsock_read,
	.dev_write =	devsock_write,
	.dev_close =	devsock_close,
	.dev_stat =	devsock_stat,
};

static int
fd2sockid(int fd)
{
	struct Fd *sfd;
	int r;

	if ((r = fd_lookup(fd, &sfd)) < 0)
		return r;
	if (sfd->fd_dev_id != devsock.dev_id)
		return -E_NOT_SUPP;
	return sfd->fd_sock.sockid;
}

static int
alloc_sockfd(int sockid)
{
	struct Fd *sfd;
	int r;

	if ((r = fd_alloc(&sfd)) < 0
	    || (r = sys_page_alloc(0, sfd, PTE_P|PTE_W|PTE_U|PTE_SHARE)) < 0) {
		nsipc_close(sockid);
		return r;
	}

	sfd->fd_dev_id = devsock.dev_id;
	sfd->fd_omode = O_RDWR;
	sfd->fd_sock.sockid = sockid;
	return fd2num(sfd);
}

int
accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int r;
	if ((r = fd2sockid(s)) < 0)
		return r;
	if ((r = nsipc_accept(r, addr, addrlen)) < 0)
		return r;
	return alloc_sockfd(r);
}

int
bind(int s, struct sockaddr *name, socklen_t namelen)
{
	int r;
	if ((r = fd2sockid(s)) < 0)
		return r;
	return nsipc_bind(r, name, namelen);
}

int
shutdown(int s, int how)
{
	int r;
	if ((r = fd2sockid(s)) < 0)
		return r;
	return nsipc_shutdown(r, how);
}

static int
devsock_close(struct Fd *fd)
{
	if (pageref(fd) == 1)
		return nsipc_close(fd->fd_sock.sockid);
	else
		return 0;
}

int
connect(int s, const struct sockaddr *name, socklen_t namelen)
{
	int r;
	if ((r = fd2sockid(s)) < 0)
		return r;
	return nsipc_connect(r, name, namelen);
}

int
listen(int s, int backlog)
{
	int r;
	if ((r = fd2sockid(s)) < 0)
		return r;
	return nsipc_listen(r, backlog);
}

static ssize_t
devsock_read(struct Fd *fd, void *buf, size_t n)
{
	return nsipc_recv(fd->fd_sock.sockid, buf, n, 0);
}

static ssize_t
devsock_write(struct Fd *fd, const void *buf, size_t n)
{
	return nsipc_send(fd->fd_sock.sockid, buf, n, 0);
}

static int
devsock_stat(struct Fd *fd, struct Stat *stat)
{
	strcpy(stat->st_name, "<sock>");
	return 0;
}

int
socket(int domain, int type, int protocol)
{
	int r;
	if ((r = nsipc_socket(domain, type, protocol)) < 0)
		return r;
	return alloc_sockfd(r);
}
