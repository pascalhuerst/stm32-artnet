#include <logging/log.h>
LOG_MODULE_REGISTER(da_artnet_node, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include <stdio.h>

#include "artnet/dispatcher.h"
#include "artnet/dmx.h"

static struct net_mgmt_event_callback iface_callback;
static struct net_mgmt_event_callback ipv4_callback;
static uint8_t mac_addr[] = {0x02, 0x80, 0xE1, 0x45, 0x14, 0x7B};
static struct k_sem network_ready;

static void iface_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
	LOG_INF("##### iface event");

	if (mgmt_event == NET_EVENT_IF_DOWN)
	{
		LOG_INF("### NET_EVENT_IF_DOWN ###");
	}
	else if (mgmt_event == NET_EVENT_IF_UP)
	{
		LOG_INF("### NET_EVENT_IF_UP ###");
	}

}


static void ipv4_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
	LOG_INF("##### ipv4 event");

	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD)
	{
		int i = 0;
		for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++)
		{

			if (iface->config.ip.ipv4->unicast[i].addr_type == NET_ADDR_DHCP)
			{

				char buf[NET_IPV4_ADDR_LEN];
				LOG_INF("Your address: %s", log_strdup(net_addr_ntop(AF_INET, &iface->config.ip.ipv4->unicast[i].address.in_addr, buf, sizeof(buf))));
				LOG_INF("Lease time: %u seconds", iface->config.dhcpv4.lease_time);
				LOG_INF("Subnet: %s", log_strdup(net_addr_ntop(AF_INET, &iface->config.ip.ipv4->netmask, buf, sizeof(buf))));
				LOG_INF("Router: %s", log_strdup(net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw, buf, sizeof(buf))));

				//k_sleep(K_SECONDS(3));
				k_sem_give(&network_ready);

				// Start ArtnetNode!
				// Start Display !
				// Start UART handlers !
				
				//do_artnet(iface);
			}
		}
	}
	else if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL)
	{
		LOG_INF("### NET_EVENT_IPV4_ADDR_DEL ###");
	}
}

void main(void)
{
	k_sem_init(&network_ready, 0, 1);
	
	LOG_INF("##### Starting Main ####");
	struct net_if *net = net_if_get_default();

	//int ret = net_linkaddr_set(&net->if_dev->link_addr, mac_addr, sizeof(mac_addr));
	//LOG_INF("##### net_linkaddr_set: ret=%d", ret);


	net_mgmt_init_event_callback(&iface_callback, iface_event_handler, NET_EVENT_IF_DOWN | NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&iface_callback);

	net_mgmt_init_event_callback(&ipv4_callback, ipv4_event_handler, NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&ipv4_callback);
	
	net_dhcpv4_start(net);
	
	LOG_INF("#### Waiting for network");
	k_sem_take(&network_ready, K_FOREVER);
	LOG_INF("Continuing....");

	struct artnet_dmx_handle *h = make_dmx_receiver(net, 0, "dmx receiver", "Domestic Affairs Artnet Node");
	if (h == NULL) {
		LOG_ERR("Cannot create artnet dmx receiver!");
	}

	//printf("A0: %p\n", (void*)h->node);
	//printf("A1: %p\n", (void*)(*h).node);




	LOG_INF("Starting DMX Receiver");
	start(h);
	LOG_INF("Done Starting DMX Receiver");

}
