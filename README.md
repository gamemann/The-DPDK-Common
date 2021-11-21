# The DPDK Common (WIP)
## Description
This project includes helpful functions and global variables for developing applications using [the DPDK](https://www.dpdk.org/). I am using this for my projects using the DPDK.

A majority of this code comes from the `l2fwd` [example](https://github.com/DPDK/dpdk/blob/main/examples/l2fwd) from the DPDK's source [files](https://github.com/DPDK/dpdk/), but I rewrote all of the code to learn more from it and I tried adding as many comments as I could explaining what I understand from the code. I also highly organized the code and removed a lot of things I thought were unnecessary in developing my applications.

I want to make clear that I am still new to the DPDK. While the helper functions and global variables in this project don't allow for in-depth configuration of the DPDK application, it is useful for general setups such as making packet generator programs or wanting to make a fast packet processing library where you're inspecting and manipulating packets.

My main goal is to help other developers with the DPDK along with myself. From what I've experienced, learning the DPDK can be very overwhelming due to the amount of complexity it has. I mean, have you seen their programming documentation/guides [here](http://doc.dpdk.org/guides/prog_guide/)?! I'm just hoping to help other developers learn the DPDK. As time goes on and I learn more about the DPDK, I will add onto this project!

## My Other Projects Using DPDK Common
I have other projects in the pipeline that'll use DPDK Common once I implement a few other things. However, here is the current list.

* [Examples/Tests](https://github.com/gamemann/The-DPDK-Examples) - A repository I'm using to store examples and tests of the DPDK while I learn it.

## Functions
Including the `src/dpdk_common.h` header in a source or another header file will additionally include general header files from the DPDK. With that said, it will allow you to use the following functions which are a part of the DPDK Common project.

```C
/**
 * Parses the port mask argument.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return Returns the port mask.
 */
unsigned long dpdkc_parse_arg_port_mask(const char *arg);

/**
 * Parses the port pair config.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return 0 on success and -1 on error.
*/
int dpdkc_parse_arg_port_pair_config(const char *arg);

/**
 * Parses the queue number argument.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return Returns the amount of queues.
*/
unsigned int dpdkc_parse_arg_queues(const char *arg);

/**
 * Checks the port pair config after initialization.
 * 
 * @return 0 on success or -1 on error.
*/
int dpdkc_check_port_pair_config(void);

/**
 * Checks and prints the status of all running ports.
 * 
 * @param port_mask The enabled port mask.
 * 
 * @return Void
*/
void dpdkc_check_link_status(__u32 port_mask);
```

## Global Variables
Additionally, there are useful global variables directed towards aspects of the program for the DPDK. However, these are prefixed with the `extern` tag within the `src/dpdk_common.h` header file allowing you to use them anywhere else assuming the file is included and the object file built from `make` is linked.

```C
// Variable to use for signals (set to 1 to force application to exit).
volatile __u8 quit;

// The RX and TX descriptor count (using defaults).
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
```

## Credits
* [Christian Deacon](https://github.com/gamemann)