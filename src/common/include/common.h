#ifndef __COMMON_H__
#define __COMMON_H__

#include <sys/types.h>
#include <netinet/in.h>
#include "mlog.h"


/** Type definitions */  
#ifndef int8_t
# define  int8_t   signed char 
#endif
#ifndef int16_t
# define  int16_t  signed short
#endif
#ifndef int32_t
# define  int32_t  signed int
#endif
#ifndef int64_t
# define  int64_t  long long
#endif
#ifndef uint8_t
# define  uint8_t  unsigned char
#endif
#ifndef uint16_t
# define  uint16_t unsigned short
#endif
#ifndef uint32_t
# define  uint32_t unsigned int
#endif
#ifndef uint64_t
# define  uint64_t unsigned long long
#endif


/** Limits of integral types. */
#ifndef INT8_MIN
# define INT8_MIN               (-0x7f - 1)
#endif
#ifndef INT16_MIN
# define INT16_MIN              (-0x7fff - 1)
#endif
#ifndef INT32_MIN
# define INT32_MIN              (-0x7fffffff - 1)
#endif
#ifndef INT64_MIN
# define INT64_MIN              (-0x7fffffffffffffffLL - 1)
#endif
#ifndef INT8_MAX
# define INT8_MAX               (0x7f)
#endif
#ifndef INT16_MAX
# define INT16_MAX              (0x7fff)
#endif
#ifndef INT32_MAX
# define INT32_MAX              (0x7fffffff)
#endif
#ifndef INT64_MAX
# define INT64_MAX              (0x7fffffffffffffffLL)
#endif
#ifndef UINT8_MAX
# define UINT8_MAX              (0xff)
#endif
#ifndef UINT16_MAX
# define UINT16_MAX             (0xffff)
#endif
#ifndef UINT32_MAX
# define UINT32_MAX             (0xffffffff)
#endif
#ifndef UINT64_MAX
# define UINT64_MAX             (0xffffffffffffffffULL)
#endif

#ifndef WITH_DEBUG
# undef assert
# define assert(e)
#endif

/** Define boolean values */
#ifndef FALSE
#  define FALSE 0
#  define TRUE (!FALSE)
#endif

/** Useful function macros */
#if !defined(min) || !defined(max)
#  define min(a,b)	((a) < (b) ? (a) : (b))
#  define max(a,b)	((a) > (b) ? (a) : (b))
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))


typedef struct cm_buf_t
{
    unsigned char* base;
    size_t len;
} cm_buf_t;


typedef struct cm_cpu_times_s{
    uint64_t user;
    uint64_t nice;
    uint64_t sys;
    uint64_t idle;
    uint64_t irq;
} cm_cpu_times_t;


typedef struct cm_cpu_info_s{
  char* model;
  int speed;
  cm_cpu_times_t cpu_times;
} cm_cpu_info_t;


void *cm_calloc(size_t nmemb, size_t size);

void *cm_malloc(size_t size);

char *cm_strdup(const char *s);

void cm_free(void *ptr);

int cm_close(int fd);

int cm_nonblock(int fd, int set);

int cm_cloexec(int fd, int set);

char *cm_ip4_inet_ntoa(struct sockaddr_in* addr);

int cm_get_cwd(char* buffer, size_t* size);

int cm_exepath(char* buffer, size_t* size);

uint64_t cm_get_free_memory(void);

uint64_t cm_get_total_memory(void);

void cm_set_process_title(const char* title);

char *cm_itoa(unsigned int i);

char* cm_s2hex(const unsigned char *s, char *t, int n);

const char* cm_hex2s(const char *s, char *t, size_t l, int *n);

char *cm_s2base64(const unsigned char *s, char *t, int n);

const char* cm_base642s(const char *s, char *t, size_t l, int *n);




#endif

