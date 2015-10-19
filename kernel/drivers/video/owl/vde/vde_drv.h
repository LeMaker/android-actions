
#ifdef __cplusplus
extern "C" {
#endif
#define VDE_MAGIC_NUMBER                'v'
/******************************************************************************/

#define VDE_QUERY                        _IOWR(VDE_MAGIC_NUMBER, 0xf0, unsigned int)
#define VDE_ENABLE_CLK                   _IOWR(VDE_MAGIC_NUMBER, 0xf1, unsigned int)
#define VDE_DISABLE_CLK                  _IOWR(VDE_MAGIC_NUMBER, 0xf2, unsigned int)
#define VDE_RUN                     		_IOWR(VDE_MAGIC_NUMBER, 0xf3, unsigned int)
#define VDE_DUMP                    		_IOWR(VDE_MAGIC_NUMBER, 0xf4, unsigned int)
#define VDE_SET_FREQ                     _IOWR(VDE_MAGIC_NUMBER, 0xf5, unsigned int)
#define VDE_GET_FREQ                    _IOWR(VDE_MAGIC_NUMBER, 0xf6, unsigned int)
#define VDE_SET_MULTI                   _IOWR(VDE_MAGIC_NUMBER, 0xf7, unsigned int)


#ifdef __cplusplus
}
#endif
