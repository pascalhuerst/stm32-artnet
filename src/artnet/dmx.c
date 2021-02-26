#include <logging/log.h>
LOG_MODULE_REGISTER(artnet_dmx, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <net/net_if.h>
#include <stdlib.h>
#include <stdio.h>
#include "artnet.h"
#include "fixup.h"
#include "dmx.h"
#include "private.h"

int dmx_handler(artnet_node n, int port, void *d);

struct artnet_dmx_handle *
make_dmx_receiver(struct net_if *net, int subnet, const char *short_name, const char *long_name)
{
    struct artnet_dmx_handle *ret = malloc(sizeof(struct artnet_dmx_handle));
    
    printf("ret: %p\n", (void*)ret->node);

    artnet_node tmp;
    
    if ((tmp = artnet_new(net, true)) == NULL)
    {
        LOG_ERR("Error: Cannot create artnet node");
        free(ret);
        return NULL;
    }

    printf("00: %p\n", (void*)tmp);
    
    
    ret->node = tmp;
    
    
    printf("01: %p\n", (void*)ret->node);
    printf("02: %p\n", (void*)(*ret).node);

    int result = 0;
    if ((result = artnet_set_short_name(ret->node, short_name)) != ARTNET_EOK) {
        LOG_ERR("Error: Cannot set short name: %d", result);
        free(ret);
        return NULL;
    }
    
    if ((result = artnet_set_long_name(ret->node, long_name)) != ARTNET_EOK) {
        LOG_ERR("Error: Cannot set long name: %d", result);
        free(ret);
        return NULL;
    }
    
    if ((result = artnet_set_dmx_handler(ret->node, dmx_handler, &ret)) != ARTNET_EOK) {
        LOG_ERR("Error: Cannot set dmx handler %d", result);
        free(ret);
        return NULL;
    }

    if ((result = artnet_set_subnet_addr(ret->node, subnet)) != ARTNET_EOK) {
        LOG_ERR("Error: Cannot set subnet address: %d", result);
        free(ret);
        return NULL;
    }

    k_sem_init(&ret->quit_lock, 0, UINT_MAX); // make no sense so far....
    
    return ret;
}

void print_ip(struct artnet_dmx_handle *h) {

    printf("PPP1: %p\n", (void*)h->node);

    struct artnet_node_s *n = h->node;
    char buf[ARTNET_IP_SIZE];
    printf("AAAA %s\n", log_strdup(net_addr_ntop(AF_INET, &n->state.ip_addr, buf, sizeof(buf))));
}

void print_ip2(void *h) {

    printf("PPP2: %p\n", h);

    struct artnet_node_s *n = (struct artnet_node_s *) &(((struct artnet_dmx_handle *)h)->node);
    char buf[ARTNET_IP_SIZE];
    printf("BBBB %s\n", log_strdup(net_addr_ntop(AF_INET, &n->state.ip_addr, buf, sizeof(buf))));
}

void start(struct artnet_dmx_handle *h)
{
    //k_sem_take(&h->quit_lock, K_FOREVER);
    
   
    LOG_INF("Passed semaphore in artnet dmx");
    int result = 0;

    if ((result = artnet_start((void *)h->node)) != ARTNET_EOK) {
        LOG_ERR("Error: Cannot start artnet: %d", result);
    }
}

void stop(struct artnet_dmx_handle *h)
{
    //k_sem_give(&h->quit_lock);

    int result = 0;
    if ((result = artnet_stop(h->node)) != ARTNET_EOK) {
        LOG_ERR("Error: Cannot stop artnet: %d", result);
    }
}

int dmx_handler(artnet_node n, int port, void *d) {
	
    struct artnet_dmx_handle *h = (struct artnet_dmx_handle *)d;

	uint8_t *data = artnet_read_dmx(n, port, &h->length);
    memcpy(h->data, data, h->length);
    
    LOG_INF("Packet: port=%d len=%d", port, h->length);
    LOG_HEXDUMP_DBG(h->data, h->length, "");

	return 0;
}