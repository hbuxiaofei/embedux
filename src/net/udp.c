#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include "common.h"
#include "loop.h"
#include "tcp.h"
#include "errors.h"
#include "queue.h"
#include "udp.h"
#include "net.h"

int up_server_init(loop_t* loop)
{
    QUEUE_INIT(&loop->server_u_queue);
    return 0;
}

int up_udp_bind(up_server_t *up_s, struct sockaddr *addr, unsigned int addrlen, int domain)
{
    int fd;    
    int err;

    err = net_socket(domain, SOCK_DGRAM, 0);
    if (err < 0) {
        MLOG_ERROR("net_socket error !\n")
        return err;
    }
    
    fd = err;    
    up_s->master = fd;

    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
        MLOG_ERROR("SOL_SOCKET, SO_REUSEADDR !\n");
        return -errno;
    }

    if (bind(up_s->master, addr, addrlen)) {
        MLOG_ERROR("bind error !\n");
        return -errno;
    }

    return 0;    
}

int up_server_start(loop_t *loop, up_server_t *up_s, unsigned int flags)
{
    int err;
    int domain;
    int addrlen;
    struct sockaddr* addr;

    up_s->flags = flags;

    /* Use the lower 8 bits for the domain (from libuv)*/
    domain = flags & 0xFF;
    if(domain != AF_INET && domain != AF_INET6 && domain != AF_UNSPEC)
        return ER_EINVAL;

    if(flags & ~0xFF)
        return ER_EINVAL;

    if (!up_s) {
        MLOG_ERROR("errno :%d \n", ER_ENOMEM);
        return ER_ENOMEM;
    }

    if (domain == AF_INET) {
        addrlen = sizeof(struct sockaddr_in);
        addr = (struct sockaddr *)cm_calloc(1, addrlen);

    } else if (domain == AF_INET6) {
        addrlen = sizeof(struct sockaddr_in6);
        addr = (struct sockaddr *)cm_calloc(1, addrlen);
        MLOG_ERROR("errno :%d \n", ER_EAFNOSUPPORT);
        return ER_EAFNOSUPPORT;

    } else {
        MLOG_ERROR("errno :%d \n", ER_EINVAL);
        return ER_EINVAL;
    }

    if (!addr) {
        MLOG_ERROR("errno :%d \n", ER_ENOMEM);
        return ER_ENOMEM;
    }

    net_ip4_addr(up_s->ip_s, up_s->port, (struct sockaddr_in *)addr);

    err = up_udp_bind(up_s, addr, addrlen, domain);    
    cm_free(addr);    
    if ( err < 0) {
        MLOG_ERROR("tp_tcp_bind error !\n");
        return err;
    } else {
        MLOG_DEBUG("tp_tcp_bind successfully !\n");
    }

    up_s->ep_io.fd = up_s->master;
    up_s->ep_io.operate = EP_OPERATE_ADD;
    up_s->ep_io.events = EPOLLIN|EPOLLET;
  
    QUEUE_INSERT_TAIL(&loop->watcher_queue, &up_s->ep_io.watcher_queue);
    QUEUE_INSERT_TAIL(&loop->server_u_queue, &up_s->server_u_queue);

    return 0;
}



ssize_t up_udp_recvmsg(up_server_t *up_s, int socket ,cm_buf_t *buf)
{
    struct sockaddr_storage peer;
    struct msghdr h;
    struct iovec iov[1];
    ssize_t nread;

    iov[0].iov_base = buf->base;
    iov[0].iov_len = buf->len;

    memset(&h, 0, sizeof(h));
    h.msg_name = &peer;
    h.msg_namelen = sizeof(peer);
    h.msg_iov = iov;
    h.msg_iovlen = 1;

    do {
      nread = recvmsg(socket, &h, 0);
    }
    while (nread == -1 && errno == EINTR);

    up_s->recvmsg_cb(up_s, nread, buf);

    return nread;
}

int up_udp_sendmsg(up_slave_t *slave, size_t count, cm_buf_t *buf)
{
    int err;
    int socket;
    int domain;
    int addrlen;
    struct sockaddr *addr;
         
    struct msghdr h;
    struct iovec iov[1];   

    if (!slave) {
        MLOG_ERROR("errno :%d \n", ER_ENOMEM);
        return ER_ENOMEM;
    }
    
    domain = slave->flags & 0xFF;
    if(domain != AF_INET && domain != AF_INET6 && domain != AF_UNSPEC)
        return ER_EINVAL;

    if(slave->flags & ~0xFF)
        return ER_EINVAL;
   
    if (domain == AF_INET) {
        addrlen = sizeof(struct sockaddr_in);        

    } else if (domain == AF_INET6) {
        addrlen = sizeof(struct sockaddr_in6);
        MLOG_ERROR("errno :%d \n", ER_EAFNOSUPPORT);
        return ER_EAFNOSUPPORT;

    } else {
        MLOG_ERROR("errno :%d \n", ER_EINVAL);
        return ER_EINVAL;
    }

    err = net_socket(domain, SOCK_DGRAM, 0);
    if (err < 0) {
        MLOG_ERROR("net_socket error !\n")
        return err;
    }
    socket = err;

    addr = (struct sockaddr *)cm_calloc(1, addrlen);    
    if (!addr) {
        MLOG_ERROR("errno :%d \n", ER_ENOMEM);
        return ER_ENOMEM;
    }
    
    net_ip4_addr(slave->ip, slave->port, (struct sockaddr_in *)addr);

    memset(&h, 0, sizeof(struct msghdr));
    
    h.msg_name = addr;
    h.msg_namelen = addrlen;

    iov[0].iov_base = buf->base;
    iov[0].iov_len = count;

    h.msg_iov = (struct iovec *)iov;
    h.msg_iovlen = 1;  

    do {
        err = sendmsg(socket, &h, 0);
    } while (err == -1 && errno == EINTR);
    
    cm_close(socket);
    cm_free(addr);

    return err;
}





