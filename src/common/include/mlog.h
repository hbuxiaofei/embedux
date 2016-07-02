#ifndef __MLOG_H__
#define __MLOG_H__

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>


#define MLOG_ENABLE
#define MLOG_LEVEL_DEBUG

#ifdef MLOG_ENABLE

#  define MLOG(format,type,args...)\
     do\
     {\
        char time_buffer[32];\
        memset(time_buffer,0,32);\
        struct timeval tv;\
        struct tm      tm;\
        size_t         len = 28;\
        gettimeofday(&tv, NULL);\
        localtime_r(&tv.tv_sec,&tm);\
        strftime(time_buffer,len,"%Y-%m-%d %H:%M:%S",&tm);\
        printf("%s[%s %s:%d %s]"format,time_buffer,type,__FILE__,__LINE__,__func__,##args);\
     }while(0);
    
#  ifdef MLOG_LEVEL_DEBUG
#    define MLOG_DEBUG(format, args...) MLOG(format,"Debug",##args)
#  else
#    define MLOG_DEBUG(format, args...)
#  endif
#  define MLOG_ERROR(format, args...)	MLOG(format,"Error",##args)

#else
#  define MLOG_ERROR(format, args...)
#  define MLOG_DEBUG(format, args...)
#endif









#endif

