#ifndef _VDE_CORE_H_
#define _VDE_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum VDE_REG_NO
{
    VDE_REG0 = 0,    
    VDE_REG1 ,  VDE_REG2,  VDE_REG3,  VDE_REG4,  VDE_REG5,  VDE_REG6,  VDE_REG7,  VDE_REG8, 
    VDE_REG9 , VDE_REG10, VDE_REG11, VDE_REG12, VDE_REG13, VDE_REG14, VDE_REG15, VDE_REG16,
    VDE_REG17, VDE_REG18, VDE_REG19, VDE_REG20, VDE_REG21, VDE_REG22, VDE_REG23, VDE_REG24,
    VDE_REG25, VDE_REG26, VDE_REG27, VDE_REG28, VDE_REG29, VDE_REG30, VDE_REG31, VDE_REG32,
    VDE_REG33, VDE_REG34, VDE_REG35, VDE_REG36, VDE_REG37, VDE_REG38, VDE_REG39, VDE_REG40,          
    VDE_REG41 ,VDE_REG42, VDE_REG43, VDE_REG44, VDE_REG45, VDE_REG46, VDE_REG47, VDE_REG48,
    VDE_REG49, VDE_REG50, VDE_REG51, VDE_REG52, VDE_REG53, VDE_REG54, VDE_REG55, VDE_REG56,
    VDE_REG57, VDE_REG58, VDE_REG59, VDE_REG60, VDE_REG61, VDE_REG62, VDE_REG63, VDE_REG64,
    VDE_REG65, VDE_REG66, VDE_REG67, VDE_REG68, VDE_REG69, VDE_REG70, VDE_REG71, VDE_REG72,  
    VDE_REG73, VDE_REG74, VDE_REG75, VDE_REG76, VDE_REG77, VDE_REG78, VDE_REG79, VDE_REG80,
    VDE_REG81, VDE_REG82, VDE_REG83, VDE_REG84, VDE_REG85, VDE_REG86, VDE_REG87, VDE_REG88,
    VDE_REG89, VDE_REG90, VDE_REG91, VDE_REG92, VDE_REG93, VDE_REG94, VDE_REG_MAX
} VDE_RegNO_t;   


#define MAX_VDE_REG_NUM         (VDE_REG_MAX+1)


// 作为一个backdoor, 提供额外的参数输入接口, 使用方法和配置寄存器无异
#define CODEC_CUSTOMIZE_ADDR            (VDE_REG_MAX)
#define CODEC_CUSTOMIZE_VALUE_PERFORMANCE  0x00000001
#define CODEC_CUSTOMIZE_VALUE_LOWPOWER     0x00000002
#define CODEC_CUSTOMIZE_VALUE_DROPFRAME    0x00000004
#define CODEC_CUSTOMIZE_VALUE_MAX          0xffffffff


typedef enum VDE_STATUS
{
    VDE_STATUS_IDLE                 = 0x1,   
    VDE_STATUS_READY_TO_RUN,                // 当前instance已经执行run, 但vde被其他instance占用
    VDE_STATUS_RUNING,                      // 正在运行
    VDE_STATUS_GOTFRAME,                    // 有帧输出
    VDE_STATUS_JPEG_SLICE_READY     = 0x100, // JPEG 解码一个slice完成, 此时不能被其他instance打断，直到GOTFRAME时才可以被打断
    VDE_STATUS_DIRECTMV_FULL,               // h264 Direct mv buffer不够用,需要重新申请再启动解码    
    VDE_STATUS_STREAM_EMPTY,                // 码流消耗完，需要继续配置数据再启动VDE, 5202不允许出现此情况     
    VDE_STATUS_ASO_DETECTED,                // 检测到h264 ASO, 需要软件做墒解码再启动vde, 5202不允许出现此情况
    VDE_STATUS_TIMEOUT              = -1,   // timeout
    VDE_STATUS_STREAM_ERROR         = -2,   // 码流出错        
    VDE_STATUS_BUS_ERROR            = -3,   // 访问ddr出错, 可能是因为配置的非物理连续内存
    VDE_STATUS_DEAD                 = -4,   // vpx挂了，无法配置任何寄存器, video中间件需要关闭所有instance    
    VDE_STATUS_UNKNOWN_ERROR        = -0x100       // 其他错误        
} VDE_Status_t;


typedef struct vde_handle 
{    
    // 读寄存器
    unsigned int (*readReg)(struct vde_handle*, VDE_RegNO_t);
    
    // 写寄存器, 状态寄存器(reg1)由驱动统一管理, 不能写, 返回-1；
    int (*writeReg)(struct vde_handle*, VDE_RegNO_t, const unsigned int);

    // 启动解码, 返回-1，表示vde状态错误，不能启动;
    int (*run)(struct vde_handle*);
    
    // 查询VDE状态，不阻塞版本，vde正在运行返回VDE_STATUS_RUNING
    int (*query)(struct vde_handle*, VDE_Status_t*);    
    
    // 查询VDE状态, 阻塞版本, 直到VDE_STATUS_DEAD或者VDE中断产生, 返回值见VDE_Status_t
    int (*query_timeout)(struct vde_handle*, VDE_Status_t*);    
    
    // 将状态转为idle
    int (*reset)(struct vde_handle*);   
    
} vde_handle_t;

// 获取句柄. 参数错误或者达到最大的运行instance个数，返回NULL;     
vde_handle_t *vde_getHandle(void);

// 关闭句柄    
void vde_freeHandle(struct vde_handle*);

// DEBUG, 打开内部打印
void vde_enable_log(void);

// DEBUG, 关闭内部打印
void vde_disable_log(void);

// DEBUG 获取当前运行的instance信息和所有寄存器信息, 单次有效
void vde_dump_info(void);



/**********************************************************
设计流程:  vd_h264.so  ----> libvde_core.so ----> vde_drv.ko

限制条件： 不能跨进程使用，即不同instance必须是在同一个进程中。

使用方法：
    Android.mk中增加 LOCAL_SHARED_LIBRARIES := libvde_core
    编译时会自动连接libvde_core.so, 就可以自动加载so, 使用api函数, 不需要直接调用vde_drv.ko

额外说明:  reg1(status寄存器）不能写

Example code：

    int vde_close(void *codec_handle)
    {
        // ...
        vde_freeHandle(codec_handle->vde_handle);
        codec_handle->vde_handle = NULL;
        // ...        
    }
    
    int vde_init(void *codec_handle)
    {
        codec_handle->vde_handle == vde_getHandle();
        if(codec_handle->vde_handle == NULL) return -1;
        
        if(DEBUG)
            vde_enable_log();           
        
        // if you need to know about overload of VDE;
        vde_dump_info();
                
        return 0;
    }
    
    int vde_decode_one_frame(void *codec_handle)
    {
        int rt;        
        unsigned int value;
        int status;
        
        vde_handle_t *vde = codec_handle->vde_handle;                
                      
        vde->reset(vde);      
        
        value = vde->readReg(vde, REG10);                
        value &= 0x2;
        
        rt = vde->writeReg(vde, REG10, value);
        if(rt) goto SOMETHING_WRONG;
        
        rt = vde->run(vde);
        if(rt) goto SOMETHING_WRONG;          
                        
#if USE_QUERY 
        // 死查，效率低    
        while(timeout_ms < 10000)        
            rt = vde->query(vde, &status);
            if(rt) return -1;
            
            if(status != VDE_STATUS_RUNING && status != VDE_STATUS_IDLE) 
                break;               
        }
#else                
        // dosomthing else, 再来查，内部有任务调度，可提高cpu利用率
        rt = vde->query_timeout(vde, &status);
        if(rt) goto SOMETHING_WRONG;

#endif
        
        if(status == VDE_STATUS_GOTFRAME) {                
            return 0;
        } else {
            goto SOMETHING_WRONG;
        }           
           
SOMETHING_WRONG:
        
        if(status == VDE_STATUS_DEAD) {
            ACTAL_ERROR("VDE Died");            
            return -1; //fatal error here.
        } else {
            ACTAL_ERROR("something wrong, check your code")
            return -1;
        }       
                   
    }

**********************************************************/
#ifdef __cplusplus
}
#endif

#endif//_VDE_CORE_H_

