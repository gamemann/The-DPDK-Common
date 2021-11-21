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

/**
 * Parses the port mask argument.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return Returns the port mask.
 */
unsigned long dpdkc_parse_arg_port_mask(const char *arg)
{
    char *end = NULL;
    unsigned long pm;

    // Parse hexadecimal.
    pm = strtoul(arg, &end, 16);

    return pm;
}

/**
 * Parses the port pair config.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return 0 on success and -1 on error.
*/
int dpdkc_parse_arg_port_pair_config(const char *arg)
{
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
            return -1;
        }

        // Get the size of the pair values.
        size = p0 - p;

        // It should not be even close to 256 bytes.
        if (size >= sizeof(s))
        {
            return -1;
        }

        // Copy the port pair data to 's' char array.
        memcpy(s, p, size);
        
        // Last byte should be 0x00.
        s[size] = '\0';

        // Split string by ',' which divides both integers for port pair.
        if (rte_strsplit(s, sizeof(s), str_fld, _NUM_FLD, ',') != _NUM_FLD)
        {
            return -1;
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
                return -1;
            }
        }

        // Make sure we don't exceed max number of port pairs.
        if (nb_port_pair_params >= RTE_MAX_ETHPORTS / 2)
        {
            fprintf(stderr, "Exceeded maximum number of port pair params: %hu.\n", nb_port_pair_params);

            return -1;
        }

        // Assign port pair values (x, y) to port pair params array and increment number of port pairs.
        port_pair_params_array[nb_port_pair_params].port[0] = (__u16)int_fld[FLD_PORT1];
        port_pair_params_array[nb_port_pair_params].port[1] = (__u16)int_fld[FLD_PORT2];

        nb_port_pair_params++;
    }

    // Assign port pair array to port pair params.
    port_pair_params = port_pair_params_array;

    return 0;
}

/**
 * Parses the queue number argument.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return Returns the amount of queues.
*/
unsigned int dpdkc_parse_arg_queues(const char *arg)
{
    // Initialize a couple necessary variables.
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

/**
 * Checks the port pair config after initialization.
 * 
 * @return 0 on success or -1 on error.
*/
int dpdkc_check_port_pair_config(void)
{
    // Port pair config mask and port pair mask.
    __u32 ppcm;
    __u32 ppm;

    // Other variables for iteration.
    __u16 index, i, port_id;

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
                fprintf(stderr, "Port #%u is not enabled through port mask.\n", port_id);

                return -1;
            }

            // Check if the port is valid.
            if (!rte_eth_dev_is_valid_port(port_id))
            {
                fprintf(stderr, "Port %u is not valid.\n", port_id);

                return -1;
            }

            // Retrieve port pair mask.
            ppm |= 1 << port_id;
        }

        // Check if this port is being used in another pair since we OR the current PPM to the PPCM variable.
        if (ppcm & ppm)
        {
            fprintf(stderr, "Port %u is used in other port pairs.\n", port_id);

            return -1;
        }

        // OR PPM to PPCM so we can perform check above to ensure this port ID isn't in use with another port pair.
        ppcm |= ppm;
    }

    // Set global enabled port mask variable for use elsewhere and return 0 for success.
    enabled_port_mask &= ppcm;

    return 0;
}

/**
 * Checks and prints the status of all running ports.
 * 
 * @param port_mask The enabled port mask.
 * 
 * @return Void
*/
void dpdkc_check_link_status(__u32 port_mask)
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
            if ((port_mask & (1 << port_id)) == 0)
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