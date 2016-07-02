#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "loop.h"
#include "queue.h"
#include "heap.h"
#include "errors.h"
#include "mlog.h"
#include "epoll.h"
#include "tcp.h"
#include "udp.h"

static uint64_t loop_timer_mstime() 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (((uint64_t) ts.tv_sec) * 1000 + ts.tv_nsec / (1000*1000));
}

static void loop_update_time(loop_t *loop)
{
    loop->time = loop_timer_mstime();
}


static int loop_timer_less_than(const struct heap_node* ha,
                           const struct heap_node* hb)
{
    const loop_timer_t* a;
    const loop_timer_t* b;

    a = container_of(ha, const loop_timer_t, heap_node);
    b = container_of(hb, const loop_timer_t, heap_node);

    if (a->nexttime< b->nexttime)
        return 1;
    if (b->nexttime < a->nexttime)
        return 0;

    /* Compare start_id when both have the same timeout. start_id is
    * allocated with loop->timer_counter in uv_timer_start().
    */
    if (a->start_id < b->start_id)
        return 1;
    if (b->start_id < a->start_id)
        return 0;

    return 0;
}


int loop_timer_start(loop_t* loop, loop_timer_t* timer)
{
    if(!timer) 
        return ER_ENOMEM;
    timer->nexttime = timer->timeout + loop_timer_mstime();        
    heap_insert(&loop->timer_heap,
                &timer->heap_node, 
                loop_timer_less_than);

    return 0;
}

int loop_timer_again(loop_t* loop, loop_timer_t* timer)
{
    if(timer->cb == NULL)
        return ER_EINVAL;

    if(timer->repeat && --timer->repeat)
        loop_timer_start(loop, timer);

    return 0;
}

int loop_timer_stop(loop_t* loop, loop_timer_t* timer) 
{
    heap_remove(&loop->timer_heap, 
                &timer->heap_node, 
                loop_timer_less_than);
    return 0;
}

void loop_timer_run(loop_t* loop)
{
    struct heap_node* heap_node;
    loop_timer_t* timer;

    for (;;) {
        heap_node = heap_min((struct heap*) &loop->timer_heap);
        
        if(heap_node == NULL)
            break;

        timer = container_of(heap_node, loop_timer_t, heap_node);

        if(timer->nexttime> loop->time)
            break;

        loop_timer_stop(loop, timer);
        loop_timer_again(loop, timer);
        timer->cb(timer);
    }
}

int loop_next_timeout(const loop_t* loop)
{
    const struct heap_node* heap_node;
    const loop_timer_t* handle;
    uint64_t diff;

    heap_node = heap_min((const struct heap*) &loop->timer_heap);
    if (heap_node == NULL)
        return -1; /* block indefinitely */

    handle = container_of(heap_node, const loop_timer_t, heap_node);
    if (handle->nexttime <= loop->time)
        return 0;

    diff = handle->nexttime- loop->time;
    if (diff > INT16_MAX)
        diff = INT16_MAX;

    return diff;
}


static void loop_timer_init(loop_t* loop)
{
    heap_init(&loop->timer_heap);
}

int loop_init(loop_t* loop)
{    
    memset(loop, 0, sizeof(loop_t));
    loop->epollop.epfd= -1;
    loop->epollop.event = cm_calloc(1, LOOP_EPOOL_MAX);

    /* timer */
    loop_timer_init(loop);

    /* tcp server */
    tp_server_init(loop);

    /* udp server */
    up_server_init(loop);

    /* epoll */
    ep_epoll_init(loop);
    
    return 0;
}

int loop_run(loop_t* loop)
{
    loop->stop_flag = 0;

    while (loop->stop_flag == 0) {       
        
        loop_update_time(loop);
        loop_timer_run(loop);
        
        loop_update_time(loop);
        ep_io_poll(loop, loop_next_timeout(loop));        

        usleep(2);
    }
    
    return 0;
}

int loop_stop(loop_t* loop)
{
    loop->stop_flag = 1;
    return 0;
}

int loop_exit(loop_t* loop)
{   
    loop->stop_flag = 1;

    if (loop->epollop.event) {
        cm_free(loop->epollop.event);
        loop->epollop.event = NULL;
    }
        

    return 0;
}



