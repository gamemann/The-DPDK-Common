#ifndef DPDK_COMMON_HEADER
#define DPDK_COMMON_HEADER

/* Include main headers from the DPDK */
#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>
#ifdef USE_HASH_TABLES
#include <rte_hash.h>
#include <rte_jhash.h>
#endif

#include <linux/types.h>

/* Common defines */
#define MAX_PCKT_BURST 32
#define BURST_TX_DRAIN_US 100
#define MEMPOOL_CACHE_SIZE 256
#define RTE_RX_DESC_DEFAULT 1024
#define RTE_TX_DESC_DEFAULT 1024
#define MAX_RX_PORTS_PER_LCORE 16
#define MAX_TX_PORTS_PER_LCORE 16
#define MAX_RX_QUEUES_PER_PORT 16
#define MAX_TX_QUEUES_PER_PORT 16
#define NUM_PORTS 2
#define MAX_TIMER_PERIOD 86400
#define CHECK_INTERVAL 100
#define MAX_CHECK_TIME 90

/* Structures */
struct port_pair_params
{
    __u16 port[NUM_PORTS];
} __rte_cache_aligned;

struct lcore_port_conf
{
    unsigned num_rx_ports;
    unsigned rx_port_list[MAX_RX_PORTS_PER_LCORE];
    unsigned num_tx_ports;
    unsigned tx_port_list[MAX_TX_PORTS_PER_LCORE];
} __rte_cache_aligned;

struct port_conf
{
    unsigned int rx : 1;
    unsigned int tx : 1;
    struct rte_ether_addr mac;
    struct rte_eth_dev_tx_buffer *tx_buffer;
    unsigned int tx_port;
};

struct dpdkc_ret
{
    char *gen_msg;
    int err_num;
    int port_id;
    int rx_id;
    int tx_id;
    __u32 data;
    void *dataptr;
};

/* Global variables for use in other objects/executables using this header file */
#ifndef DPDK_COMMON_IGNORE_GLOBAL_VARS
extern volatile __u8 quit;
extern __u16 nb_rxd;
extern __u16 nb_txd;
extern __u32 enabled_port_mask;
extern struct port_pair_params port_pair_params_array[RTE_MAX_ETHPORTS / 2];
extern struct port_pair_params *port_pair_params;
extern __u16 nb_port_pair_params;
extern struct port_conf ports[RTE_MAX_ETHPORTS];
extern unsigned int rx_port_pl;
extern unsigned int tx_port_pl;
extern unsigned int rx_queue_pp;
extern unsigned int tx_queue_pp;
extern struct lcore_port_conf lcore_port_conf[RTE_MAX_LCORE];
extern struct rte_eth_conf port_conf;
extern struct rte_mempool *pcktmbuf_pool;
extern __u16 port_id;
extern __u16 nb_ports;
extern __u16 nb_ports_available;
extern unsigned int lcore_id;
extern unsigned int nb_lcores;
#endif

/* Functions for use in other objects/executables using this header file */
struct dpdkc_ret dpdkc_ret_init();
struct dpdkc_ret dpdkc_parse_arg_port_mask(const char *arg);
struct dpdkc_ret dpdkc_parse_arg_port_pair_config(const char *arg);
struct dpdkc_ret dpdkc_parse_arg_queues(const char *arg, int tx);
struct dpdkc_ret dpdkc_check_port_pair_config(void);
void dpdkc_check_link_status();
struct dpdkc_ret dpdkc_eal_init(int argc, char **argv);
struct dpdkc_ret dpdkc_get_nb_ports();
struct dpdkc_ret dpdkc_check_port_pairs();
struct dpdkc_ret dpdkc_ports_are_valid();
void dpdkc_reset_dst_ports();
void dpdkc_populate_dst_ports();
struct dpdkc_ret dpdkc_ports_queues_mapping();
struct dpdkc_ret dpdkc_create_mbuf();
struct dpdkc_ret dpdkc_ports_queues_init(int promisc, int rx_queue, int tx_queue);
struct dpdkc_ret dpdkc_get_available_lcore_count();
struct dpdkc_ret dpdkc_ports_available();
void dpdkc_launch_and_run(void *f);
struct dpdkc_ret dpdkc_port_stop_and_remove();
struct dpdkc_ret dpdkc_eal_cleanup();
void dpdkc_check_ret(struct dpdkc_ret *ret);
#ifdef USE_HASH_TABLES
int check_and_del_lru_from_hash_table(void *tbl, __u32 max_entries);
#endif
#endif