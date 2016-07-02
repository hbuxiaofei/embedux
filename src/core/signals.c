#include <stdlib.h>
#include <signal.h>             /* signal */
#include <time.h>


#include "mlog.h"

void signals_handler(int dummy)
{
    MLOG_ERROR("signal :%d !\n", dummy);
    abort();
}

void signals_init(void)
{    
    struct sigaction sa;

    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGINT);

    sa.sa_handler = signals_handler;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

}




