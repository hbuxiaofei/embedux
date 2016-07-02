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
#include "net.h"

int tp_tcp_nodelay(int fd, int on)
{
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on))) {
        MLOG_ERROR("IPPROTO_TCP, TCP_NODELAY");
        return -errno;
    }
    return 0;
}

int tp_tcp_keepalive(int fd, int on, unsigned int delay)
{
    /* From libuv .But way ? */
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on))) {
        MLOG_ERROR("SOL_SOCKET, SO_KEEPALIVE");
        return -errno;
    }

#ifdef TCP_KEEPIDLE
    if (on && setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &delay, sizeof(delay))) {
        MLOG_ERROR("IPPROTO_TCP, TCP_KEEPIDLE");
        return -errno;
    }
#endif

    /* Solaris/SmartOS, if you don't support keep-alive,
    * then don't advertise it in your system headers...
    */
    /* FIXME(bnoordhuis) That's possibly because sizeof(delay) should be 1. */

#if defined(TCP_KEEPALIVE) && !defined(__sun)
    if (on && setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &delay, sizeof(delay))) {
        MLOG_ERROR("IPPROTO_TCP, TCP_KEEPALIVE");
        return -errno;
    }
#endif

return 0;
}

int tp_server_slave_invaild(tp_slave_t *slave)
{
    if (!slave)
        return -1;

    if (slave->ep_io.fd_status == EP_FD_CLOSE) {
        printf("#### slave->ep_io.fd = %d \n", slave->ep_io.fd);
        QUEUE_REMOVE(&slave->slave_queue);
        
        cm_free(slave);
        slave = NULL;

        return TRUE;
    }
    
    return FALSE;
}


int tp_server_init(loop_t* loop)
{
    QUEUE_INIT(&loop->server_t_queue);
    return 0;
}

static int tp_tcp_bind(tp_server_t* handle, 
                        struct sockaddr *addr,
                        unsigned int addrlen, 
                        int domain)
{
    int fd;    
    int err;

    err = net_socket(domain, SOCK_STREAM, 0);
    if (err < 0) {
        MLOG_ERROR("net_socket error !\n")
        return err;
    }
    fd = err;    
    handle->master = fd;

    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&on, sizeof(on)) < 0) {
        cm_close(fd);
        MLOG_ERROR("SOL_SOCKET, SO_KEEPALIVE !\n");
        return -errno;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on)) < 0 ) {
        cm_close(fd);
        MLOG_ERROR("SOL_SOCKET, SO_REUSEADDR !\n");
        return -errno;
    }   

    if (bind(handle->master, addr, addrlen)) {
        MLOG_ERROR("bind error !\n");
        return -errno;
    }

    return 0;
}

int tp_tcp_connect(int socket, const struct sockaddr* addr, unsigned int addrlen)
{
    int err;
    
    do {
      err = connect(socket, addr, addrlen);
    } while (err == -1 && errno == EINTR);

    if (err == -1) {
        if (errno == EINPROGRESS)
            ; /* not an error */
        else if (errno == ECONNREFUSED)
            /* If we get a ECONNREFUSED wait until the next tick to report the
            * error. Solaris wants to report immediately--other unixes want to
            * wait.
            */
            err = -errno;
        else
            err = -errno;
    }
    
    return err;
}

static int tp_tcp_listen(tp_server_t* tcp, int backlog)
{
    if (listen(tcp->master, backlog))
        return -errno;

    return 0;
}

int tp_tcp_accept(loop_t* loop, tp_server_t* tcp)
{   
    tp_slave_t *slave;
    int addrlen;
    socklen_t socket;
    struct sockaddr_in slaveaddr;
    int err;

    if (tcp->flags == AF_INET)
        addrlen = sizeof(struct sockaddr_in);
    else if (tcp->flags == AF_INET6)
        addrlen = sizeof(struct sockaddr_in6);
    else
        return ER_EINVAL;
    
    slave = (tp_slave_t *)cm_calloc(1, sizeof(tp_slave_t));

    if (!slave) {
        return -1;
    }   
    
    socket = accept(tcp->master, (struct sockaddr *)&slaveaddr, (socklen_t *)&addrlen);
    if (socket < 0) {
        MLOG_ERROR("error accept !\n");
        return -1;
    }

    MLOG_DEBUG("connect :%s \n", net_ip4_inet_ntoa(&slaveaddr) );

    err = cm_nonblock(socket, 1);
    if (err < 0) {
        cm_close(socket);
        return err;
    }
    
    err = cm_cloexec(socket, 1);
    if (err < 0) {
        cm_close(socket);
        return err;
    }
    
    slave->socket = socket;

    slave->ep_io.fd = socket;
    slave->ep_io.operate = EP_OPERATE_ADD;
    slave->ep_io.events = EPOLLIN|EPOLLET;
  
    QUEUE_INSERT_TAIL(&loop->watcher_queue, &slave->ep_io.watcher_queue);

    QUEUE_INSERT_TAIL(&tcp->slave_queue, &slave->slave_queue);

    return 0;
}



int tp_server_start(loop_t *loop, tp_server_t *tp_s, unsigned int flags)
{
    int err;
    int domain;
    int addrlen;
    struct sockaddr* addr;

    tp_s->flags = flags;
    
    /* Use the lower 8 bits for the domain (from libuv)*/
    domain = flags & 0xFF;
    if(domain != AF_INET && domain != AF_INET6 && domain != AF_UNSPEC)
        return ER_EINVAL;

    if(flags & ~0xFF)
        return ER_EINVAL;

    if (!tp_s) {
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

    net_ip4_addr(tp_s->ip_s, tp_s->port, (struct sockaddr_in *)addr);

    err = tp_tcp_bind(tp_s, addr, addrlen, domain);
    cm_free(addr);
    if ( err < 0) {
        MLOG_ERROR("tp_tcp_bind error !\n");
        return err;
    } else {
        MLOG_DEBUG("tp_tcp_bind successfully !\n");
    }
    
    err = tp_tcp_listen(tp_s, TCP_CONN_MAX);
    if ( err < 0) {
        MLOG_ERROR("tp_tcp_listen error !");
        return err;
    } else {
        MLOG_DEBUG("tp_tcp_listen successfully !\n");
    } 

    tp_s->ep_io.fd = tp_s->master;
    tp_s->ep_io.operate = EP_OPERATE_ADD;
    tp_s->ep_io.events = EPOLLIN|EPOLLET;
  
    QUEUE_INSERT_TAIL(&loop->watcher_queue, &tp_s->ep_io.watcher_queue);
    QUEUE_INSERT_TAIL(&loop->server_t_queue, &tp_s->server_t_queue);
    QUEUE_INIT(&tp_s->slave_queue);
    
    return 0;
}


ssize_t tp_tcp_read(int socket ,cm_buf_t *buf)
{
    ssize_t nread;

    if (!buf || !buf->base)
        return -1;

    do {
        nread = read(socket, buf->base, buf->len);
    } while (nread < 0 && errno == EINTR);    

    return nread;
}


ssize_t tp_tcp_write(int socket , size_t count, cm_buf_t *buf)
{
    ssize_t nwrite;
    size_t bytestosend;
    unsigned char* buffer;
    
    if (!buf || !buf->base)
        return -1;

    if (count > buf->len)
        return -1;

    buffer = buf->base;
    bytestosend = count;
    while (1) {
        nwrite =  write(socket, buffer, bytestosend);
        if (nwrite < 0) {
            if (errno == EINTR)
                continue;
            else
                return -errno;
        }

        if (nwrite == bytestosend)
            break;

        buffer += nwrite;
        bytestosend -= nwrite;
    }

    return count;
}













