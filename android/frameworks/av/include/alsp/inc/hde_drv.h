

#ifdef __cplusplus
extern "C" {
#endif
#define HDE_MAGIC_NUMBER                'h'
/******************************************************************************/

#define HDE_QUERY                 	_IOWR(HDE_MAGIC_NUMBER, 0xf0, unsigned int)
#define HDE_ENABLE_CLK             	_IOWR(HDE_MAGIC_NUMBER, 0xf1, unsigned int)
#define HDE_DISABLE_CLK            	_IOWR(HDE_MAGIC_NUMBER, 0xf2, unsigned int)
#define HDE_RUN                    	_IOWR(HDE_MAGIC_NUMBER, 0xf3, unsigned int)
#define HDE_DUMP                 		_IOWR(HDE_MAGIC_NUMBER, 0xf4,unsigned int)
#define HDE_SET_FREQ                _IOWR(HDE_MAGIC_NUMBER, 0xf5, unsigned int)
#define HDE_GET_FREQ                _IOWR(HDE_MAGIC_NUMBER, 0xf6,unsigned int)
#define HDE_SET_MULTI               _IOWR(HDE_MAGIC_NUMBER, 0xf7,unsigned int)



#ifdef __cplusplus
}
#endif
