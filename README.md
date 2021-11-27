# The DPDK Common
## Description
This project includes helpful functions and global variables for developing applications using [the DPDK](https://www.dpdk.org/). I am using this for my projects using the DPDK.

A majority of this code comes from the `l2fwd` [example](https://github.com/DPDK/dpdk/blob/main/examples/l2fwd) from the DPDK's source [files](https://github.com/DPDK/dpdk/), but I rewrote all of the code to learn more from it and I tried adding as many comments as I could explaining what I understand from the code. I also highly organized the code and removed a lot of things I thought were unnecessary in developing my applications.

I want to make clear that I am still new to the DPDK. While the helper functions and global variables in this project don't allow for in-depth configuration of the DPDK application, it is useful for general setups such as making packet generator programs or wanting to make a fast packet processing library where you're inspecting and manipulating packets.

My main goal is to help other developers with the DPDK along with myself. From what I've experienced, learning the DPDK can be very overwhelming due to the amount of complexity it has. I mean, have you seen their programming documentation/guides [here](http://doc.dpdk.org/guides/prog_guide/)?! I'm just hoping to help other developers learn the DPDK. As time goes on and I learn more about the DPDK, I will add onto this project!

## My Other Projects Using DPDK Common
I have other projects in the pipeline that'll use DPDK Common once I implement a few other things. However, here is the current list.

* [Examples/Tests](https://github.com/gamemann/The-DPDK-Examples) - A repository I'm using to store examples and tests of the DPDK while I learn it.

## The Custom Return Structure
This project uses a custom return structure for functions returning values (non-void). The name of the structure is `dpdkc_ret`.

```C
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
```

With that said, the function `dpdkc_check_ret(struct dpdkc_ret *ret)` checks for an error in the structure and exits the application with debugging information if there is an error found (`!= 0`).

Any data from the functions returning this structure should be stored in the `data` pointer. You will need to cast when using this data in the application since it is of type `void *`.

## Functions
Including the `src/dpdk_common.h` header in a source or another header file will additionally include general header files from the DPDK. With that said, it will allow you to use the following functions which are a part of the DPDK Common project.

```C
/**
 * Initializes a DPDK Common result type and returns it with default values.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret) with its default values.
**/
struct dpdkc_ret dpdkc_ret_init();

/**
 * Parses the port mask argument.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret). The port mask is stored in ret->data.
**/
struct dpdkc_ret dpdkc_parse_arg_port_mask(const char *arg);

/**
 * Parses the port pair config argument.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_parse_arg_port_pair_config(const char *arg);

/**
 * Parses the queue number argument.
 * 
 * @param arg A (const) pointer to the optarg variable from getopt.h.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret). The amount of queues is stored in ret->data.
**/
struct dpdkc_ret dpdkc_parse_arg_queues(const char *arg);

/**
 * Checks the port pair config after initialization.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_check_port_pair_config(void);

/**
 * Checks and prints the status of all running ports.
 * 
 * @return Void
**/
void dpdkc_check_link_status();

/**
 * Initializes the DPDK application's EAL.
 * 
 * @param argc The argument count.
 * @param argv Pointer to arguments array.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_eal_init(int argc, char **argv);

/**
 * Retrieves the amount of ports available.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret). Number of available ports are stored inside of ret->data.
**/
struct dpdkc_ret dpdkc_get_nb_ports();

/**
 * Checks all port pairs.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_check_port_pairs();

/**
 * Checks all ports against port mask.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_ports_are_valid();

/**
 * Resets all destination ports.
 * 
 * @return Void
**/
void dpdkc_reset_dst_ports();

/**
 * Populates all destination ports.
 * 
 * @return Void
**/
void dpdkc_populate_dst_ports();

/**
 * Maps ports and queues to each l-core.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_ports_queues_mapping();

/**
 * Creates the packet's mbuf pool.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_create_mbuf();

/**
 * Initializes all ports and RX/TX queues.
 * 
 * @param promisc If 1, promisc mode is turned on for all ports/devices.
 * @param rx_queue The amount of RX queues per port (recommend setting to 1).
 * @param tx_queue The amount of TX queues per port (recommend setting to 1).
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_ports_queues_init(int promisc, int rx_queue, int tx_queue);

/**
 * Check if the number of available ports is above one.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret). The amount of available ports is returned in ret->data.
**/
struct dpdkc_ret dpdkc_ports_available();

/**
 * Launches the DPDK application and waits for all l-cores to exit.
 * 
 * @param f A pointer to the function to launch on all l-cores when ran.
 * 
 * @return Void
**/
void dpdkc_launch_and_run(void *f);

/**
 * Stops and removes all running ports.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_port_stop_and_remove();

/**
 * Cleans up the DPDK application's EAL.
 * 
 * @return The DPDK Common return structure (struct dpdkc_ret).
**/
struct dpdkc_ret dpdkc_eal_cleanup();

/**
 * Checks error from dpdkc_ret structure and prints error along with exits if found.
 * 
 * @return Void
**/
void dpdkc_check_ret(struct dpdkc_ret *ret);
```

The following function(s) are available if `USE_HASH_TABLES` is defined.

```C
/**
 * Removes the least recently used item from a regular hash table if the table exceeds max entries.
 * 
 * @param tbl A pointer to the hash table.
 * @param max_entries The max entries in the table.
 * 
 * @return 0 on success or -1 on error (failed to delete key from table).
**/
int check_and_del_lru_from_hash_table(void *tbl, __u64 max_entries);
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
        .split_hdr_size = 1
    },
    .rxmode =
    {
        .mq_mode = RTE_ETH_MQ_TX_NONE
    }
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
```

## Credits
* [Christian Deacon](https://github.com/gamemann)