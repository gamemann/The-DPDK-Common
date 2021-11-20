#ifndef DPDK_COMMON_HEADER
#define DPDK_COMMON_HEADER

#include <linux/types.h>

/* Common defines */
#define MAX_PCKT_BURST 32
#define BURST_TX_DRAIN_US 100
#define MEMPOOL_CACHE_SIZE 256
#define RTE_RX_DESC_DEFAULT 1024
#define RTE_TX_DESC_DEFAULT 1024
#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16
#define NUM_PORTS 2
#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16
#define MAX_TIMER_PERIOD 86400
#define CHECK_INTERVAL 100
#define MAX_CHECK_TIME 90

struct port_pair_params
{
    __u16 port[NUM_PORTS];
} __rte_cache_aligned;

struct lcore_queue_conf
{
    unsigned numrxport;
    unsigned rxportlist[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;

/* Extern global variables */
extern volatile bool quit;
extern int promison;
extern __u16 nb_rxd;
extern __u16 nb_txd;
extern struct rte_ether_addr portseth;
extern __u32 enabled_portmask;
extern __u32 dstports;
extern struct port_pair_params port_pair_params_array;
extern struct port_pair_params *port_pair_params;
extern __u16 nb_port_pair_params;
extern unsigned int rxqueuepl;
extern struct lcore_queue_conf lcore_queue_conf;
extern struct rte_eth_dev_tx_buffer *tx_buffer;
extern struct rte_eth_conf port_conf;
extern struct rte_mempool *pcktmbuf_pool;

/* Define functions */
void parse_portmask(const char *arg);
int parse_port_pair_config(const char *arg);
unsigned int parse_queues(const char *arg);
#endif