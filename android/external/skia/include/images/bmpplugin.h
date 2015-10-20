#ifdef __cplusplus
extern "C"
{
#endif

#define SEEK_SET 0
#define SEEK_CUR 1
#define NULL 0

#define BI_RGB 0
#define BI_RLE8 1
#define BI_RLE4 2
#define BI_BITFIELDS 3
#define MAX_BMP_SIZE
typedef unsigned char UINT8;
typedef struct bmp_head_s
{
	//BITMAPFILEHEADER
	unsigned char        bfType[2];							//"bm"
	unsigned long        bfSize;							//filesize
	unsigned int         bfReserved1;						
	unsigned int         bfReserved2;
	unsigned long        ccNNNoffset_bits_Img;				//numbers bytes front image bytes  
	//BITMAPINFOHEADER
	unsigned long	     headerSize_Img;					//40
	unsigned long	     image_w;							
	//add 11-13 maybe < 0;
		 long 	     image_h;
	unsigned char	     flag_swap;

	unsigned int	     input_iMCU_rowNNNbiPlanes_Img;		//planes:1
	unsigned int	     bits_per_pixel_Img;				//pixel:1,4,8,24
	unsigned long	     cNNNbiCompression_Img;				//0-none,1-rle 8,2-rle 4,3-bitfields
	unsigned long	     sNNNimage_Size_Img;				//bmp data,in 4 bytes boundary
	unsigned long	     rNNNbiXPelsPerMeter_Img;			//hresolution
	unsigned long	     lookNNNbiYPelsPerMeter_Img;		//vresolution
	unsigned long	     yNNNbiClrUsed_Img;					//number of colors
	unsigned long	     cbNNNcolors_important_Img;	
//add 12-5
	unsigned long		 image_w_out;
	unsigned long		 image_h_out;
	unsigned int		 offset_w;
	unsigned int		 offset_h;
//add 01/23/2007
	int					 flag_os2;
//add 01/25/2007
	unsigned int		 read_byte_for_head;
}bmp_head_s_t;

typedef struct bmp_struct_def
{
	int  (*read)(struct bmp_struct_def *bmp_ptr,unsigned char *buf,unsigned int len);
	void (*seek)(struct bmp_struct_def *bmp_ptr,unsigned int offset, int seek_flag);
	void *io_ptr;
}bmp_struct_def_t;

typedef int  (*bmp_rw_ptr)(struct bmp_struct_def *,unsigned char *,unsigned int);
typedef void (*bmp_seek_ptr)(struct bmp_struct_def *,unsigned int,int);

extern struct bmp_head_s* bmp_creat_header();
extern struct bmp_struct_def* bmp_creat_ptr();
extern void bmp_destory_ptr(struct bmp_struct_def *bmp_ptr, struct bmp_head_s *bmp_header);
extern void bmp_set_read_fn(struct bmp_struct_def *bmp_ptr, void *io_ptr, bmp_rw_ptr read_data_fn);
extern void bmp_set_seek_fn(struct bmp_struct_def *bmp_ptr, bmp_seek_ptr seek_data_fn);
extern int decode_bmp(struct bmp_head_s *bmp_h,struct bmp_struct_def *bmp_ptr,unsigned char *b);
extern int read_input_bmp_header(struct bmp_head_s *bmp_h,struct bmp_struct_def *bmp_ptr);
extern int swap(unsigned char *swapbuf, int width, int height,int n);
#ifdef __cplusplus
}
#endif