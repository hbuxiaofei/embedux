#include "loop.h"
#include "queue.h"
#include "parser.h"
#include "mlog.h"
#include "threads.h"


int parser_start(parser_t *parser)
{
    int err = 0;
    dictionary *ini;

    if (!parser || !parser->path) {
        MLOG_ERROR("param error !\n");
        err = -1;
    }

    if (err == 0) {
        ini = iniparser_load(parser->path);
        if (!ini) {
            MLOG_ERROR("cannot parse file :%s !\n", parser->path);
            err = -1 ;
        }
        
        if (err == 0) {
            parser->ini = ini;
            err = trd_rwlock_init(&parser->lock);
        }
    }

    return err;
}

int parser_exit(parser_t *parser)
{
    if (!parser || !parser->ini) {
        MLOG_ERROR("param error !\n");
        return -1;
    }

    trd_rwlock_wrlock(&parser->lock);
    
    iniparser_freedict(parser->ini);    
    parser->ini = NULL;
    
    trd_rwlock_wrunlock(&parser->lock);

    return 0;
}

char *parser_getstr(parser_t *parser, const char *key, char *def)
{
    int err = 0;
    char *str = def;
    
    if (!parser || !parser->ini || !key) {
        MLOG_ERROR("param error !\n");
        err = -1;
    }

    if (err == 0) {
        trd_rwlock_rdlock(&parser->lock);
        str = iniparser_getstring(parser->ini, key, def);
        trd_rwlock_rdunlock(&parser->lock);
    }

    return str;
}


int parser_getint(parser_t *parser, const char *key, int def)
{
    int err = def;
    
    if (!parser || !parser->ini || !key) {
        MLOG_ERROR("param error !\n");
        err = -1;
    }

    if (err == 0) {
        trd_rwlock_rdlock(&parser->lock);
        err = iniparser_getint(parser->ini, key, def);
        trd_rwlock_rdunlock(&parser->lock);
    }

    return err;
}

double parser_getdouble(parser_t *parser, const char *key, double def)
{
    int err = def;
    double dub = def;
    
    if (!parser || !parser->ini || !key) {
        MLOG_ERROR("param error !\n");
        err = -1;
    }

    if (err == 0) {
        trd_rwlock_rdlock(&parser->lock);
        dub = iniparser_getdouble(parser->ini, key, def);
        trd_rwlock_rdunlock(&parser->lock);
    }

    return dub;
}

int parser_setstr(parser_t *parser, const char *entry, const char *val)
{
    int err = 0;
    
    if (!parser || !parser->ini || !entry || !val) {
        MLOG_ERROR("param error !\n");
        err = -1;
    }

    if (err == 0) {
        trd_rwlock_wrlock(&parser->lock);
        err = iniparser_set(parser->ini, entry, val);
        trd_rwlock_wrunlock(&parser->lock);

    }

    return err;
}

int parser_fflush(parser_t *parser)
{
    FILE *fd = NULL;
    int err = 0;
    
    if (!parser || !parser->path || !parser->ini) {
        MLOG_ERROR("param error !\n");
        err = -1;
    }

    trd_rwlock_wrlock(&parser->lock);

    if (err == 0) {
    	fd = fopen(parser->path, "w");        
        if (!fd) {
            MLOG_ERROR("error !\n");
            err = -1;
        }
    }

    if (err == 0) {
    	iniparser_dump_ini(parser->ini, fd);
	    fclose(fd);
    }

    trd_rwlock_wrunlock(&parser->lock);
    
    return err;
}

