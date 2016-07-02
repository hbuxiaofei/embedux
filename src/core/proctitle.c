#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "common.h"

static void* args_mem;

static struct {
    char* str;
    size_t len;
} process_title;


char** proct_setup_args(int argc, char** argv)
{
    char** new_argv;
    size_t size;
    char* s;
    int i;

    if (argc <= 0)
        return argv;

    /* Calculate how much memory we need for the argv strings. */
    size = 0;
    for (i = 0; i < argc; i++)
        size += strlen(argv[i]) + 1;

    process_title.str = argv[0];
    process_title.len = argv[argc - 1] + strlen(argv[argc - 1]) - argv[0];
    assert(process_title.len + 1 == size);  /* argv memory should be adjacent. */

    /* Add space for the argv pointers. */
    size += (argc + 1) * sizeof(char*);

    new_argv = cm_malloc(size);
    if (new_argv == NULL)
        return argv;
    args_mem = new_argv;

    /* Copy over the strings and set up the pointer table. */
    s = (char*) &new_argv[argc + 1];
    for (i = 0; i < argc; i++) {
        size = strlen(argv[i]) + 1;
        memcpy(s, argv[i], size);
        new_argv[i] = s;
        s += size;
    }
    new_argv[i] = NULL;

    return new_argv;
}

int proct_get_process_title(char* buffer, size_t size)
{
    if (process_title.len > 0)
        strncpy(buffer, process_title.str, size);
    else if (size > 0)
        buffer[0] = '\0';

    return 0;
}


int proct_set_process_title(const char* title)
{
    if (process_title.len == 0)
    return 0;

    /* No need to terminate, byte after is always '\0'. */
    strncpy(process_title.str, title, process_title.len);
    cm_set_process_title(title);

    return 0;
}


void proct_free_args_mem(void)
{
    cm_free(args_mem);
    args_mem = NULL;
}



