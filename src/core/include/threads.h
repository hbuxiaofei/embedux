#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>
#include <semaphore.h>

typedef pthread_t trd_thread_t;
typedef pthread_mutex_t trd_mutex_t;
typedef pthread_rwlock_t trd_rwlock_t;
typedef sem_t trd_sem_t;


typedef struct trd_thread_ctx_s{
    void (*entry)(void* arg);
    void* arg;
} trd_thread_ctx_t;


#define MUTEX_INITIALIZER	PTHREAD_MUTEX_INITIALIZER


int trd_thread_create(trd_thread_t *tid, void (*entry)(void *arg), void *arg);

trd_thread_t trd_thread_self(void);

int trd_thread_join(trd_thread_t *tid);

int trd_thread_detach(trd_thread_t *tid);

int trd_thread_equal(const trd_thread_t* t1, const trd_thread_t* t2);




int trd_mutex_init(trd_mutex_t* mutex);

void trd_mutex_destroy(trd_mutex_t* mutex);

void trd_mutex_lock(trd_mutex_t* mutex);

int trd_mutex_trylock(trd_mutex_t* mutex);

void uv_mutex_unlock(trd_mutex_t* mutex);



int trd_rwlock_init(trd_rwlock_t* rwlock);

void trd_rwlock_destroy(trd_rwlock_t* rwlock);

void trd_rwlock_rdlock(trd_rwlock_t* rwlock);

int trd_rwlock_tryrdlock(trd_rwlock_t* rwlock);

void trd_rwlock_rdunlock(trd_rwlock_t* rwlock);

void trd_rwlock_wrlock(trd_rwlock_t* rwlock);

int trd_rwlock_trywrlock(trd_rwlock_t* rwlock);

void trd_rwlock_wrunlock(trd_rwlock_t* rwlock);



int trd_sem_init(trd_sem_t* sem, unsigned int value);

void trd_sem_destroy(trd_sem_t* sem);

void trd_sem_wait(trd_sem_t* sem);

int trd_sem_trywait(trd_sem_t* sem);

void trd_sem_post(trd_sem_t* sem);






#endif

