#include <stdint.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

#include "common.h"
#include "errors.h"
#include "queue.h"
#include "loop.h"
#include "epoll.h"
#include "tcp.h"
#include "udp.h"
#include "epoll.h"

/* Initialize the kernel queue.
 * The size field is ignored since 2.6.8.
 */
int ep_epoll_create(int size)
{
#if !defined(__NR_epoll_create) && defined(__NR_epoll_create1)
	if (size <= 0)
    {
		errno = EINVAL;
		return -1;
	}
	return (syscall(__NR_epoll_create1, 0));
#else
	return (syscall(__NR_epoll_create, size));
#endif
}

int ep_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
	return (syscall(__NR_epoll_ctl, epfd, op, fd, event));
}

int ep_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
#if !defined(__NR_epoll_wait) && defined(__NR_epoll_pwait)
	return (syscall(__NR_epoll_pwait, epfd, events, maxevents, timeout, NULL, 0));
#else
	return (syscall(__NR_epoll_wait, epfd, events, maxevents, timeout));
#endif
}

int ep_epoll_init(loop_t* loop)
{
    int epfd;
    
    /** From libevent */
    if((epfd = ep_epoll_create(32000)) == -1)
    {
        MLOG_ERROR("ep_epoll_create error !\n");
        return -1;
    }
    cm_cloexec(epfd, 1);
    loop->epollop.epfd = epfd;

    QUEUE_INIT(&loop->watcher_queue);    

    return 0;
}

void ep_ip_watcher_oper(loop_t *loop)
{
    QUEUE *q;
    ep_io_t *handle_io;
    struct epoll_event ev;
    
    while (!QUEUE_EMPTY(&loop->watcher_queue)) {            

        q = QUEUE_HEAD(&loop->watcher_queue);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);
        handle_io = QUEUE_DATA(q, ep_io_t, watcher_queue);
        
        ev.data.fd = handle_io->fd;
        ev.events = handle_io->events;

        if (handle_io->operate == EP_OPERATE_DEL 
            && loop->epollop.nevents <= 0)
            continue;

        if (handle_io->operate == EP_OPERATE_ADD 
            && loop->epollop.nevents >= LOOP_EPOOL_MAX)
            continue;

        if (handle_io->operate == EP_OPERATE_ADD) {
            if (!ep_epoll_ctl(loop->epollop.epfd, 
                            EPOLL_CTL_ADD, 
                            handle_io->fd, 
                            &ev))           
                loop->epollop.nevents++;
            
        } else if(handle_io->operate == EP_OPERATE_DEL) {
            if (!ep_epoll_ctl(loop->epollop.epfd, 
                            EPOLL_CTL_DEL, 
                            handle_io->fd, 
                            &ev)) {
                loop->epollop.nevents--;
                if (handle_io->fd_status == EP_FD_CLOSE) {
                    cm_close(handle_io->fd);
                }
            }
                
        }  else if(handle_io->operate == EP_OPERATE_MOD) {
            ep_epoll_ctl(loop->epollop.epfd, 
                            EPOLL_CTL_MOD, 
                            handle_io->fd, 
                            &ev);         
        } else {
            MLOG_ERROR("Operater not supported !\n");
        }
    }    
}

int ep_tcp_server_process(loop_t *loop, 
                        tp_server_t *tp_s, 
                        unsigned int events)
{
    int err;
    int on;
    
    MLOG_DEBUG("events = %d !\n", events);

    if (events & EPOLLIN) {    
        err = tp_tcp_accept(loop, tp_s);
    } else {
        MLOG_ERROR("events is not matched !\n");
        return -1;
    }    

    on = 1;
    tp_tcp_nodelay(tp_s->master, on);
    
    tp_tcp_keepalive(tp_s->master, on, 60);

    return err;
}

int ep_tcp_server_slave_process(loop_t *loop, 
                        tp_server_t *tp_s, 
                        tp_slave_t *slave, 
                        unsigned int events)
{
    ssize_t nread;

    MLOG_DEBUG("events = %d !\n", events);

    if (events & EPOLLIN) {
        nread = tp_tcp_read(slave->socket, &tp_s->buf);
    } else {
        MLOG_ERROR("events is not matched !\n");
        nread = -1;
    }

    if (nread <= 0) {
                
        slave->ep_io.fd_status = EP_FD_CLOSE;
        slave->ep_io.operate = EP_OPERATE_DEL;

        printf("######### slave->socket = %d \n", slave->socket);
        
        QUEUE_INSERT_TAIL(&loop->watcher_queue, &slave->ep_io.watcher_queue);

        return nread;
    }

    tp_s->buf.base[nread] = 0;
    tp_s->read_cb((void *)slave, nread, &tp_s->buf);
   
    return nread;
}


int ep_udp_server_process(loop_t *loop, 
                        up_server_t *up_s, 
                        unsigned int events)
{
    int err;
    
    MLOG_DEBUG("events = %d !\n", events);
        
    if (events & EPOLLIN) {
        err = up_udp_recvmsg(up_s, up_s->master, &up_s->buf);
    } else {
        MLOG_ERROR("events is not matched !\n");
        err = -1;
    }

    return err;
}


void ep_io_poll(loop_t *loop, int timeout)
{
    QUEUE *q, *q1;
    tp_server_t *handle_tcp_s;
    tp_slave_t *handle_tcp_sla;
    up_server_t *handle_udp_s;
    int nfds;
    int i, err;
    static unsigned int count;

    ep_ip_watcher_oper(loop);
    
    if (!loop->epollop.event)
        return ;

    if (loop->epollop.nevents <= 0)
        return ;
    
    /* TODO:timeout */
    nfds = ep_epoll_wait(loop->epollop.epfd, 
                        loop->epollop.event, 
                        LOOP_EPOOL_MAX, 
                        timeout);  

    if (count++ >= 100) {
        count = 0;
        MLOG_DEBUG("nevents = %d nfds = %d \n", loop->epollop.nevents, nfds);
    }

    for (i = 0; i < nfds; i++) {       

        if (!QUEUE_EMPTY(&loop->server_u_queue)) {
            QUEUE_FOREACH(q, &loop->server_u_queue) {
                handle_udp_s = QUEUE_DATA(q, up_server_t, server_u_queue);
                
                if (loop->epollop.event[i].data.fd == handle_udp_s->master) {
                    
                    err = ep_udp_server_process(loop,
                                        handle_udp_s,
                                        loop->epollop.event[i].events);

                    if (err < 0) {
                        MLOG_ERROR("error ep_udp_server_process :%d !\n", err);
                    }
                }
            }
        }            

        
        if (!QUEUE_EMPTY(&loop->server_t_queue)) {
            QUEUE_FOREACH(q, &loop->server_t_queue) {
                handle_tcp_s = QUEUE_DATA(q, tp_server_t, server_t_queue);
                
                if (loop->epollop.event[i].data.fd == handle_tcp_s->master) {
                    
                    err = ep_tcp_server_process(loop,
                                        handle_tcp_s,
                                        loop->epollop.event[i].events);                   
                    
                    if (err < 0) {
                        MLOG_ERROR("error ep_server_t_process :%d !\n", err);
                    }
                }
                
                if (!QUEUE_EMPTY(&handle_tcp_s->slave_queue)) {
                    QUEUE_FOREACH(q1, &handle_tcp_s->slave_queue) {
                        handle_tcp_sla = QUEUE_DATA(q1, tp_slave_t, slave_queue);                       

                        if (tp_server_slave_invaild(handle_tcp_sla))
                            continue;

                        if (loop->epollop.event[i].data.fd == handle_tcp_sla->socket) {

                            err = ep_tcp_server_slave_process(loop, 
                                                        handle_tcp_s, 
                                                        handle_tcp_sla, 
                                                        loop->epollop.event[i].events);
                            if (err < 0) {
                                MLOG_ERROR("error ep_tcp_server_slave_process :%d !\n", err);
                            }
                        }
                    }
                }
            }
        }
    }    
}




