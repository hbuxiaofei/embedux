#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>  /* getrlimit() */
#include <unistd.h>  /* getpagesize() */
#include <limits.h>

#include "common.h"
#include "mlog.h"
#include "threads.h"

static void* trd_thread_start(void *arg)
{
    trd_thread_ctx_t *ctx_p;
    trd_thread_ctx_t ctx;

    ctx_p = (trd_thread_ctx_t *)arg;
    ctx = *ctx_p;

    cm_free(ctx_p);

    ctx.entry(ctx.arg);

    return 0;
}

int trd_thread_create(pthread_t *tid, void (*entry)(void *arg), void *arg)
{
    trd_thread_ctx_t *ctx;
    int err;
    pthread_attr_t* attr;

    ctx = (trd_thread_ctx_t *)cm_malloc(sizeof(trd_thread_ctx_t));
    if (ctx == NULL)
        return -1;

    ctx->entry = entry;
    ctx->arg = arg;
    attr = NULL;

    err = pthread_create(tid, attr, trd_thread_start, ctx);
    
    if (err)
        cm_free(ctx);

    return err;   
}

trd_thread_t trd_thread_self(void)
{
    return pthread_self();
}

int trd_thread_join(trd_thread_t *tid)
{
    return -pthread_join(*tid, NULL);
}

int trd_thread_detach(trd_thread_t *tid)
{
    return pthread_detach(*tid);
}

int trd_thread_equal(const trd_thread_t* t1, const trd_thread_t* t2)
{
    return pthread_equal(*t1, *t2);
}


int trd_mutex_init(trd_mutex_t* mutex)
{
    return -pthread_mutex_init(mutex, NULL);
}

void trd_mutex_destroy(trd_mutex_t* mutex)
{
    if (pthread_mutex_destroy(mutex)) {
        MLOG_ERROR("error !\n");
        abort();
    }
}

void trd_mutex_lock(trd_mutex_t* mutex)
{
    if (pthread_mutex_lock(mutex)) {
        MLOG_ERROR("error !\n");
        abort();
    }
}

int trd_mutex_trylock(trd_mutex_t* mutex)
{
    int err;

    err = pthread_mutex_trylock(mutex);
    if (err) {
      if (err != EBUSY && err != EAGAIN)
          abort();
      return -EBUSY;
    }

    return 0;
}

void uv_mutex_unlock(pthread_mutex_t* mutex)
{
    if (pthread_mutex_unlock(mutex)) {
        MLOG_ERROR("error !\n");
        abort();
    }
}




int trd_rwlock_init(trd_rwlock_t* rwlock)
{
    return -pthread_rwlock_init(rwlock, NULL);
}

void trd_rwlock_destroy(trd_rwlock_t* rwlock)
{
    if (pthread_rwlock_destroy(rwlock)) {
        MLOG_ERROR("error !\n");
        abort();
    }
}

void trd_rwlock_rdlock(trd_rwlock_t* rwlock)
{
    if (pthread_rwlock_rdlock(rwlock)) {
        MLOG_ERROR("error !\n");
        abort();
    }
}

int trd_rwlock_tryrdlock(trd_rwlock_t* rwlock)
{
    int err;

    err = pthread_rwlock_tryrdlock(rwlock);
    if (err) {
        if (err != EBUSY && err != EAGAIN)
            abort();
        return -EBUSY;
    }

    return 0;
}

void trd_rwlock_rdunlock(trd_rwlock_t* rwlock)
{
    if (pthread_rwlock_unlock(rwlock)) {
        MLOG_ERROR("error !\n");   
        abort();
    }
}

void trd_rwlock_wrlock(trd_rwlock_t* rwlock)
{
    if (pthread_rwlock_wrlock(rwlock)) {
        MLOG_ERROR("error !\n");
        abort();
    }
}

int trd_rwlock_trywrlock(trd_rwlock_t* rwlock)
{
    int err;

    err = pthread_rwlock_trywrlock(rwlock);
    if (err) {
        if (err != EBUSY && err != EAGAIN)
            abort();
        return -EBUSY;
    }

    return 0;
}

void trd_rwlock_wrunlock(trd_rwlock_t* rwlock)
{
    if (pthread_rwlock_unlock(rwlock)) {
        MLOG_ERROR("error !\n");
        abort();
    }
}



int trd_sem_init(trd_sem_t* sem, unsigned int value)
{
    if (sem_init(sem, 0, value))
        return -errno;
    return 0;
}

void trd_sem_destroy(trd_sem_t* sem)
{
    if (sem_destroy(sem))
        abort();
}

void trd_sem_wait(trd_sem_t* sem)
{
    int r;

    do
        r = sem_wait(sem);
    while (r == -1 && errno == EINTR);

    if (r)
        abort();
}

int trd_sem_trywait(trd_sem_t* sem)
{
    int r;

    do
        r = sem_trywait(sem);
    while (r == -1 && errno == EINTR);

    if (r) {
        if (errno == EAGAIN)
            return -EAGAIN;
        abort();
    }

    return 0;
}

void trd_sem_post(trd_sem_t* sem)
{
    if (sem_post(sem))
        abort();
}



