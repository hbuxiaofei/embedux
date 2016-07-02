#ifndef __UDP_H__
#define __UDP_H__

#include "loop.h"
#include "epoll.h"

typedef int (*up_recvmsg_cb)(void *up_s, ssize_t nread, cm_buf_t *buf);

typedef struct up_slave_s{
    unsigned int flags;       /* domain */
    char *ip;
    int port;
} up_slave_t;


typedef struct up_server_s{    
    char *ip_s;               /* ip str */
    int port;                 /* port number */
    int master;               /* socket bound to TCP/IP port */ 
    unsigned int flags;       /* domain */ 
    ep_io_t ep_io;
    up_recvmsg_cb recvmsg_cb;
    void* server_u_queue[2];
    cm_buf_t buf;
} up_server_t;


int up_server_init(loop_t* loop);

int up_server_start(loop_t *loop, up_server_t *up_s, unsigned int flags);

ssize_t up_udp_recvmsg(up_server_t *up_s, int socket ,cm_buf_t *buf);

int up_udp_sendmsg(up_slave_t *slave, size_t count, cm_buf_t *buf);



#endif

