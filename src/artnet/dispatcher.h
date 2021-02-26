#ifndef ARTNET_DISPATCHER_H
#define ARTNET_DISPATCHER_H


#include "artnet.h"
#include "fixup.h"

void print_node_config(artnet_node_entry ne);
int reply_handler(artnet_node n, void *pp, void *d);
void do_artnet(struct net_if *iface);

#endif // ARTNET_DISPATCHER_H