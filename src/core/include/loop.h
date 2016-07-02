#ifndef __LOOP_H__
#define __LOOP_H__

#include "common.h"
#include "heap.h"

#define LOOP_EPOOL_MAX 32

#define LOOP_TIMER_MAX UINT64_MAX
#define LOOP_TIMER_MIN (1)


typedef void (*loop_timer_cb)(void* timer);

typedef struct loop_epoll_s{
    int epfd;    
    struct epoll_event* event;
    int nevents;
} loop_epoll_t;


typedef struct loop_timer_s{
    /** User data */
    void* data;
    loop_timer_cb cb;
    struct heap_node heap_node;
    uint64_t timeout;  // ms
    uint64_t nexttime; // ms
    uint64_t repeat;
    uint64_t start_id;
} loop_timer_t;


typedef struct loop_s{
    /** User data - use this for whatever. */
    void* data;
    /** Loop reference counting. */
    unsigned int active_handles; 
    /** Internal flag to signal loop stop. */
    unsigned int stop_flag;    

    uint64_t time;          // timer time (ms)
    struct heap timer_heap;
    
    void* server_t_queue[2]; // tcp server
    
    void* server_u_queue[2]; // udp server    

    void* watcher_queue[2];  // will add to epoll
    loop_epoll_t epollop;    // epoll    
    
} loop_t;


int loop_init(loop_t *loop);

int loop_run(loop_t* loop);

int loop_stop(loop_t* loop);

int loop_exit(loop_t* loop);

int loop_timer_start(loop_t* loop, loop_timer_t* timer);




#endif

