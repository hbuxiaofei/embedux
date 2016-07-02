#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> /* malloc */
#include <string.h> /* memset */
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <sys/sysinfo.h>
#include "common.h"

/* This is rather annoying: CLOCK_BOOTTIME lives in <linux/time.h> but we can't
 * include that file because it conflicts with <time.h>. We'll just have to
 * define it ourselves.
 */
#ifndef CLOCK_BOOTTIME
# define CLOCK_BOOTTIME 7
#endif


static const char g_base64o[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char g_base64i[81] = "\76XXX\77\64\65\66\67\70\71\72\73\74\75XXXXXXX\00\01\02\03\04\05\06\07\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31XXXXXX\32\33\34\35\36\37\40\41\42\43\44\45\46\47\50\51\52\53\54\55\56\57\60\61\62\63";

void *cm_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

void *cm_malloc(size_t size) 
{
    return malloc(size);
}

void cm_free(void *ptr)
{
    free(ptr);
}

char *cm_strdup(const char *s) 
{
    size_t len = strlen(s) + 1;
    char *m = cm_malloc(len);
    if (m == NULL)
        return NULL;
    return memcpy(m, s, len);
}

char* cm_strndup(const char* s, size_t n)
{
    char* m;
    size_t len = strlen(s);
    if (n < len)
        len = n;
    m = cm_malloc(len + 1);
    if (m == NULL)
        return NULL;
    m[len] = '\0';
    
    return memcpy(m, s, len);
}


int cm_nonblock(int fd, int set) 
{
    int r;

    do {
        r = ioctl(fd, FIONBIO, &set);
    } while (r == -1 && errno == EINTR);

    if (r)
        return -errno;

    return 0;
}

int cm_cloexec(int fd, int set)
{
      int r;

      do {
        r = ioctl(fd, set ? FIOCLEX : FIONCLEX);
      } while (r == -1 && errno == EINTR);

      if (r)
          return -errno;

      return 0;
}


int cm_close_nocheckstdio(int fd)
{
    int saved_errno;
    int rc;

    assert(fd > -1);  /* Catch uninitialized io_watcher.fd bugs. */

    saved_errno = errno;
    rc = close(fd);
    if (rc == -1) {
        rc = -errno;
    if (rc == -EINTR)
        rc = -EINPROGRESS;  /* For platform/libc consistency. */
        errno = saved_errno;
    }

    return rc;
}

int cm_close(int fd)
{
    assert(fd > STDERR_FILENO);  /* Catch stdio close bugs. */
    return cm_close_nocheckstdio(fd);
}

int cm_open_cloexec(const char* path, int flags)
{
    int err;
    int fd;

    fd = open(path, flags);
    if (fd == -1)
        return -errno;

    err = cm_cloexec(fd, 1);
    if (err) {
        cm_close(fd);
        return err;
    }

    return fd;
}

/* get a file pointer to a file in read-only and close-on-exec mode */
FILE* cm_open_file(const char* path)
{
    int fd;
    FILE* fp;

    fd = cm_open_cloexec(path, O_RDONLY);
    if (fd < 0)
        return NULL;

    fp = fdopen(fd, "r");
    if (fp == NULL)
        cm_close(fd);

    return fp;
}

int cm_get_cwd(char* buffer, size_t* size)
{
    if (buffer == NULL || size == NULL)
        return -1;

    if (getcwd(buffer, *size) == NULL)
        return -errno;

    *size = strlen(buffer);
    if (*size > 1 && buffer[*size - 1] == '/') {
        buffer[*size-1] = '\0';
        (*size)--;
    }

    return 0;
}

int cm_exepath(char* buffer, size_t* size)
{
    ssize_t n;

    if (buffer == NULL || size == NULL || *size == 0)
        return -EINVAL;

    n = *size - 1;
    if (n > 0)
        n = readlink("/proc/self/exe", buffer, n);

    if (n == -1)
        return -errno;

    buffer[n] = '\0';
    *size = n;

    return 0;
}

char *cm_itoa(unsigned int i)
{
    /* 21 digits plus null terminator, good for 64-bit or smaller ints
     * for bigger ints, use a bigger buffer!
     *
     * 4294967295 is, incidentally, MAX_UINT (on 32bit systems at this time)
     * and is 10 bytes long
     */
    static char local[22];
    char *p = &local[21];
    *p = '\0';
    do {
        *--p = '0' + i % 10;
        i /= 10;
    } while (i != 0);
    return p;
}


char* cm_s2hex(const unsigned char *s, char *t, int n)
{ 
    char *p;
  
    if (!t)
        return NULL;
    
    p = t;
    t[0] = '\0';
    if (s) {
        for (; n > 0; n--) {
            int m = *s++;
            *t++ = (char)((m >> 4) + (m > 159 ? 'a' - 10 : '0'));
            m &= 0x0F;
            *t++ = (char)(m + (m > 9 ? 'a' - 10 : '0'));
        }
    }
    *t++ = '\0';
    return p;
}

const char* cm_hex2s(const char *s, char *t, size_t l, int *n)
{
    const char *p;
    if (!s || !*s) {
        if (n)
            *n = 0;   
        return NULL;    
    }
    
    l = strlen(s) / 2 + 1;	/* make sure enough space for \0 */
    
    if (!t)
        return NULL;
    p = t;

    while (l) { 
        int d1, d2;
        d1 = *s++;
        if (!d1)
            break;
        d2 = *s++;
        if (!d2)
            break;
        *t++ = (char)(((d1 >= 'A' ? (d1 & 0x7) + 9 : d1 - '0') << 4) + (d2 >= 'A' ? (d2 & 0x7) + 9 : d2 - '0'));
        l--;
    }
    if (n)
        *n = (int)(t - p);
    if (l)
        *t = '\0';
    return p;
}

    
char *cm_s2base64(const unsigned char *s, char *t, int n)
{ 
    int i;
    unsigned long m;
    char *p;

    if (!t)
        return NULL;

    p = t;
    t[0] = '\0';
    if (!s)
        return p;
    
    for (; n > 2; n -= 3, s += 3) {
        m = s[0];
        m = (m << 8) | s[1];
        m = (m << 8) | s[2];
        for (i = 4; i > 0; m >>= 6)
        t[--i] = g_base64o[m & 0x3F];
        t += 4;
    }
    
    t[0] = '\0';
    if (n > 0) { /* 0 < n <= 2 implies that t[0..4] is allocated (base64 scaling formula) */
        m = 0;
        for (i = 0; i < n; i++)
            m = (m << 8) | *s++;
        for (; i < 3; i++)
            m <<= 8;
        for (i = 4; i > 0; m >>= 6)
            t[--i] = g_base64o[m & 0x3F];
        for (i = 3; i > n; i--)
            t[i] = '=';
        t[4] = '\0';
    }
    return p;
}


const char* cm_base642s(const char *s, char *t, size_t l, int *n)
{ 
    size_t i, j;
    int c;
    unsigned long m;
    const char *p;

    if (!s || !*s) { 
        if (n)
            *n = 0;
        return NULL;
    }        
    l = (strlen(s) + 3) / 4 * 3 + 1;  /* space for raw binary and \0 */

    if (!t)
        return NULL;    
    p = t;
    
    if (n)
        *n = 0;
    
    for (i = 0; ; i += 3, l -= 3) { 
        m = 0;
        j = 0;
        while (j < 4) { 
            c = *s++;
            if (c == '=' || !c) { 
                if (l >= j - 1) { 
                    switch (j) { 
                        case 2:
                            *t++ = (char)((m >> 4) & 0xFF);
                            i++;
                            l--;
                            break;
                        case 3:
                        *t++ = (char)((m >> 10) & 0xFF);
                        *t++ = (char)((m >> 2) & 0xFF);
                        i += 2;
                        l -= 2;
                    }
                }
                if (n)
                    *n = (int)i;
                if (l)
                    *t = '\0';
                return p;
            }


            c -= '+';
            if (c >= 0 && c <= 79) {
                int b = g_base64i[c];
                if (b >= 64) {
                    return NULL;
                }
                m = (m << 6) + b;
                j++;
            } else if (!((c + '+')+1 > 0 && (c + '+') <= 32)) { 
                return NULL;
            }
        }

        if (l < 3) { 
            if (n)
                *n = (int)i;
            if (l)
                *t = '\0';
            return p;
        }
        *t++ = (char)((m >> 16) & 0xFF);
        *t++ = (char)((m >> 8) & 0xFF);
        *t++ = (char)(m & 0xFF);
    }
}


/* -------------------------------sys info------------------------------------- */

int cm_resident_set_memory(size_t* rss)
{
    char buf[1024];
    const char* s;
    ssize_t n;
    long val;
    int fd;
    int i;

    do
        fd = open("/proc/self/stat", O_RDONLY);
    while (fd == -1 && errno == EINTR);

    if (fd == -1)
        return -errno;

    do
        n = read(fd, buf, sizeof(buf) - 1);
    while (n == -1 && errno == EINTR);

    cm_close(fd);
    if (n == -1)
        return -errno;
    buf[n] = '\0';

    s = strchr(buf, ' ');
    if (s == NULL)
        goto err;

    s += 1;
    if (*s != '(')
        goto err;

    s = strchr(s, ')');
    if (s == NULL)
        goto err;

    for (i = 1; i <= 22; i++) {
        s = strchr(s + 1, ' ');
        if (s == NULL)
            goto err;
    }

    errno = 0;
    val = strtol(s, NULL, 10);
    if (errno != 0)
        goto err;
    if (val < 0)
        goto err;

    *rss = val * getpagesize();
    return 0;

err:
    return -EINVAL;
}

int cm_uptime(double* uptime)
{
    static volatile int no_clock_boottime;
    struct timespec now;
    int r;

    /* Try CLOCK_BOOTTIME first, fall back to CLOCK_MONOTONIC if not available
    * (pre-2.6.39 kernels). CLOCK_MONOTONIC doesn't increase when the system
    * is suspended.
    */
    if (no_clock_boottime) {
retry:  r = clock_gettime(CLOCK_MONOTONIC, &now);
    } else if ((r = clock_gettime(CLOCK_BOOTTIME, &now)) && errno == EINVAL) {
        no_clock_boottime = 1;
        goto retry;
    }

    if (r)
        return -errno;

    *uptime = now.tv_sec;
    return 0;
}


static int cm_cpu_num(FILE* statfile_fp, unsigned int* numcpus)
{
    unsigned int num;
    char buf[1024];

    if (!fgets(buf, sizeof(buf), statfile_fp))
        return -EIO;

    num = 0;
    while (fgets(buf, sizeof(buf), statfile_fp)) {
        if (strncmp(buf, "cpu", 3))
            break;
        num++;
    }

    if (num == 0)
        return -EIO;

    *numcpus = num;
    return 0;
}

/* Also reads the CPU frequency on x86. The other architectures only have
 * a BogoMIPS field, which may not be very accurate.
 *
 * Note: Simply returns on error, uv_cpu_info() takes care of the cleanup.
 */
static int read_models(unsigned int numcpus, cm_cpu_info_t* ci) 
{
    static const char model_marker[] = "model name\t: ";
    static const char speed_marker[] = "cpu MHz\t\t: ";
    const char* inferred_model;
    unsigned int model_idx;
    unsigned int speed_idx;
    char buf[1024];
    char* model;
    FILE* fp;

    /* Most are unused on non-ARM, non-MIPS and non-x86 architectures. */
    (void) &model_marker;
    (void) &speed_marker;
    (void) &speed_idx;
    (void) &model;
    (void) &buf;
    (void) &fp;

    model_idx = 0;
    speed_idx = 0;

#if defined(__arm__) || \
    defined(__i386__) || \
    defined(__mips__) || \
    defined(__x86_64__)
    fp = cm_open_file("/proc/cpuinfo");
    if (fp == NULL)
        return -errno;

    while (fgets(buf, sizeof(buf), fp)) {
        if (model_idx < numcpus) {
            if (strncmp(buf, model_marker, sizeof(model_marker) - 1) == 0) {
                model = buf + sizeof(model_marker) - 1;
                model = cm_strndup(model, strlen(model) - 1);  /* Strip newline. */
                if (model == NULL) {
                    fclose(fp);
                    return -ENOMEM;
                }
                ci[model_idx++].model = model;
                continue;
            }
        }
#if defined(__arm__) || defined(__mips__)
        if (model_idx < numcpus) {
#if defined(__arm__)
            /* Fallback for pre-3.8 kernels. */
            static const char model_marker[] = "Processor\t: ";
#else	/* defined(__mips__) */
            static const char model_marker[] = "cpu model\t\t: ";
#endif
            if (strncmp(buf, model_marker, sizeof(model_marker) - 1) == 0) {
                model = buf + sizeof(model_marker) - 1;
                model = cm_strndup(model, strlen(model) - 1);  /* Strip newline. */
                if (model == NULL) {
                    fclose(fp);
                    return -ENOMEM;
                }
                ci[model_idx++].model = model;
                continue;
            }
        }
#else  /* !__arm__ && !__mips__ */
        if (speed_idx < numcpus) {
            if (strncmp(buf, speed_marker, sizeof(speed_marker) - 1) == 0) {
                ci[speed_idx++].speed = atoi(buf + sizeof(speed_marker) - 1);
                continue;
            }
        }
#endif  /* __arm__ || __mips__ */
    }

    fclose(fp);
#endif  /* __arm__ || __i386__ || __mips__ || __x86_64__ */

    /* Now we want to make sure that all the models contain *something* because
    * it's not safe to leave them as null. Copy the last entry unless there
    * isn't one, in that case we simply put "unknown" into everything.
    */
    inferred_model = "unknown";
        if (model_idx > 0)
            inferred_model = ci[model_idx - 1].model;

    while (model_idx < numcpus) {
        model = cm_strndup(inferred_model, strlen(inferred_model));
        if (model == NULL)
            return -ENOMEM;
        ci[model_idx++].model = model;
    }

    return 0;
}

static int read_times(FILE* statfile_fp,
                      unsigned int numcpus,
                      cm_cpu_info_t* ci)
{
    unsigned long clock_ticks;
    cm_cpu_times_t ts;
    unsigned long user;
    unsigned long nice;
    unsigned long sys;
    unsigned long idle;
    unsigned long dummy;
    unsigned long irq;
    unsigned int num;
    unsigned int len;
    char buf[1024];

    clock_ticks = sysconf(_SC_CLK_TCK);
    assert(clock_ticks != (unsigned long) -1);
    assert(clock_ticks != 0);

    rewind(statfile_fp);

    if (!fgets(buf, sizeof(buf), statfile_fp))
        abort();

    num = 0;

    while (fgets(buf, sizeof(buf), statfile_fp)) {
        if (num >= numcpus)
            break;

        if (strncmp(buf, "cpu", 3))
            break;

        /* skip "cpu<num> " marker */
        {
            unsigned int n;
            int r = sscanf(buf, "cpu%u ", &n);
            assert(r == 1);
            (void) r;  /* silence build warning */
            for (len = sizeof("cpu0"); n /= 10; len++);
        }

        /* Line contains user, nice, system, idle, iowait, irq, softirq, steal,
         * guest, guest_nice but we're only interested in the first four + irq.
         *
         * Don't use %*s to skip fields or %ll to read straight into the uint64_t
         * fields, they're not allowed in C89 mode.
         */
        if (6 != sscanf(buf + len,
                        "%lu %lu %lu %lu %lu %lu",
                        &user,
                        &nice,
                        &sys,
                        &idle,
                        &dummy,
                        &irq))
            abort();

        ts.user = clock_ticks * user;
        ts.nice = clock_ticks * nice;
        ts.sys  = clock_ticks * sys;
        ts.idle = clock_ticks * idle;
        ts.irq  = clock_ticks * irq;
        ci[num++].cpu_times = ts;
    }
    assert(num == numcpus);
    return 0;
}

void cm_free_cpu_info(cm_cpu_info_t* cpu_infos, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        cm_free(cpu_infos[i].model);
    }

    cm_free(cpu_infos);
}

static unsigned long read_cpufreq(unsigned int cpunum)
{
    unsigned long val;
    char buf[1024];
    FILE* fp;

    snprintf(buf,
            sizeof(buf),
            "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_cur_freq",
            cpunum);

    fp = cm_open_file(buf);
    if (fp == NULL)
    return 0;

    if (fscanf(fp, "%lu", &val) != 1)
        val = 0;

    fclose(fp);

    return val;
}


static void read_speeds(unsigned int numcpus, cm_cpu_info_t* ci)
{
    unsigned int num;

    for (num = 0; num < numcpus; num++)
        ci[num].speed = read_cpufreq(num) / 1000;
}


int cm_cpu_info(cm_cpu_info_t** cpu_infos, int* count)
{
    unsigned int numcpus;
    cm_cpu_info_t* ci;
    int err;
    FILE* statfile_fp;

    *cpu_infos = NULL;
    *count = 0;

    statfile_fp = cm_open_file("/proc/stat");
    if (statfile_fp == NULL)
        return -errno;

    err = cm_cpu_num(statfile_fp, &numcpus);
    if (err < 0)
        goto out;

    err = -ENOMEM;
    ci = cm_calloc(numcpus, sizeof(*ci));
    if (ci == NULL)
        goto out;

    err = read_models(numcpus, ci);
    if (err == 0)
        err = read_times(statfile_fp, numcpus, ci);

    if (err) {
        cm_free_cpu_info(ci, numcpus);
        goto out;
    }

    /* read_models() on x86 also reads the CPU speed from /proc/cpuinfo.
    * We don't check for errors here. Worst case, the field is left zero.
    */
    if (ci[0].speed == 0)
        read_speeds(numcpus, ci);

    *cpu_infos = ci;
    *count = numcpus;
    err = 0;

out:

    if (fclose(statfile_fp))
        if (errno != EINTR && errno != EINPROGRESS)
            abort();

    return err;
}


uint64_t cm_get_free_memory(void)
{
    return (uint64_t) sysconf(_SC_PAGESIZE) * sysconf(_SC_AVPHYS_PAGES);
}

uint64_t cm_get_total_memory(void)
{
    return (uint64_t) sysconf(_SC_PAGESIZE) * sysconf(_SC_PHYS_PAGES);
}

void cm_loadavg(double avg[3])
{
    struct sysinfo info;

    if (sysinfo(&info) < 0) return;

    avg[0] = (double) info.loads[0] / 65536.0;
    avg[1] = (double) info.loads[1] / 65536.0;
    avg[2] = (double) info.loads[2] / 65536.0;
}

void cm_set_process_title(const char* title)
{
#if defined(PR_SET_NAME)
    prctl(PR_SET_NAME, title);  /* Only copies first 16 characters. */
#endif
}



