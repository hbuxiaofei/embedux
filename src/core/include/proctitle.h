#ifndef __PROCTITLE_H__
#define __PROCTITLE_H__

#include <sys/types.h>


char** proct_setup_args(int argc, char** argv);

int proct_get_process_title(char* buffer, size_t size);

int proct_set_process_title(const char* title);

void proct_free_args_mem(void);


#endif

