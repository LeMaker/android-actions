#include <ctype.h>
#include <limits.h>
#include "xpl_Regex.h"


#ifdef __cplusplus  
extern "C" {
#endif

BOOLEAN XPL_RG_Comp(CPCHAR pattern, CPCHAR str)
{
    int status;
    regex_t re;

    status = regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB);
    if ( status == 0 )
    {
        status = regexec(&re, str, (size_t) 0, XPL_NULL, 0);
    }    
    regfree(&re);
    if ( status != 0 )
        return FALSE;
    else
        return TRUE;
}


#ifdef __cplusplus  
}
#endif

