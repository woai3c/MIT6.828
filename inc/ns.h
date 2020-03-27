// See COPYRIGHT for copyright information.

#ifndef JOS_INC_NS_H
#define JOS_INC_NS_H

#include <inc/types.h>
#include <inc/mmu.h>
#include <lwip/sockets.h>

struct jif_pkt {
	int jp_len;
	char jp_data[0];
};

// Definitions for requests from clients to network server
enum {
	// The following messages pass a page containing an Nsipc.
	// Accept returns a Nsret_accept on the request page.
	NSREQ_ACCEPT = 1,
	NSREQ_BIND,
	NSREQ_SHUTDOWN,
	NSREQ_CLOSE,
	NSREQ_CONNECT,
	NSREQ_LISTEN,
	// Recv returns a Nsret_recv on the request page.
	NSREQ_RECV,
	NSREQ_SEND,
	NSREQ_SOCKET,

	// The following two messages pass a page containing a struct jif_pkt
	NSREQ_INPUT,
	// NSREQ_OUTPUT, unlike all other messages, is sent *from* the
	// network server, to the output environment
	NSREQ_OUTPUT,

	// The following message passes no page
	NSREQ_TIMER,
};

union Nsipc {
	struct Nsreq_accept {
		int req_s;
		socklen_t req_addrlen;
	} accept;

	struct Nsret_accept {
		struct sockaddr ret_addr;
		socklen_t ret_addrlen;
	} acceptRet;

	struct Nsreq_bind {
		int req_s;
		struct sockaddr req_name;
		socklen_t req_namelen;
	} bind;

	struct Nsreq_shutdown {
		int req_s;
		int req_how;
	} shutdown;

	struct Nsreq_close {
		int req_s;
	} close;

	struct Nsreq_connect {
		int req_s;
		struct sockaddr req_name;
		socklen_t req_namelen;
	} connect;

	struct Nsreq_listen {
		int req_s;
		int req_backlog;
	} listen;

	struct Nsreq_recv {
		int req_s;
		int req_len;
		unsigned int req_flags;
	} recv;

	struct Nsret_recv {
		char ret_buf[0];
	} recvRet;

	struct Nsreq_send {
		int req_s;
		int req_size;
		unsigned int req_flags;
		char req_buf[0];
	} send;

	struct Nsreq_socket {
		int req_domain;
		int req_type;
		int req_protocol;
	} socket;

	struct jif_pkt pkt;

	// Ensure Nsipc is one page
	char _pad[PGSIZE];
};

#endif // !JOS_INC_NS_H
