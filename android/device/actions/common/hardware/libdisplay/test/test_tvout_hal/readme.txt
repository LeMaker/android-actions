test_tvout_hal使用说明
    1.test_tvout_hal主要是对tvout输出管理的hal层进行测试，主要涉及到libcvbs,libhdmi,libdisplay三个驱动的hal层。
    2.test_tvout_hal的使用方法如下：
        (1)使用时将本测试用例所生成的二进制文件test_tvout_hal(所处位置参看Android.mk)通过adb push
        到小机。
        (2)进入到小机中test_tvout_hal文件做在目录，执行./test_tvout_hal 参数1 参数2 参数3 参数4,其中各个参数用空格间隔。
        (3)各参数说明
            参数1表示CVBS的制式，值为 "pal" 或 "ntsc";
            参数2表示HDMI的video id，值参考附录；
            参数3表示display mode,值参考附录；
            参数4表示显示设备，值为cvbs_lcd0 或 hdmi_lcd0 或 lcd0
            注：若某参数不设置则用“！”代替
        (4)举例
           a.假设要测试CVBS的pal制式与lcd同显，且display mode为0,则：./test_tvout_hal pal ！ 0 cvbs_lcd0;
           b.假设要测试HDMI的vid为86的分辨率与lcd同显，且display mode为0,则：./test_tvout_hal ！86 0 hdmi_lcd0;
           c.如果要设置lcd单显，则:./test_tvout_hal ! ! ! lcd0.
    3.附录
        (1)display mode值
         MODE_DISP_SYNC_DEFAULT_TV_GV_LCD_GV:0
 
        MODE_DISP_DOUBLE_DEFAULT_NO_SYNC_TV_GV_LCD_GV:1
        MODE_DISP_DOUBLE_DEFAULT_NO_SYNC_TV_GV_LCD_G:17
        MODE_DISP_DOUBLE_DEFAULT_NO_SYNC_TV_V_LCD_G:33

       MODE_DISP_DOUBLE_DEFAULT_SYNC_EXT_TV_GV_LCD_GV:3
       MODE_DISP_DOUBLE_DEFAULT_SYNC_EXT_TV_GV_LCD_G:19
       MODE_DISP_DOUBLE_DEFAULT_SYNC_EXT_TV_V_LCD_G:35
       
       (2)hdmi vid值
       VID640x480P_60_4VS3 : 1	
       VID720x480P_60_4VS3 : 2	
       VID720x480P_60_16VS9  : 3	
       VID1280x720P_60_16VS9 ：4	
       VID1920x1080I_60_16VS9 ：5	
       VID720x480I_60_4VS3 ：6
       VID720x480I_60_16VS9 ：7
       VID1440x480P_60_4VS3 : 14	
       VID1440x480P_60_16VS9 : 15	
       VID1920x1080P_60_16VS9 : 16	
       VID720x576P_50_4VS3:17
       VID720x576P_50_16VS9 : 18
       VID1280x720P_50_16VS9  : 19	
       VID1920x1080I_50_16VS9 : 20	
       VID720x576I_50_4VS3 : 21
       VID720x576I_50_16VS9 : 22
       VID1440x576P_50_4VS3 : 29	
       VID1440x576P_50_16VS9 : 30	
       VID1920x1080P_50_16VS9 : 31	
       VID1920x1080P_24_16VS9 : 32	
       VID1920x1080P_25_16VS9 : 33	
       VID1920x1080P_30_16VS9 : 34	
       VID720x480P_59P94_4VS3 : 72	
       VID1280X720P_59P94_16VS9 : 74	
           