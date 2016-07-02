#ifndef __NET_H__
#define __NET_H__

#include <netinet/in.h>

int net_socket(int domain, int type, int protocol);

unsigned long net_ip4_ifaddr(char *ifname);

int net_ip4_addr(const char* ip, int port, struct sockaddr_in* addr);

char *net_ip4_inet_ntoa(struct sockaddr_in* addr);


#endif

