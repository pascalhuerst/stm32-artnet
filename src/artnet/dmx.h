#ifndef ARTNET_DMX_H
#define ARTNET_DMX_H

struct artnet_dmx_handle
{
    artnet_node node;
    struct k_sem quit_lock;
    uint8_t *data;
    int length;
} __attribute__((__packed__));


struct artnet_dmx_handle *
make_dmx_receiver(struct net_if *net, int subnet, const char *short_name, const char *long_name);
void start(struct artnet_dmx_handle *h);
void stop(struct artnet_dmx_handle *h);

void print_ip2(void *h);
void print_ip(struct artnet_dmx_handle *h);



#endif //#ifndef ARTNET_DMX_H