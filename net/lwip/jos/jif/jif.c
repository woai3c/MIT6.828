/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <inc/lib.h>
#include <inc/ns.h>

#include <jif/jif.h>

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include <lwip/stats.h>

#include <netif/etharp.h>

#define PKTMAP		0x10000000

struct jif {
    struct eth_addr *ethaddr;
    envid_t envid;
};

static void
low_level_init(struct netif *netif)
{
    int r;

    netif->hwaddr_len = 6;
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST;

    // MAC address is hardcoded to eliminate a system call
    netif->hwaddr[0] = 0x52;
    netif->hwaddr[1] = 0x54;
    netif->hwaddr[2] = 0x00;
    netif->hwaddr[3] = 0x12;
    netif->hwaddr[4] = 0x34;
    netif->hwaddr[5] = 0x56;
}

/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
    int r = sys_page_alloc(0, (void *)PKTMAP, PTE_U|PTE_W|PTE_P);
    if (r < 0)
	panic("jif: could not allocate page of memory");
    struct jif_pkt *pkt = (struct jif_pkt *)PKTMAP;

    struct jif *jif;
    jif = netif->state;

    char *txbuf = pkt->jp_data;
    int txsize = 0;
    struct pbuf *q;
    for (q = p; q != NULL; q = q->next) {
	/* Send the data from the pbuf to the interface, one pbuf at a
	   time. The size of the data in each pbuf is kept in the ->len
	   variable. */

	if (txsize + q->len > 2000)
	    panic("oversized packet, fragment %d txsize %d\n", q->len, txsize);
	memcpy(&txbuf[txsize], q->payload, q->len);
	txsize += q->len;
    }

    pkt->jp_len = txsize;

    ipc_send(jif->envid, NSREQ_OUTPUT, (void *)pkt, PTE_P|PTE_W|PTE_U);
    sys_page_unmap(0, (void *)pkt);

    return ERR_OK;
}

/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
static struct pbuf *
low_level_input(void *va)
{
    struct jif_pkt *pkt = (struct jif_pkt *)va;
    s16_t len = pkt->jp_len;

    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == 0)
	return 0;

    /* We iterate over the pbuf chain until we have read the entire
     * packet into the pbuf. */
    void *rxbuf = (void *) pkt->jp_data;
    int copied = 0;
    struct pbuf *q;
    for (q = p; q != NULL; q = q->next) {
	/* Read enough bytes to fill this pbuf in the chain. The
	 * available data in the pbuf is given by the q->len
	 * variable. */
	int bytes = q->len;
	if (bytes > (len - copied))
	    bytes = len - copied;
	memcpy(q->payload, rxbuf + copied, bytes);
	copied += bytes;
    }

    return p;
}
/*
 * jif_output():
 *
 * This function is called by the TCP/IP stack when an IP packet
 * should be sent. It calls the function called low_level_output() to
 * do the actual transmission of the packet.
 *
 */

static err_t
jif_output(struct netif *netif, struct pbuf *p,
      struct ip_addr *ipaddr)
{
    /* resolve hardware address, then send (or queue) packet */
    return etharp_output(netif, p, ipaddr);
}

/*
 * jif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */

void
jif_input(struct netif *netif, void *va)
{
    struct jif *jif;
    struct eth_hdr *ethhdr;
    struct pbuf *p;

    jif = netif->state;
  
    /* move received packet into a new pbuf */
    p = low_level_input(va);

    /* no packet could be read, silently ignore this */
    if (p == NULL) return;
    /* points to packet payload, which starts with an Ethernet header */
    ethhdr = p->payload;

    switch (htons(ethhdr->type)) {
    case ETHTYPE_IP:
	/* update ARP table */
	etharp_ip_input(netif, p);
	/* skip Ethernet header */
	pbuf_header(p, -(int)sizeof(struct eth_hdr));
	/* pass to network layer */
	netif->input(p, netif);
	break;
      
    case ETHTYPE_ARP:
	/* pass p to ARP module  */
	etharp_arp_input(netif, jif->ethaddr, p);
	break;

    default:
	pbuf_free(p);
    }
}

/*
 * jif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */

err_t
jif_init(struct netif *netif)
{
    struct jif *jif;
    envid_t *output_envid; 

    jif = mem_malloc(sizeof(struct jif));

    if (jif == NULL) {
	LWIP_DEBUGF(NETIF_DEBUG, ("jif_init: out of memory\n"));
	return ERR_MEM;
    }

    output_envid = (envid_t *)netif->state;

    netif->state = jif;
    netif->output = jif_output;
    netif->linkoutput = low_level_output;
    memcpy(&netif->name[0], "en", 2);

    jif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
    jif->envid = *output_envid; 

    low_level_init(netif);

    etharp_init();

    // qemu user-net is dumb; if the host OS does not send and ARP request
    // first, the qemu will send packets destined for the host using the mac
    // addr 00:00:00:00:00; do a arp request for the user-net NAT at 10.0.2.2
    uint32_t ipaddr = inet_addr("10.0.2.2");
    etharp_query(netif, (struct ip_addr *) &ipaddr, 0);

    return ERR_OK;
}
