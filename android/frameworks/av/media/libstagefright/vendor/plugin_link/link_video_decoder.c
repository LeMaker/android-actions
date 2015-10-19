extern int videodec_plugin_info;

void *get_plugin_info(void)
{
    return (void *)&videodec_plugin_info;
}

#ifdef _OS_UC_1100_  
#include <actal_posix_dev.h>

int __attribute__((constructor)) so_init(void)
{
	return 0;
}

int __attribute__((destructor)) so_exit(void)
{
	return 0;
}

DL_EXPORT_SYMBOL(get_plugin_info);
#endif // _OS_UC_1100_
