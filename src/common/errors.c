#include "common.h"
#include "errors.h"


static const char* er_unknown_err()
{
    char *err = "Unknown system error";
    return err;
}


#define ER_ERR_NAME_GEN(name, _) case E##name : return #name;
const char* er_err_name(int err)
{
    switch(err) 
    {
        ER_ERRNO_MAP(ER_ERR_NAME_GEN)
    }
    return er_unknown_err();
}
#undef ER_ERR_NAME_GEN


#define ER_STRERROR_GEN(name, msg) case E##name : return msg;
const char* er_strerror(int err)
{
    switch(err)
    {
        ER_ERRNO_MAP(ER_STRERROR_GEN)
    }
    return er_unknown_err();
}
#undef ER_STRERROR_GEN



















  









