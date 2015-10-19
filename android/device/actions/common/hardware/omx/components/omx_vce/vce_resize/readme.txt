本代码实现yuvsp格式的下采样功能，主要用于4kx2k的降2下采样。

规格说明如下：

1.  代码直接调用vcedrv，支持多instances，支持gl5202/gl5203/gl5207/gl5202c;

2.  gl5202和gl5203支持降1、2、4、8倍下采样；gl5027(gl5202c与之相同)支持降1、2倍下采样；

3.  输入buffer为yuv420sp格式，输入宽高须8像素对齐（stride须16像素对齐），输出宽高须16像素对齐（其中gl5207/gl5202c的输出宽须32像素对齐）；

4.  把附件解压到：~android\device\actions\hardware\omx\components\omx_vce\目录下 编译。
    默认为gl5207((gl5202c)配置。要用于gl5202/gl5203需要修改Android.mk和vce_cfg.h。

5.  可以通过编译动态库或静态库来调用，修改一下Android.mk即可。

6.  封装了三个接口，以供给上层调用（例子见main.c测试程序）：
void* VceReSize_Open(void * input/*VR_Input_t*/);
int   VceReSize_Run(void* handle,void* param/*VR_Parm_t*/);
void  VceReSize_Close(void* handle);
