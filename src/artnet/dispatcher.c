#include <logging/log.h>
LOG_MODULE_REGISTER(dispatcher, LOG_LEVEL_DBG);


#include <zephyr.h>
#include <stdio.h>

#include "dispatcher.h"

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

void do_artnet(struct net_if *iface)
{

    artnet_node node;
    int verbose = 1;

    int sd, maxsd, timeout = 20000;
    fd_set rset;
    struct timeval tv;
    time_t start;

    // Wait for DHCP to finish
    //LOG_INF("#### Got sem! Now we have an ip: %s, let's try some artnet fun....", log_strdup(ip_address_string));

    k_sleep(Z_TIMEOUT_MS(3000));

    if ((node = artnet_new(iface, verbose)) == NULL)
    {
        LOG_INF("Error: artnet_new");
    }

    artnet_set_short_name(node, "da-artnet-discovery");
    artnet_set_long_name(node, "DA ArtNet Discovery Node");
    artnet_set_node_type(node, ARTNET_SRV);

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