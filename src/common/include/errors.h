#ifndef __ERRORS_H__
#define __ERRORS_H__

#define ER_UNKNOWN          (-4095)
#define ER_EOF              (-4094)
#define ER_EINVAL           (-4093)
#define ER_MEMORY           (-4092)
#define ER_EBADF            (-4091)
#define ER_EAFNOSUPPORT     (-4090)
#define ER_ENOMEM           (-4089)


#define ER_ERRNO_MAP(XX)                                                   \
    XX(UNKNOWN, "unknown error")                                           \
    XX(EOF, "end of file")                                                 \
    XX(EINVAL, "invalid argument")                                         \
    XX(MEMORY, "out of memory")                                            \
    XX(EBADF, "bad file descriptor")                                       \
    XX(EAFNOSUPPORT, "address family not supported")                       \
    XX(ENOMEM, "not enough memory")                                        \


typedef enum {
#define XX(code, _) E##code = ER_##code,
    ER_ERRNO_MAP(XX)
#undef XX
}uv_errno_t;


const char* er_err_name(int err);

const char* er_strerror(int err);



#endif

