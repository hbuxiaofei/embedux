#ifndef __TCP_H__
#define __TCP_H__

#include "common.h"
#include "loop.h"
#include "epoll.h"

#define TCP_CONN_MAX      (128)

typedef void (*tp_connect_cb)(void* tp_server, int status);

typedef int (*tp_read_cb)(void *slave, ssize_t nread, cm_buf_t *buf);


typedef struct tp_slave_s{
    int socket;               /* socket to send and receive */   
    ep_io_t ep_io;
    void* slave_queue[2];
} tp_slave_t;


typedef struct tp_server_s{    
    char *ip_s;               /* ip str */
    int port;                 /* port number */
    int master;               /* socket bound to TCP/IP port */ 
    unsigned int flags;       /* domain */
    ep_io_t ep_io;
    void* server_t_queue[2];
    cm_buf_t buf;
    tp_read_cb read_cb;
    void* slave_queue[2];
} tp_server_t;

int tp_tcp_nodelay(int fd, int on);

int tp_tcp_keepalive(int fd, int on, unsigned int delay);

int tp_server_slave_invaild(tp_slave_t *slave);

int tp_tcp_accept(loop_t* loop, tp_server_t* tcp);

int tp_tcp_connect(int socket, const struct sockaddr* addr, unsigned int addrlen);    

ssize_t tp_tcp_read(int socket ,cm_buf_t *buf);

ssize_t tp_tcp_write(int socket , size_t count, cm_buf_t *buf);

int tp_server_init(loop_t *loop);

int tp_server_start(loop_t *loop, tp_server_t *tp_s, unsigned int flags);


#endif

