#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "loop.h"

#define EP_FD_CLOSE       (-1)
#define EP_FD_OPEN        (0)

#define EP_OPERATE_ADD    (1)
#define EP_OPERATE_DEL    (2)
#define EP_OPERATE_MOD    (3)


typedef struct ep_io_s{
    int fd;
    int fd_status;
    int operate;
    int events;
    void* watcher_queue[2];
} ep_io_t;


int ep_epoll_init(loop_t* loop);

void ep_io_poll(loop_t* loop, int timeout);






#endif


