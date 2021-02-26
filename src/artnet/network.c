/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * network.c
 * Network code for libartnet
 * Copyright (C) 2004-2007 Simon Newton
 *
 */

#include <errno.h>

#include "private.h"
#include "common.h"

#include <posix/sys/socket.h>
#include <posix/netinet/in.h>
#include <posix/arpa/inet.h>


enum { INITIAL_IFACE_COUNT = 10 };
enum { IFACE_COUNT_INC = 5 };
enum { IFNAME_SIZE = 32 }; // 32 sounds a reasonable size

typedef struct iface_s {
  struct sockaddr_in ip_addr;
  struct sockaddr_in bcast_addr;
  int8_t hw_addr[ARTNET_MAC_SIZE];
  char   if_name[IFNAME_SIZE];
  struct iface_s *next;
} iface_t;

unsigned long LOOPBACK_IP = 0x7F000001;

/*
 * Start listening on the socket
 */
int artnet_net_start(node n) {
  artnet_socket_t sock;
  struct sockaddr_in servAddr;
  node tmp;

  // only attempt to bind if we are the group master
  if (n->peering.master == TRUE) {

    // create socket
    sock = socket(PF_INET, SOCK_DGRAM, 0);

    if (sock == INVALID_SOCKET) {
      artnet_error("Could not create socket %s", artnet_net_last_error());
      return ARTNET_ENET;
    }

    memset(&servAddr, 0x00, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(ARTNET_PORT);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    char buf[32];

    if (n->state.verbose)
      printf("Binding to %s \n", inet_ntop(AF_INET, &servAddr.sin_addr, buf, sizeof(buf)));

    if (n->state.verbose)
      printf("Binding to %s \n", inet_ntop(AF_INET, &servAddr.sin_addr, buf, sizeof(buf)));

    // bind sockets
    if (bind(sock, (SA *) &servAddr, sizeof(servAddr)) == -1) {
      artnet_error("Failed to bind to socket %s", artnet_net_last_error());
      artnet_net_close(sock);
      return ARTNET_ENET;
    }

    n->sd = sock;
    // Propagate the socket to all our peers
    for (tmp = n->peering.peer; tmp && tmp != n; tmp = tmp->peering.peer)
      tmp->sd = sock;
  }
  return ARTNET_EOK;
}


/*
 * Receive a packet.
 */
int artnet_net_recv(node n, artnet_packet p, int delay) {
  ssize_t len;
  struct sockaddr_in cliAddr;
  socklen_t cliLen = sizeof(cliAddr);
  fd_set rset;
  struct timeval tv;
  int maxfdp1 = n->sd + 1;

  FD_ZERO(&rset);
  FD_SET((unsigned int) n->sd, &rset);

  tv.tv_usec = 0;
  tv.tv_sec = delay;
  p->length = 0;

  switch (select(maxfdp1, &rset, NULL, NULL, &tv)) {
    case 0:
      // timeout
      return RECV_NO_DATA;
      break;
    case -1:
      if ( errno != EINTR) {
        artnet_error("Select error %s", artnet_net_last_error());
        return ARTNET_ENET;
      }
      return ARTNET_EOK;
      break;
    default:
      break;
  }

  // need a check here for the amount of data read
  // should prob allow an extra byte after data, and pass the size as sizeof(Data) +1
  // then check the size read and if equal to size(data)+1 we have an error
  len = recvfrom(n->sd,
                 (char*) &(p->data), // char* for win32
                 sizeof(p->data),
                 0,
                 (SA*) &cliAddr,
                 &cliLen);
  if (len < 0) {
    artnet_error("Recvfrom error %s", artnet_net_last_error());
    return ARTNET_ENET;
  }

  if (cliAddr.sin_addr.s_addr == n->state.ip_addr.s_addr ||
      ntohl(cliAddr.sin_addr.s_addr) == LOOPBACK_IP) {
    p->length = 0;
    return ARTNET_EOK;
  }

  p->length = len;
  memcpy(&(p->from), &cliAddr.sin_addr, sizeof(struct in_addr));
  // should set to in here if we need it
  return ARTNET_EOK;
}


/*
 * Send a packet.
 */
int artnet_net_send(node n, artnet_packet p) {
  struct sockaddr_in addr;
  int ret;

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(ARTNET_PORT);
  addr.sin_addr = p->to;
  p->from = n->state.ip_addr;

  char buf[32];

  if (n->state.verbose)
    printf("sending to %s\n" , inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf)));

  ret = sendto(n->sd,
               (char*) &p->data, // char* required for win32
               p->length,
               0,
               (SA*) &addr,
               sizeof(addr));
  if (ret == -1) {
    artnet_error("Sendto failed: %s", artnet_net_last_error());
    n->state.report_code = ARTNET_RCUDPFAIL;
    return ARTNET_ENET;

  } else if (p->length != ret) {
    artnet_error("failed to send full datagram");
    n->state.report_code = ARTNET_RCSOCKETWR1;
    return ARTNET_ENET;
  }

  if (n->callbacks.send.fh) {
    get_type(p);
    n->callbacks.send.fh(n, p, n->callbacks.send.data);
  }
  return ARTNET_EOK;
}

int artnet_net_set_fdset(node n, fd_set *fdset) {
  FD_SET((unsigned int) n->sd, fdset);
  return ARTNET_EOK;
}

/*
 * Close a socket
 */
int artnet_net_close(artnet_socket_t sock) {
  if (zsock_close(sock)) {
    artnet_error(artnet_net_last_error());
    return ARTNET_ENET;
  }

  return ARTNET_EOK;
}


/*
 * Convert a string to an in_addr
 */
int artnet_net_inet_aton(const char *ip_address, struct in_addr *address) {
  in_addr_t *addr = (in_addr_t*) address;
  if (inet_pton(AF_INET, ip_address, addr) != 1 &&
      strcmp(ip_address, "255.255.255.255")) {

      artnet_error("IP conversion from %s failed", ip_address);
      return ARTNET_EARG;
  }
  return ARTNET_EOK;
}


/*
 *
 */
const char *artnet_net_last_error() {
  return strerror(errno);
}

