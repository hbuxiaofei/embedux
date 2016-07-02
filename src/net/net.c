#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> /* malloc */
#include <string.h> /* memset */
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "common.h"

int net_socket(int domain, int type, int protocol)
{
    int sockfd;
    int err;

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
    sockfd = socket(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol);
    if (sockfd != -1)
        return sockfd;

    if (errno != EINVAL)
        return -errno;
#endif

    sockfd = socket(domain, type, protocol);
    if (sockfd == -1) {
        MLOG_ERROR("error socket !\n");
        return -errno;
    }

    err = cm_nonblock(sockfd, 1);
    if (err < 0) {
        cm_close(sockfd);
        return err;
    }
    
    err = cm_cloexec(sockfd, 1);
    if (err < 0) {
        cm_close(sockfd);
        return err;
    }

#if defined(SO_NOSIGPIPE)
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&on, sizeof(on));
#endif

    return sockfd;
}


/* ifname:"eth0" */
unsigned long net_ip4_ifaddr(char *ifname)
{
	struct ifreq ifr;
	int skfd;
	struct sockaddr_in *saddr;

	if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
		close(skfd);
		return -1;
	}
	close(skfd);
	
	saddr = (struct sockaddr_in *) &ifr.ifr_addr;
	return saddr->sin_addr.s_addr;
}

int net_ip4_addr(const char* ip, int port, struct sockaddr_in* addr)
{   
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = inet_addr(ip);
    
    return 0;
}

char *net_ip4_inet_ntoa(struct sockaddr_in* addr)
{
    return inet_ntoa(addr->sin_addr);
}




