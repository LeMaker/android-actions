#ifndef _HDE_CORE_H_
#define _HDE_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum HDE_REG_NO
{
    HDE_REG0 = 0,    
    HDE_REG1 ,  HDE_REG2,  HDE_REG3,  HDE_REG4,  HDE_REG5,  HDE_REG6,  HDE_REG7,  HDE_REG8, 
    HDE_REG9 , HDE_REG10, HDE_REG11, HDE_REG12, HDE_REG13, HDE_REG14, HDE_REG15, HDE_REG16,
    HDE_REG17, HDE_REG18, HDE_REG19, HDE_REG20, HDE_REG21, HDE_REG22, HDE_REG23, HDE_REG24,
    HDE_REG25, HDE_REG26, HDE_REG27, HDE_REG28, HDE_REG29, HDE_REG30, HDE_REG31, HDE_REG32,
    HDE_REG33, HDE_REG34, HDE_REG35, HDE_REG36, HDE_REG37, HDE_REG38, HDE_REG39, HDE_REG40,          
    HDE_REG41 ,HDE_REG42, HDE_REG43, HDE_REG44, HDE_REG45, HDE_REG46, HDE_REG47, HDE_REG48,
    HDE_REG49, HDE_REG50, HDE_REG51, HDE_REG52, HDE_REG53, HDE_REG54, HDE_REG55, HDE_REG56,
    HDE_REG57, HDE_REG58, HDE_REG59, HDE_REG60, HDE_REG61, HDE_REG62, HDE_REG63, HDE_REG64,
    HDE_REG65, HDE_REG66, HDE_REG67, HDE_REG68, HDE_REG69, HDE_REG70, HDE_REG71, HDE_REG72,  
    HDE_REG73, HDE_REG74, HDE_REG75, HDE_REG76, HDE_REG77, HDE_REG78, HDE_REG79, HDE_REG80,
    HDE_REG81, HDE_REG82, HDE_REG83, HDE_REG84, HDE_REG85, HDE_REG86, HDE_REG87, HDE_REG88,
    HDE_REG89, HDE_REG90, HDE_REG91, HDE_REG92, HDE_REG93, HDE_REG94, HDE_REG_MAX
} HDE_RegNO_t;   


#define MAX_HDE_REG_NUM         (HDE_REG_MAX+1)


// 作为一个backdoor, 提供额外的参数输入接口, 使用方法和配置寄存器无异
#define CODEC_CUSTOMIZE_ADDR            (HDE_REG_MAX)
#define CODEC_CUSTOMIZE_VALUE_PERFORMANCE  0x00000001
#define CODEC_CUSTOMIZE_VALUE_LOWPOWER     0x00000002
#define CODEC_CUSTOMIZE_VALUE_DROPFRAME    0x00000004
#define CODEC_CUSTOMIZE_VALUE_MAX          0xffffffff


typedef enum HDE_STATUS
{
    HDE_STATUS_IDLE                 = 0x1,   
    HDE_STATUS_READY_TO_RUN,                // 当前instance已经执行run, 但hde被其他instance占用
    HDE_STATUS_RUNING,                      // 正在运行
    HDE_STATUS_GOTFRAME,                    // 有帧输出
    HDE_STATUS_JPEG_SLICE_READY     = 0x100, // JPEG 解码一个slice完成, 此时不能被其他instance打断，直到GOTFRAME时才可以被打断
    HDE_STATUS_DIRECTMV_FULL,               // h264 Direct mv buffer不够用,需要重新申请再启动解码    
    HDE_STATUS_STREAM_EMPTY,                // 码流消耗完，需要继续配置数据再启动HDE, 5202不允许出现此情况     
    HDE_STATUS_ASO_DETECTED,                // 检测到h264 ASO, 需要软件做墒解码再启动hde, 5202不允许出现此情况
    HDE_STATUS_TIMEOUT              = -1,   // timeout
    HDE_STATUS_STREAM_ERROR         = -2,   // 码流出错        
    HDE_STATUS_BUS_ERROR            = -3,   // 访问ddr出错, 可能是因为配置的非物理连续内存
    HDE_STATUS_DEAD                 = -4,   // vpx挂了，无法配置任何寄存器, video中间件需要关闭所有instance    
    HDE_STATUS_UNKNOWN_ERROR        = -0x100       // 其他错误        
} HDE_Status_t;


typedef struct hde_handle 
{    
    // 读寄存器
    unsigned int (*readReg)(struct hde_handle*, HDE_RegNO_t);
    
    // 写寄存器, 状态寄存器(reg1)由驱动统一管理, 不能写, 返回-1；
    int (*writeReg)(struct hde_handle*, HDE_RegNO_t, const unsigned int);

    // 启动解码, 返回-1，表示hde状态错误，不能启动;
    int (*run)(struct hde_handle*);
    
    // 查询HDE状态，不阻塞版本，hde正在运行返回HDE_STATUS_RUNING
    int (*query)(struct hde_handle*, HDE_Status_t*);    
    
    // 查询HDE状态, 阻塞版本, 直到HDE_STATUS_DEAD或者HDE中断产生, 返回值见HDE_Status_t
    int (*query_timeout)(struct hde_handle*, HDE_Status_t*);    
    
    // 将状态转为idle
    int (*reset)(struct hde_handle*);   
    
} hde_handle_t;

// 获取句柄. 参数错误或者达到最大的运行instance个数，返回NULL;     
hde_handle_t *hde_getHandle(void);

// 关闭句柄    
void hde_freeHandle(struct hde_handle*);

// DEBUG, 打开内部打印
void hde_enable_log(void);

// DEBUG, 关闭内部打印
void hde_disable_log(void);

// DEBUG 获取当前运行的instance信息和所有寄存器信息, 单次有效
void hde_dump_info(void);



/**********************************************************
设计流程:  vd_h264.so  ----> libhde_core.so ----> hde_drv.ko

限制条件： 不能跨进程使用，即不同instance必须是在同一个进程中。

使用方法：
    Android.mk中增加 LOCAL_SHARED_LIBRARIES := libhde_core
    编译时会自动连接libhde_core.so, 就可以自动加载so, 使用api函数, 不需要直接调用hde_drv.ko

额外说明:  reg1(status寄存器）不能写

Example code：

    int hde_close(void *codec_handle)
    {
        // ...
        hde_freeHandle(codec_handle->hde_handle);
        codec_handle->hde_handle = NULL;
        // ...        
    }
    
    int hde_init(void *codec_handle)
    {
        codec_handle->hde_handle == hde_getHandle();
        if(codec_handle->hde_handle == NULL) return -1;
        
        if(DEBUG)
            hde_enable_log();           
        
        // if you need to know about overload of HDE;
        hde_dump_info();
                
        return 0;
    }
    
    int hde_decode_one_frame(void *codec_handle)
    {
        int rt;        
        unsigned int value;
        int status;
        
        hde_handle_t *hde = codec_handle->hde_handle;                
                      
        hde->reset(hde);      
        
        value = hde->readReg(hde, REG10);                
        value &= 0x2;
        
        rt = hde->writeReg(hde, REG10, value);
        if(rt) goto SOMETHING_WRONG;
        
        rt = hde->run(hde);
        if(rt) goto SOMETHING_WRONG;          
                        
#if USE_QUERY 
        // 死查，效率低    
        while(timeout_ms < 10000)        
            rt = hde->query(hde, &status);
            if(rt) return -1;
            
            if(status != HDE_STATUS_RUNING && status != HDE_STATUS_IDLE) 
                break;               
        }
#else                
        // dosomthing else, 再来查，内部有任务调度，可提高cpu利用率
        rt = hde->query_timeout(hde, &status);
        if(rt) goto SOMETHING_WRONG;

#endif
        
        if(status == HDE_STATUS_GOTFRAME) {                
            return 0;
        } else {
            goto SOMETHING_WRONG;
        }           
           
SOMETHING_WRONG:
        
        if(status == HDE_STATUS_DEAD) {
            ACTAL_ERROR("HDE Died");            
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

#endif//_HDE_CORE_H_

