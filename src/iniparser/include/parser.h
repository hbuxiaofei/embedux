#ifndef __PARSER_H__
#define __PARSER_H__

#include <pthread.h>
#include "iniparser.h"
#include "loop.h"

typedef struct parser_s{
    char *path;
    dictionary *ini;
    pthread_rwlock_t lock;
} parser_t;


int parser_start(parser_t *parser);

int parser_exit(parser_t *parser);

char *parser_getstr(parser_t *parser, const char *key, char *def);

int parser_getint(parser_t *parser, const char *key, int def);

double parser_getdouble(parser_t *parser, const char *key, double def);

int parser_setstr(parser_t *parser, const char *entry, const char *val);

int parser_fflush(parser_t *parser);



#endif

