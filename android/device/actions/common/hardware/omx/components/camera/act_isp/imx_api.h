#ifndef __IMX_API_H__
#define __IMX_API_H__

/*
* 
*/
typedef struct{
    int enable;          // 1: enable, or disable
    int level;           // 1: r=0.05,d=3; 2: r=0.05,d=5; 3: r=0.15,d=3; 4: r=0.15,d=5
    int bl_size;         // 1: 3x3; 2: 5x5; 3: 7x7; 4: 9x9
}imx_nr_t;

/*
* 
*/
typedef struct{
    float k1;         // edge enhancement:
    float k2;         // low:    k1 = 0.000f; k2 = 0.100f; k3 = 0.060f; t1 = 10; t2 = 12;
    float k3;         // mid:   k1 = 0.010f; k2 = 0.120f; k3 = 0.080f; t1 = 9; t2 = 15;
    int t1;           // high:  k1 = 0.050f; k2 = 0.200f; k3 = 0.120f; t1 = 8; t2 = 16;
    int t2;
}imx_ee_t;

enum{
    YUV420SP,
    YUV420P
};

/*
* 
*/
typedef struct{
    void *ptr[3];         // physical address, aligned by 64 bits
    int format;           // 0: yuv420sp; 1: yuv420p
    int canvas_w;        // 16 pixels aligned
    int canvas_h;
    int xoffset;
    int yoffset;
    int pic_w;           // 8 pixels aligned
    int pic_h;          // 2 pixels aligned
}imx_img_t;

typedef struct{
    imx_img_t *in;
    imx_img_t *out;
}imx_imgio_t;

enum{
    IMX_SET_NR,     /*imx_nr_t*/
    IMX_SET_EE,     /*imx_ee_t*/
    IMX_PROCESS_ONE,        /*imx_imgio_t*/
    IMX_QUERY_FINISHED,      /*NULL*/
};

void *imx_open(void);
int imx_cmd(void *handle, int id, void *arg);
int imx_close(void *handle);

int imx_process(void *handle, imx_img_t *imgin, imx_img_t *imgout, imx_nr_t *nr_para, imx_ee_t *ee_para);


#endif