#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

// LAB 6: Your driver code here

volatile void *bar_va;
struct e1000_tdh *tdh;
struct e1000_tdt *tdt;
struct e1000_tx_desc tx_desc_array[TXDESCS];
char tx_buffer_array[TXDESCS][TX_PKT_SIZE];


struct e1000_rdh *rdh;
struct e1000_rdt *rdt;
struct e1000_rx_desc rx_desc_array[RXDESCS];
char rx_buffer_array[RXDESCS][RX_PKT_SIZE];

uint32_t E1000_MAC[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

int
e1000_attachfn(struct pci_func *pcif)
{
	pci_func_enable(pcif);
	cprintf("reg_base:%x, reg_size:%x\n", pcif->reg_base[0], pcif->reg_size[0]);
			
	//Exercise4 create virtual memory mapping
	bar_va = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	
	uint32_t *status_reg = E1000REG(E1000_STATUS);
	assert(*status_reg == 0x80080783);

	e1000_transmit_init();
	e1000_receive_init();

	return 0;
}

//see section 14.5 in https://pdos.csail.mit.edu/6.828/2018/labs/lab6/e1000_hw.h
static void
e1000_transmit_init()
{
       int i;
       for (i = 0; i < TXDESCS; i++) {
               tx_desc_array[i].addr = PADDR(tx_buffer_array[i]);
               tx_desc_array[i].cmd = 0;
               tx_desc_array[i].status |= E1000_TXD_STAT_DD;
       }
			 //TDLEN register
       struct e1000_tdlen *tdlen = (struct e1000_tdlen *)E1000REG(E1000_TDLEN);
       tdlen->len = TXDESCS;
			 
			 //TDBAL register
       uint32_t *tdbal = (uint32_t *)E1000REG(E1000_TDBAL);
       *tdbal = PADDR(tx_desc_array);

			 //TDBAH regsiter
       uint32_t *tdbah = (uint32_t *)E1000REG(E1000_TDBAH);
       *tdbah = 0;

		   //TDH register, should be init 0
       tdh = (struct e1000_tdh *)E1000REG(E1000_TDH);
       tdh->tdh = 0;

		   //TDT register, should be init 0
       tdt = (struct e1000_tdt *)E1000REG(E1000_TDT);
       tdt->tdt = 0;

			 //TCTL register
       struct e1000_tctl *tctl = (struct e1000_tctl *)E1000REG(E1000_TCTL);
       tctl->en = 1;
       tctl->psp = 1;
       tctl->ct = 0x10;
       tctl->cold = 0x40;

			 //TIPG register
       struct e1000_tipg *tipg = (struct e1000_tipg *)E1000REG(E1000_TIPG);
       tipg->ipgt = 10;
       tipg->ipgr1 = 4;
       tipg->ipgr2 = 6;
}

static void
get_ra_address(uint32_t mac[], uint32_t *ral, uint32_t *rah)
{
       uint32_t low = 0, high = 0;
       int i;

       for (i = 0; i < 4; i++) {
               low |= mac[i] << (8 * i);
       }

       for (i = 4; i < 6; i++) {
               high |= mac[i] << (8 * i);
       }

       *ral = low;
       *rah = high | E1000_RAH_AV;
}

static void
e1000_receive_init()
{			 //RDBAL and RDBAH register
       uint32_t *rdbal = (uint32_t *)E1000REG(E1000_RDBAL);
       uint32_t *rdbah = (uint32_t *)E1000REG(E1000_RDBAH);
       *rdbal = PADDR(rx_desc_array);
       *rdbah = 0;

       int i;
       for (i = 0; i < RXDESCS; i++) {
               rx_desc_array[i].addr = PADDR(rx_buffer_array[i]);
       }
			 //RDLEN register
       struct e1000_rdlen *rdlen = (struct e1000_rdlen *)E1000REG(E1000_RDLEN);
       rdlen->len = RXDESCS;

			 //RDH and RDT register
       rdh = (struct e1000_rdh *)E1000REG(E1000_RDH);
       rdt = (struct e1000_rdt *)E1000REG(E1000_RDT);
       rdh->rdh = 0;
       rdt->rdt = RXDESCS-1;

       uint32_t *rctl = (uint32_t *)E1000REG(E1000_RCTL);
       *rctl = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC;

       uint32_t *ra = (uint32_t *)E1000REG(E1000_RA);
       uint32_t ral, rah;
       get_ra_address(E1000_MAC, &ral, &rah);
       ra[0] = ral;
       ra[1] = rah;
}



int
e1000_transmit(void *data, size_t len)
{
       uint32_t current = tdt->tdt;		//tail index in queue
       if(!(tx_desc_array[current].status & E1000_TXD_STAT_DD)) {
               return -E_TRANSMIT_RETRY;
       }
       tx_desc_array[current].length = len;
       tx_desc_array[current].status &= ~E1000_TXD_STAT_DD;
       tx_desc_array[current].cmd |= (E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS);
       memcpy(tx_buffer_array[current], data, len);
       uint32_t next = (current + 1) % TXDESCS;
       tdt->tdt = next;
       return 0;
}

int
e1000_receive(void *addr, size_t *len)
{
       static int32_t next = 0;
       if(!(rx_desc_array[next].status & E1000_RXD_STAT_DD)) {	//simply tell client to retry
               return -E_RECEIVE_RETRY;
       }
       if(rx_desc_array[next].errors) {
               cprintf("receive errors\n");
               return -E_RECEIVE_RETRY;
       }
       *len = rx_desc_array[next].length;
       memcpy(addr, rx_buffer_array[next], *len);

       rdt->rdt = (rdt->rdt + 1) % RXDESCS;
       next = (next + 1) % RXDESCS;
       return 0;
}

