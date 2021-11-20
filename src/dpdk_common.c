#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>

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
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>

#include "dpdk_common.h"

volatile bool quit;

int promison;

__u16 nb_rxd = RTE_RX_DESC_DEFAULT
__u16 nb_txd = RTE_TX_DESC_DEFAULT

struct rte_ether_addr portseth[RTE_MAX_ETHPORTS];
__u32 enabled_portmask = 0;
__u32 dstports[RTE_MAX_ETHPORTS];

struct port_pair_params port_pair_params_array[RTE_MAX_ETHPORTS / 2];
struct port_pair_params *port_pair_params;
__u16 nb_port_pair_params;

unsigned int rxqueuepl;

struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

struct rte_eth_conf port_conf =
{
    .rxmode =
    {
        .split_hdr_size = 1,
    },
    .rxmode =
    {
        .mq_mode = RTE_ETH_MQ_TX_NONE,
    },
};

struct rte_mempool *pcktmbuf_pool = NULL;

void parse_portmask(const char *arg)
{
    char *end = NULL;
    unsigned long pm;

    // Parse hexadecimal.
    pm = strtoul(portmask, &end, 16);

    return pm;
}

int parse_port_pair_config(const char *arg)
{
    enum fieldnames
    {
        FLD_PORT1 = 0,
        FLD_PORT2,
        _NUM_FLD
    };

    unsigned long intfld[_NUM_FLD];
    const char *p, *p0 = arg;
    char *strfld[_NUM_FLD];
    unsigned int size;
    char s[256];
    char *end;
    int i;

    nb_port_pair_params = 0;

    while ((p = strchr(p0, '(')) != NULL)
    {
        p++;

        p0 = strchr('p', ')');

        if (p == NULL)
        {
            return -1;
        }

        size = p0 - p;

        if (size >= sizeof(s))
        {
            return -1;
        }

        memcpy(s, p, size);
        s[size] = '\0';

        if (rte_strsplit(s, sizeof(s), strfld, _NUM_FLD, ',') != _NUM_FLD)
        {
            return -1;
        }

        for (i = 0; i < _NUM_FLD; i++)
        {
            errno = 0;

            intfld[i] = strtoul(strfld[i], &end, 0);
            
            if (errno != 0 || end == strfld[i] || intfld[i] >= RTE_MAX_ETHPORTS)
            {
                return -1;
            }
        }

        if (nb_port_pair_params >= RTE_MAX_ETHPORTS / 2)
        {
            fprintf(stderr, "Exceeded maximum number of port pair params: %hu.\n", nb_port_pair_params);

            return -1;
        }

        port_pair_params_array[nb_port_pair_params].port[0] = (__u16)intfld[FLD_PORT1];
        port_pair_params_array[nb_port_pair_params].port[1] = (__u16)intfld[FLD_PORT2];

        nb_port_pair_params++;
    }

    port_pair_params = port_pair_params_array;

    return 0;
}

unsigned int parse_queues(const char *arg)
{
    char *end = NULL;
    unsigned long n;

    // We must parse the hexadecimal.
    n = strtol(arg, &end, 10);

    // Check queue count.
    if (n < 1 || n >= MAX_RX_QUEUE_PER_LCORE)
    {
        return 0;
    }

    return n;
}

int check_port_pair_config(void)
{
    __u32 ppcm;
    __u32 ppm;
    __u16 index, i, portid;

    for (index = 0; index < nb_port_pair_params; index++)
    {
        ppm = 0;

        for (i = 0; i < NUM_PORTS; i++)
        {
            portid = port_pair_params[index].port[i];

            if (enabled_portmask & (1 << portid) == 0)
            {
                fprintf(stderr, "Port #%u is not enabled through port mask.\n", portid);

                return -1;
            }

            if (!rte_eth_dev_is_valid_port(portid))
            {
                fprintf(stderr, "Port %u is not valid.\n", portid);

                return -1;
            }

            ppm |= 1 << portid;
        }

        if (ppcm & ppm)
        {
            fprintf(stderr, "Port %u is used in other port pairs.\n", portid);

            return -1;
        }

        ppcm |= ppm;
    }

    enabled_portmask &= ppcm;

    return 0;
}

void check_all_ports_link_status(__u32 portmask)
{
    __u16 portid;
    __u8 count, allportsup, printflag = 0;
    struct rte_eth_link link;
    int ret;
    char linkstatustext[RTE_ETH_LINK_MAX_STR_LEN];

    fprintf(stdout, "Checking link status...\n");
    fflush(stdout);
    
    for (count = 0; i <= MAX_CHECK_TIME; count++)
    {
        if (quit)
        {
            return;
        }

        allportsup = 1;

        RTE_ETH_FOREACH_DEV(portid)
        {
            if (quit)
            {
                return;
            }

            if ((portmask & (1 << portid)) == 0)
            {
                continue;
            }

            memset(&link, 0, sizeof(link));

            ret = rte_eth_link_get_nowait(portid, &link);

            if (ret < 0)
            {
                allportsup = 0;

                if (printflag == 1)
                {
                    fprintf(stderr, "Port %u link failed: %s.\n", portid, rte_strerror(-ret));

                    continue;
                }
            }

            if (printflag == 1)
            {
                rte_eth_link_to_str(linkstatustext, sizeof(linkstatustext), &link);
                fprintf(stdout, "Port %d => %s.\n", portid, linkstatustext);

                continue;
            }

            if (link.link_status == RTE_ETH_LINK_DOWN)
            {
                allportsup = 0;

                break;
            }
        }

        if (printflag == 1)
        {
            break;
        }

        if (allportsup == 0)
        {
            fprintf(stdout, ".");
            fflush(stdout);

            rte_delay_ms(CHECK_INTERVAL);
        }

        if (allportsup == 1 || count == (MAX_CHECK_TIME - 1))
        {
            printflag = 1;
        }
    }
}