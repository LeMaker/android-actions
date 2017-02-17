/**
 * wrapper for libcidop
 */

extern unsigned int read_current_uref(void);

unsigned int read_current_uref2(void)
{
	read_current_uref();
}
EXPORT_SYMBOL_GPL(read_current_uref2);