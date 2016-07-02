#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include "common.h"
#include "errors.h"
#include "mlog.h"
#include "loop.h"
#include "epoll.h"
#include "tcp.h"
#include "udp.h"
#include "parser.h"
#include "daemon.h"
#include "signals.h"

void do_timer1_cb(void* timer)
{
    loop_timer_t *ptimer = (loop_timer_t *)timer;
    parser_t *parser;
    char *str;
    char exepath[128];
    int path_len;
    
    parser = (parser_t *)ptimer->data;
    
    str = parser_getstr(parser, "wine:country", NULL);

    MLOG_DEBUG("[Country] :%s \n", str);

    path_len = sizeof(exepath);
    cm_exepath(exepath, (size_t *)&path_len);

    MLOG_DEBUG("exepath = %s \n", exepath);
    
}

void do_timer2_cb(void* timer)
{    
    char str1[64] = {"hello world !"};
    char str2[64] = {0};
    char dst1[64] = {0};
    int n;

    loop_timer_t *ptimer = (loop_timer_t *)timer;
    parser_t *parser;
    double alc;
    
    parser = (parser_t *)ptimer->data;
    
    alc = parser_getdouble(parser, "wine:alcohol", 0);

    MLOG_DEBUG("[alcohol] :%f \n", alc);

    
    cm_s2base64((const unsigned char *)str1, dst1, strlen(str1));    
    cm_base642s((const char *)dst1, str2, strlen(dst1), &n);

    MLOG_DEBUG("str1 :%s \n", str1);
    MLOG_DEBUG("dst1 :%s \n", dst1);
    MLOG_DEBUG("str2 :%s \n", str2);
}

void do_ep_io_cb(loop_t* loop, void* args, unsigned int events)
{
    MLOG_DEBUG(" do_ep_io_cb run !\n");
}

int do_tp_read_cb(void *slave, ssize_t nread, cm_buf_t *buf)
{   
    tp_slave_t *sla = (tp_slave_t *)slave;
    char *str = "Hello slave !";
        
    MLOG_DEBUG("-->buf->base(%d) : %s !\n", nread, buf->base);

    memcpy(buf->base, str, strlen(str));
    nread = strlen(str);

    tp_tcp_write(sla->socket, nread, buf);

    return 0;
}

int do_up_recvmsg_cb(void *up_s, ssize_t nread, cm_buf_t *buf)
{
    //up_slave_t slave;
        
    MLOG_DEBUG("recvmsg :%s \n", buf->base);

    //up_udp_sendmsg(up_slave_t *slave, size_t count, cm_buf_t *buf)
    
    return 0;
}


int main(int argc, char *argv[])
{
    loop_t loop;
    loop_timer_t timer1;
    loop_timer_t timer2;

    tp_server_t tcp_server1;
    up_server_t udp_server1;

    parser_t parser;

    signals_init();

    MLOG_DEBUG("%s is \"%s\" \n", er_err_name(ER_EINVAL), er_strerror(ER_EINVAL));
    MLOG_DEBUG("%s is \"%s\" \n", er_err_name(11), er_strerror(11));

    MLOG_DEBUG("argc = %d \n", argc);
    if (0 && argc != 3) {
        MLOG_ERROR("Use as : x.out <ip> <port> \n");
        return 0;
    }

    char buffer[128];
    int cwdsize;
    cm_get_cwd(buffer, (size_t *)&cwdsize);
    strcat(strrchr(buffer, '/'), "/example.ini");
    MLOG_DEBUG("getcwd(%d) :%s \n", cwdsize, buffer);
        
    //daemon_make();

    memset(&parser, 0, sizeof(parser_t));
    parser.path = buffer;
    if (parser_start(&parser) != 0) {
        MLOG_ERROR("parser_start error !\n");
    }
        
    
#if 1
    loop_init(&loop);

    timer1.start_id = 1;
    timer1.repeat = 200;
    timer1.data = (void *)&parser;
    timer1.timeout = 1 * 1000;
    timer1.cb = do_timer1_cb;
    loop_timer_start(&loop, &timer1);

    timer2.start_id = 2;
    timer2.repeat = 5;
    timer2.timeout = 2 * 1000;
    timer2.data = (void *)&parser;
    timer2.cb = do_timer2_cb;
    loop_timer_start(&loop, &timer2);

    memset(&tcp_server1, 0 ,sizeof(tcp_server1));
    tcp_server1.ip_s = "192.168.2.200";
    tcp_server1.port = 9800;
    tcp_server1.read_cb = do_tp_read_cb;
    tcp_server1.buf.base = cm_malloc(1024);
    tcp_server1.buf.len = 1024;
    tp_server_start(&loop, &tcp_server1, AF_INET);

    memset(&udp_server1, 0 ,sizeof(udp_server1));
    udp_server1.ip_s = "192.168.2.200";
    udp_server1.port = 9800;
    udp_server1.recvmsg_cb = do_up_recvmsg_cb;
    udp_server1.buf.base = cm_malloc(1024);
    udp_server1.buf.len = 1024;
    up_server_start(&loop, &udp_server1, AF_INET);    

    loop_run(&loop);
#else
    char *udp_str = "udp 192.168.2.200";
    up_slave_t udp_slave;
    cm_buf_t udp_buf;
    int udp_err;
        
    udp_slave.flags = AF_INET;
    udp_slave.ip = "192.168.2.200";
    udp_slave.port = 9800;

    udp_buf.base = cm_malloc(1024);
    udp_buf.len = 1024;
    strcpy(udp_buf.base, udp_str);
    udp_err = up_udp_sendmsg(&udp_slave, strlen(udp_buf.base), &udp_buf);

    MLOG_DEBUG("udp_err = %d \n", udp_err);

    if (udp_buf.base)
        cm_free(udp_buf.base);
    
#endif

    return 0;
}





