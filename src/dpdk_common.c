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

#include "dpdk_common.h"

/* Global variables that may be used in other programs. */
// Variable to use for signals.
volatile __u8 quit;

// The RX and TX descriptor sizes (using defaults).
__u16 nb_rxd = RTE_RX_DESC_DEFAULT;
__u16 nb_txd = RTE_TX_DESC_DEFAULT;

// Array to store the MAC addresses of all ports.
struct rte_ether_addr ports_eth[RTE_MAX_ETHPORTS];

// The enabled port mask.
__u32 enabled_port_mask = 0;

// Destination ports array.
__u32 dst_ports[RTE_MAX_ETHPORTS];

// Port pair params array.
struct port_pair_params port_pair_params_array[RTE_MAX_ETHPORTS / 2];

// Port pair params pointer.
struct port_pair_params *port_pair_params;

// The number of port pair parameters.
__u16 nb_port_pair_params;

// The amount of RX queues per l-core.
unsigned int rx_queue_pl;

// The queue's lcore config.
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

// The tx buffer.
struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

// The ethernet port's config to set.
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

// A pointer to the mbuf_pool for packets.
struct rte_mempool *pcktmbuf_pool = NULL;

// The current port ID.
__u16 port_id = 0;

// Number of ports and ports available.
__u16 nb_ports = 0;
__u16 nb_ports_available = 0;

// L-core ID.
unsigned int lcore_id = 0;

// Number of l-cores.
unsigned int nb_lcores = 0;

/**
 * Returns whether or not the currently set port_id is enabled with the configured port mask.
 * WARNING - Static function (cannot use outside of this file).
 * 
 * @return 1 on enabled or 0 on disabled.
**/
static int dpdkc_port_enabled()
{
    return (enabled_port_mask & (1 << port_id)) > 0;
}

/**
 * Initializes a DPDK Common result type and returns it with default values.
 * WARNING - Static function (cannot use outside of this file).
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
static struct dpdkc_ret dpdkc_ret_init()
{
    struct dpdkc_ret ret =
    {
        .err_num = 0,
        .gen_msg = NULL,
        .port_id = -1,
        .rx_id = -1,
        .tx_id = -1,
        .data = NULL
    };

    return ret;
}

/**
 * Parses the port mask argument.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret). The port mask is stored in ret->data.
**/
struct dpdkc_ret dpdkc_parse_arg_port_mask(const char *arg)
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    char *end = NULL;
    unsigned long pm;

    // Parse hexadecimal.
    pm = strtoul(arg, &end, 16);

    ret.data = (void *)&pm;

    return ret;
}

/**
 * Parses the port pair config.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_parse_arg_port_pair_config(const char *arg)
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    // For readability, we'll define these in an enum.s
    enum fieldnames
    {
        FLD_PORT1 = 0,
        FLD_PORT2,
        _NUM_FLD
    };

    // This array holds the values (integers) of the port IDs to pair.
    unsigned long int_fld[_NUM_FLD];

    // This array holds the values (strings) of the port IDs to pair.
    char *str_fld[_NUM_FLD];

    // Useful pointers for parsing port pair.  
    const char *p, *p0 = arg;

    // Buffer for data in-between each () representing a port pair.
    char s[256];
    
    // The size of the data in-between each () representing a port pair.
    unsigned int size;
    
    // Pointer to use with str to unsigned long functions.
    char *end;

    // Variable for iterations.
    int i;

    // Set the number of port pair parameters to 0.s
    nb_port_pair_params = 0;

    // Loop through each value in format (x,y).
    while ((p = strchr(p0, '(')) != NULL)
    {
        // Increase our address one.
        p++;

        // Now find the ending address where the ending ')' is.
        p0 = strchr(p, ')');

        // Check if we're NULL (otherwise may segfault).
        if (p == NULL)
        {
            ret.err_num = -1;
            ret.gen_msg = "Port pair variable p is NULL.";

            return ret;
        }

        // Get the size of the pair values.
        size = p0 - p;

        // It should not be even close to 256 bytes.
        if (size >= sizeof(s))
        {
            ret.err_num = -1;
            ret.gen_msg = "Port pair data size exceeds 256 bytes (should never happen).";

            return ret;
        }

        // Copy the port pair data to 's' char array.
        memcpy(s, p, size);
        
        // Last byte should be 0x00.
        s[size] = '\0';

        // Split string by ',' which divides both integers for port pair.
        if (rte_strsplit(s, sizeof(s), str_fld, _NUM_FLD, ',') != _NUM_FLD)
        {
            ret.err_num = -1;
            ret.gen_msg = "Failed to split port pair data.";
            
            return ret;
        }

        // Loop through _NUM_FLD (should be 2).
        for (i = 0; i < _NUM_FLD; i++)
        {
            // Resest error number.
            errno = 0;

            // Copy unsigned long to int_fld at i index.
            int_fld[i] = strtoul(str_fld[i], &end, 0);
            
            // If error isn't empty, we're locaated at the ')' character or we exceed max ethernet ports, fail.
            if (errno != 0 || end == str_fld[i] || int_fld[i] >= RTE_MAX_ETHPORTS)
            {
                ret.err_num = errno;
                ret.gen_msg = "Error number present or reached out of port pair data.";
                
                return ret;
            }
        }

        // Make sure we don't exceed max number of port pairs.
        if (nb_port_pair_params >= RTE_MAX_ETHPORTS / 2)
        {
            ret.err_num = -1;
            ret.gen_msg = "Exceeded maximum number of port pairs.";
            
            return ret;
        }

        // Assign port pair values (x, y) to port pair params array and increment number of port pairs.
        port_pair_params_array[nb_port_pair_params].port[0] = (__u16)int_fld[FLD_PORT1];
        port_pair_params_array[nb_port_pair_params].port[1] = (__u16)int_fld[FLD_PORT2];

        nb_port_pair_params++;
    }

    // Assign port pair array to port pair params.
    port_pair_params = port_pair_params_array;

    // Success!
    ret.err_num = 0;

    return ret;
}

/**
 * Parses the queue number argument.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret). The amount of queues is stored in ret->data.
**/
struct dpdkc_ret dpdkc_parse_arg_queues(const char *arg)
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    // Initialize a couple necessary variables.
    char *end = NULL;
    unsigned long n;

    // We must parse the hexadecimal.
    n = strtol(arg, &end, 10);

    // Check queue count.
    if (n < 1 || n >= MAX_RX_QUEUE_PER_LCORE)
    {
        ret.err_num = -1;
        ret.gen_msg = "Amount of queues is below 0.";

        return ret;
    }

    ret.data = (void *)&n;

    return ret;
}

/**
 * Checks the port pair config after initialization.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_check_port_pair_config(void)
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    // Port pair config mask and port pair mask.
    __u32 ppcm;
    __u32 ppm;

    // Other variables for iteration.
    __u16 index; 
    __u16 i; 
    __u16 port_id;

    // Loop through each port pair.
    for (index = 0; index < nb_port_pair_params; index++)
    {
        // Reset port 
        ppm = 0;

        // Loop through max number of ports (likely 2).
        for (i = 0; i < NUM_PORTS; i++)
        {
            // Retrieve the port ID (<i = 0>, <i = 1).
            port_id = port_pair_params[index].port[i];

            // Check if this port is enabled via the port mask.
            if (enabled_port_mask & (1 << port_id) == 0)
            {
                ret.err_num = -1;
                ret.port_id = port_id;
                ret.gen_msg = "Port is not enabled in port mask.";

                return ret;
            }

            // Check if the port is valid.
            if (!rte_eth_dev_is_valid_port(port_id))
            {
                ret.err_num = -1;
                ret.port_id = port_id;
                ret.gen_msg = "Port is not valid.";

                return ret;
            }

            // Retrieve port pair mask.
            ppm |= 1 << port_id;
        }

        // Check if this port is being used in another pair since we OR the current PPM to the PPCM variable.
        if (ppcm & ppm)
        {
            ret.err_num = -1;
            ret.port_id = port_id;
            ret.gen_msg = "Port is being used by another port pair.";

            return ret;
        }

        // OR PPM to PPCM so we can perform check above to ensure this port ID isn't in use with another port pair.
        ppcm |= ppm;
    }

    // Set global enabled port mask variable for use elsewhere and return 0 for success.
    enabled_port_mask &= ppcm;

    // Return for success!
    return ret;
}

/**
 * Checks and prints the status of all running ports.
 * 
 * @return Void
**/
void dpdkc_check_link_status()
{
    // Initialize variables.
    __u16 port_id;
    __u8 count, all_ports_up, print_flag = 0;
    struct rte_eth_link link;
    int ret;
    char link_status_text[RTE_ETH_LINK_MAX_STR_LEN];

    // Verbose output.
    fprintf(stdout, "Checking link status...\n");
    fflush(stdout);
    
    // Create loop for checking links and can only occur up to MAX_CHECK_TIME.
    for (count = 0; count <= MAX_CHECK_TIME; count++)
    {
        // If the program is supposed to exit, return.
        if (quit)
        {
            return;
        }

        // Assume all ports are up until they aren't.
        all_ports_up = 1;

        // Loop through all ports.
        RTE_ETH_FOREACH_DEV(port_id)
        {
            // Due to processing time, we want to check if the program has exited again.
            if (quit)
            {
                return;
            }

            // Make sure this port is enabled through the port mask.
            if ((enabled_port_mask & (1 << port_id)) == 0)
            {
                continue;
            }

            // Clear the link by setting it to all 0x00 bytes.
            memset(&link, 0, sizeof(link));

            // Retrieve the link with no wait support.
            ret = rte_eth_link_get_nowait(port_id, &link);

            if (ret < 0)
            {
                // Port is down, set all ports up variable to 0 and print message if we were successful before.
                all_ports_up = 0;

                if (print_flag == 1)
                {
                    fprintf(stderr, "Port %u link failed: %s.\n", port_id, rte_strerror(-ret));

                    continue;
                }
            }

            // Print the status of the link if we've scanned successfully all the way through.
            if (print_flag == 1)
            {
                rte_eth_link_to_str(link_status_text, sizeof(link_status_text), &link);
                fprintf(stdout, "Port %d => %s.\n", port_id, link_status_text);

                continue;
            }

            // If the link's status is down itself, set all ports up as 0 and break the loop.
            if (link.link_status == RTE_ETH_LINK_DOWN)
            {
                all_ports_up = 0;

                break;
            }
        }

        // If print flag is one (for debug, no actions), break.
        if (print_flag == 1)
        {
            break;
        }

        // If all ports up is 0 (at least on port down), flush stdout and wait CHECK_INTERVAL milliseconds.
        if (all_ports_up == 0)
        {
            fprintf(stdout, ".");
            fflush(stdout);

            rte_delay_ms(CHECK_INTERVAL);
        }

        // If we were completely successful if we exceed the max check count, set print_flag to 1 for debug only.
        if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1))
        {
            print_flag = 1;
        }
    }
}

/**
 * Initializes the DPDK application's EAL.
 * 
 * @param argc The argument count.
 * @param argv Pointer to arguments array.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_eal_init(int argc, char **argv)
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    int tmp = rte_eal_init(argc, argv);

    if (tmp < 0)
    {
        ret.err_num = -1;
        ret.gen_msg = "Failed to initialize EAL.";
    }
    else
    {
        ret.data = (void *)&tmp;
    }

    return ret;
}

/**
 * Retrieves the amount of ports available.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret). Number of available ports are stored inside of ret->data.
**/
struct dpdkc_ret dpdkc_get_nb_ports()
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    unsigned short num = rte_eth_dev_count_avail();

    ret.err_num = (num > 0) ? 0 : -1;

    if (ret.err_num != 0)
    {
        ret.gen_msg = "No available ports.";
    }

    ret.data = (void *)&num;

    return ret;
}

/**
 * Checks all port pairs.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_check_port_pairs()
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    // If port params is NULL, ignore and return success.
    if (port_pair_params != NULL)
    {
        ret.err_num = dpdkc_check_port_pair_config().err_num;
    }

    if (ret.err_num != 0)
    {
        ret.gen_msg = "Failed to check port pair config.";
    }

    return ret;
}

/**
 * Checks all ports against port mask.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_ports_are_valid()
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();
    
    ret.err_num = !(enabled_port_mask & ~((1 << nb_ports) - 1));

    if (ret.err_num != 0)
    {
        ret.gen_msg = "Number of ports failed against enabled port mask.";
    }

    return ret;
}

/**
 * Resets all destination ports.
 * 
 * @return Void
**/
void dpdkc_reset_dst_ports()
{
    // Loop through all ports and set them to 0.
    for (port_id = 0; port_id < RTE_MAX_ETHPORTS; port_id++)
    {
        dst_ports[port_id] = 0;
    }
}

/**
 * Populates all destination ports.
 * 
 * @return Void
**/
void dpdkc_populate_dst_ports()
{
    // Number of ports in a mask.
    unsigned nb_ports_in_mask = 0;

    // The last port used.
    __u16 last_port = 0;

    // Check port pair params first.
    if (port_pair_params != NULL)
    {
        // Specify index and p.
        __u16 index;
        __u16 p;

        // Loop through all port pair params.
        for (index = 0; index < (nb_port_pair_params << 1); index++)
        {
            // Set port ID and dst port.
            p = index & 1;
            port_id = port_pair_params[index >> 1].port[p];
            dst_ports[port_id] = port_pair_params[index >> 1].port[p ^ 1];
        }
    }
    else
    {
        // Loop through all ports.
        RTE_ETH_FOREACH_DEV(port_id)
        {
            // Check if port is 
            if (!dpdkc_port_enabled())
            {
                continue;
            }

            // Get remainder and assign dst ports.
            if (nb_ports_in_mask % 2)
            {
                dst_ports[port_id] = last_port;
                dst_ports[last_port] = port_id;
            }
            else
            {
                last_port = port_id;
            }

            // Increase ports count.
            nb_ports_in_mask++;
        }

        // If we have an odd number of ports in the port mask, output a warning.
        if (nb_ports_in_mask % 2)
        {
            fprintf(stdout, "WARNING - Odd number of ports in port mask.\n");
            dst_ports[last_port] = last_port;
        }
    }
}

/**
 * Maps ports and queues to each l-core.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_ports_queues_mapping()
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    // Pointer we'll be storing individual l-core configs in.
    struct lcore_queue_conf *qconf = NULL;

    // RX l-core ID.
    unsigned int rx_lcore_id = 0;

    RTE_ETH_FOREACH_DEV(port_id)
    {
        // Skip any ports not available.
        if (!dpdkc_port_enabled())
        {
            continue;
        }

        // Retrieve the l-core ID.
        while (rte_lcore_is_enabled(rx_lcore_id) == 0 || lcore_queue_conf[rx_lcore_id].num_rx_ports == rx_queue_pl)
        {
            // Increment the lcore ID.
            rx_lcore_id++;

            // If the new ID is higher or equal to the maximum lcore count, we need to exit with an error.
            if (rx_lcore_id >= RTE_MAX_LCORE)
            {
                ret.err_num = -1;
                ret.gen_msg = "Failed due to l-core ID exceeding the maximum amount of l-cores.";

                return ret;
            }
        }

        // Assign qconf to current lcore queue config's memory location.
        if (qconf != &lcore_queue_conf[rx_lcore_id])
        {
            qconf = &lcore_queue_conf[rx_lcore_id];

            // Increase l-cores count.
            nb_lcores++;
        }

        // Assign port_id to port list at qconf->num_rx_ports index and increase RX port count.
        qconf->rx_port_list[qconf->num_rx_ports] = port_id;
        qconf->num_rx_ports++;

        // Print verbose message.
        fprintf(stdout, "Setting up l-core #%u with RX port %u and TX port %u.\n", rx_lcore_id, port_id, dst_ports[port_id]);
    }

    return ret;
}

/**
 * Creates the packet's mbuf pool.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_create_mbuf()
{
    // Initialize return variable (custom error).
    struct dpdkc_ret ret = dpdkc_ret_init();

    // Retrieve amount of mbufs to create.
    unsigned int nb_mbufs = RTE_MAX(nb_ports * (nb_rxd + nb_txd + nb_lcores & MEMPOOL_CACHE_SIZE), 8192U);

    // Create mbuf pool.
    pcktmbuf_pool = rte_pktmbuf_pool_create("pckt_pool", nb_mbufs, MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    // Check if the mbuf pool is NULL.
    if (pcktmbuf_pool == NULL)
    {
        ret.gen_msg = "Failed to create packet's mbuf pool.";
        ret.err_num = -1;
    }

    return ret;
}

/**
 * Initializes all ports and RX/TX queues.
 * 
 * @param promisc If 1, promisc mode is turned on for all ports/devices.
 * @param rx_queue The amount of RX queues per port (recommend setting to 1).
 * @param tx_queue The amount of TX queues per port (recommend setting to 1).
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_ports_queues_init(int promisc, int rx_queue, int tx_queue)
{
    // Initialize return variable (custom error).
    struct dpdkc_ret ret = dpdkc_ret_init();

    RTE_ETH_FOREACH_DEV(port_id)
    {
        // Initialize queue/port conifgs and device info.
        int i;
        
        struct rte_eth_conf local_port_conf = port_conf;
        struct rte_eth_dev_info dev_info;

        // Skip any ports not available.
        if (!dpdkc_port_enabled())
        {
            fprintf(stdout, "Skipping port #%u initialize due to it being disabled.\n", port_id);
            
            continue;
        }

        // Increment the ports available count.
        nb_ports_available++;

        // Initialize the port itself.
        fprintf(stdout, "Initializing port #%u...\n", port_id);
        fflush(stdout);

        // Attempt to receive device information for this specific port and check.
        if ((ret.err_num = rte_eth_dev_info_get(port_id, &dev_info)) != 0)
        {
            ret.port_id = port_id;
            ret.gen_msg = "Failed to retrieve device info.";

            return ret;
        }

        // Check for TX mbuf fast free support on this specific device.
        if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
        {
            local_port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
        }

        // Configure the queues for this port.
        if ((ret.err_num = rte_eth_dev_configure(port_id, rx_queue, tx_queue, &local_port_conf)) < 0)
        {
            ret.port_id = port_id;
            ret.gen_msg = "Failed to configure ethernet device with RX and TX queues.";

            return ret;
        }

        // Retrieve MAC address of device and store in array.
        if ((ret.err_num = rte_eth_macaddr_get(port_id, &ports_eth[port_id])) < 0)
        {
            ret.port_id = port_id;
            ret.gen_msg = "Failed to retrieve MAC address on port.";

            return ret;
        }

        // Initialize the RX queues.
        fflush(stdout);

        for (i = 0; i < rx_queue; i++)
        {
            // Initialize RX config and set values from port configuration.
            struct rte_eth_rxconf rxq_conf;

            rxq_conf = dev_info.default_rxconf;
            rxq_conf.offloads = local_port_conf.rxmode.offloads;

            // Setup the RX queue and check.
            if ((ret.err_num = rte_eth_rx_queue_setup(port_id, i, nb_rxd, rte_eth_dev_socket_id(port_id), &rxq_conf, pcktmbuf_pool)) < 0)
            {
                ret.port_id = port_id;
                ret.rx_id = i;
                ret.gen_msg = "Failed to setup RX queue.";

                return ret;
            }
        }

        // Initialize the TX queues.
        fflush(stdout);

        for (i = 0; i < tx_queue; i++)
        {
            // Initialize TX config and sset values from port configuration.
            struct rte_eth_txconf txq_conf;

            txq_conf = dev_info.default_txconf;
            txq_conf.offloads = local_port_conf.txmode.offloads;

            // Setup the TX queue and check.
            if ((ret.err_num = rte_eth_tx_queue_setup(port_id, i, nb_txd, rte_eth_dev_socket_id(port_id), &txq_conf)) < 0)
            {
                ret.port_id = port_id;
                ret.tx_id = i;
                ret.gen_msg = "Failed to setup TX queue.";

                return ret;
            }
        }

        // Initialize TX buffers.
        tx_buffer[port_id] = rte_zmalloc_socket("tx_buffer", RTE_ETH_TX_BUFFER_SIZE(MAX_PCKT_BURST), 0, rte_eth_dev_socket_id(port_id));

        // Check if the TX buffer allocation was successful.
        if (tx_buffer[port_id] == NULL)
        {
            ret.err_num = -1;
            ret.port_id = port_id;
            ret.gen_msg = "Failed to allocate TX buffer.";

            return ret;
        }

        // Initialize the buffer itself within TX and check its result.
        rte_eth_tx_buffer_init(tx_buffer[port_id], MAX_PCKT_BURST);

        if ((ret.err_num = rte_eth_tx_buffer_set_err_callback(tx_buffer[port_id], rte_eth_tx_buffer_count_callback, NULL)) < 0)
        {
            ret.port_id = port_id;
            ret.gen_msg = "Failed to setup TX buffer callback.";

            return ret;
        }

        // We'll want to disable PType parsing.
        if ((ret.err_num = rte_eth_dev_set_ptypes(port_id, RTE_PTYPE_UNKNOWN, NULL, 0)) < 0)
        {
            ret.port_id = port_id;
            ret.gen_msg = "Failed to disable PType parsing for performance.";

            return ret;
        }

        // Start the device itself.
        if ((ret.err_num = rte_eth_dev_start(port_id)) < 0)
        {
            ret.port_id = port_id;
            ret.gen_msg = "Failed to start device.";

            return ret;
        }

        // Check for promiscuous mode.
        if (promisc)
        {
            // If we aren't able to enable promiscuous mode, error out.
            if ((ret.err_num = rte_eth_promiscuous_enable(port_id)) < 0)
            {
                ret.port_id = port_id;
                ret.gen_msg = "Failed to enable promiscuous mode on port.";

                return ret;
            }
        }

        // Set verbose message.
        fprintf(stdout, "Port #%u setup successfully. MAC Address => " RTE_ETHER_ADDR_PRT_FMT ".\n", port_id, RTE_ETHER_ADDR_BYTES(&ports_eth[port_id]));
    }

    // We're done!
    return ret;
}

/**
 * Check if the number of available ports is above one.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret). The amount of available ports is returned in ret->data.
**/
struct dpdkc_ret dpdkc_ports_available()
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    ret.err_num = nb_ports_available > 0;
    ret.data = (void *)&nb_ports_available;

    return ret;
}

/**
 * Launches the DPDK application and waits for all l-cores to exit.
 * 
 * @param f A pointer to the function to launch on all l-cores when ran.
 * 
 * @return Void
**/
void dpdkc_launch_and_run(void *f)
{
    // Launch the application on each l-core.
    rte_eal_mp_remote_launch((int (*)(void *))f, NULL, CALL_MAIN);

    RTE_LCORE_FOREACH_WORKER(lcore_id)
    {
        if (rte_eal_wait_lcore(lcore_id) < 0)
        {
            break;
        }
    }
}

/**
 * Stops and removes all running ports.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_port_stop_and_remove()
{
    // Create DPDK Common's return structure.
    struct dpdkc_ret ret = dpdkc_ret_init();

    RTE_ETH_FOREACH_DEV(port_id)
    {
        // Skip disabled ports.
        if (!dpdkc_port_enabled())
        {
            continue;
        }

        fprintf(stdout, "Closing port #%u.\n", port_id);

        // Stop the port and check.
        if ((ret.err_num = rte_eth_dev_stop(port_id)) != 0)
        {
            return ret;
        }

        // Finally, close the port.
        rte_eth_dev_close(port_id);
    }

    return ret;
}

/**
 * Cleans up the DPDK application's EAL.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_eal_cleanup()
{
    struct dpdkc_ret ret = dpdkc_ret_init();

    ret.err_num = rte_eal_cleanup();

    return ret;
}

/**
 * Checks error from dpdkc_ret structure and prints error along with exits if found.
 * 
 * @return Void
**/
void dpdkc_check_error(struct dpdkc_ret *ret)
{
    if (ret->err_num != 0)
    {
        char msg[256];
        snprintf(msg, sizeof(msg) - 1, "%s Port ID => %d. RX queue ID => %d. TX queue ID => %d. Error => %s (%d).\n", (ret->gen_msg != NULL) ? ret->gen_msg : "N/A", ret->port_id, ret->rx_id, ret->tx_id, strerror(-ret->err_num), ret->err_num);

        rte_exit(EXIT_FAILURE, "%s", msg);
    }
}
