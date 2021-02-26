#include <logging/log.h>
LOG_MODULE_REGISTER(net_dhcpv4_client_sample, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <linker/sections.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <device.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include "artnet/artnet.h"
#include "artnet/fixup.h"

struct k_sem dhcp_done_sem;
char ip_address[NET_IPV4_ADDR_LEN];
int nodes_found = 0;
int verbose = 1;

static struct net_mgmt_event_callback mgmt_cb;

static void handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
	int i = 0;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD)
	{
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++)
	{
		char buf[NET_IPV4_ADDR_LEN];

		if (iface->config.ip.ipv4->unicast[i].addr_type != NET_ADDR_DHCP)
		{
			continue;
		}

		LOG_INF("Your address: %s", log_strdup(net_addr_ntop(AF_INET, &iface->config.ip.ipv4->unicast[i].address.in_addr, ip_address, sizeof(ip_address))));
		LOG_INF("Lease time: %u seconds", iface->config.dhcpv4.lease_time);
		LOG_INF("Subnet: %s", log_strdup(net_addr_ntop(AF_INET, &iface->config.ip.ipv4->netmask, buf, sizeof(buf))));
		LOG_INF("Router: %s", log_strdup(net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw, buf, sizeof(buf))));
		LOG_INF("GEVING SEMAPHORE!! in 3s");
		k_sleep(Z_TIMEOUT_MS(3000));
		k_sem_give(&dhcp_done_sem);
		LOG_INF("GAVE SEMAPHORE!!!");
	}
}

void print_node_config(artnet_node_entry ne)
{
	printf("--------- %d.%d.%d.%d -------------\n", ne->ip[0], ne->ip[1], ne->ip[2], ne->ip[3]);
	printf("Short Name:   %s\n", ne->shortname);
	printf("Long Name:    %s\n", ne->longname);
	printf("Node Report:  %s\n", ne->nodereport);
	printf("Subnet:       0x%02x\n", ne->sub);
	printf("Numb Ports:   %d\n", ne->numbports);
	printf("Input Addrs:  0x%02x, 0x%02x, 0x%02x, 0x%02x\n", ne->swin[0], ne->swin[1], ne->swin[2], ne->swin[3]);
	printf("Output Addrs: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", ne->swout[0], ne->swout[1], ne->swout[2], ne->swout[3]);
	printf("----------------------------------\n");
}

int reply_handler(artnet_node n, void *pp, void *d)
{
	artnet_node_list nl = artnet_get_nl(n);
	artnet_node_entry entry = artnet_nl_first(nl);

	if (entry)
	{
		do
		{
			print_node_config(entry);
		} while ((entry = artnet_nl_next(nl)) != NULL);
	}

	return 0;
}

void main(void)
{
	artnet_node node;
	struct net_if *iface;

	int ret, sd, maxsd, timeout = 20000;
	fd_set rset;
	struct timeval tv;
	time_t start;

	k_sem_init(&dhcp_done_sem, 0, 1);

	LOG_INF("Run dhcpv4 client");
	net_mgmt_init_event_callback(&mgmt_cb, handler, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);
	iface = net_if_get_default();
	net_dhcpv4_start(iface);

	LOG_INF("#### Waiting for sem");
	if ((ret = k_sem_take(&dhcp_done_sem, K_FOREVER)) != 0)
	{
		LOG_INF("Cannot take dhcp done semaphore: ret=%d", ret);
	}
	else
	{

		// Wait for DHCP to finish
		LOG_INF("#### Got sem! Now we have an ip: %s, let's try some artnet fun....", log_strdup(ip_address));

		k_sleep(Z_TIMEOUT_MS(3000));

		if ((node = artnet_new(iface, verbose)) == NULL)
		{
			LOG_INF("Error: artnet_new");
		}

		artnet_set_short_name(node, "da-artnet-discovery");
		artnet_set_long_name(node, "DA ArtNet Discovery Node");
		artnet_set_node_type(node, ARTNET_SRV);

		// set poll reply handler
		artnet_set_handler(node, ARTNET_REPLY_HANDLER, reply_handler, NULL);

		if (artnet_start(node) != ARTNET_EOK)
		{
			LOG_INF("Error: artnet_start");
			while (1)
				;
		}

		LOG_INF("Loop start!");

		sd = artnet_get_sd(node);
		if (artnet_send_poll(node, NULL, ARTNET_TTM_DEFAULT) != ARTNET_EOK)
		{
			LOG_ERR("send poll failed");
			while (1)
				;
		}

		start = time(NULL);
		// wait for timeout seconds before quitting
		while (time(NULL) - start < timeout)
		{
			LOG_INF("Loop...");

			FD_ZERO(&rset);
			FD_SET(sd, &rset);

			tv.tv_usec = 0;
			tv.tv_sec = 1;
			maxsd = sd;

			switch (select(maxsd + 1, &rset, NULL, NULL, &tv))
			{
			case 0:
				LOG_INF("TIMEOUT");
				break;
			case -1:
				printf("select error\n");
				break;
			default:
				artnet_read(node, 0);
			}
		}
	}

	LOG_INF("REACHED END!!!W!");
}
