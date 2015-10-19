在debug fs 打开的情况下，在系统下面有个目录 dmm
/sys/kernel/debug/dmm/
  	enable
	master0
	   id 
	   mode
	master1
	   id 
	   mode
	max_statistic_cnt
	result
	sampling_rate
	statistic_cnt
	
enable: 开始或者是停止统计 
      开始统计 echo 1 > enable 
      停止统计 echo 0 > enable 
      
master0 和 master1 分别是需要统计的两个master 信息

id ：在选择ID 的时候 需要根据此值进行设置
	MASTER_ID_CPU = 0,
	MASTER_ID_USB3,
	MASTER_ID_VCE,
	MASTER_ID_ENTHERNET,
	MASTER_ID_USB2,
	MASTER_ID_DE,
	MASTER_ID_GPU3D,
	MASTER_ID_SI,
	MASTER_ID_DMA,
	MASTER_ID_DAP,
	MASTER_ID_ALL,
	MASTER_ID_IDLE,
	
mode ：在选择MODE  的时候 需要根据此值进行设置
	MASTER_MODE_READ = 0,   // 统计读
	MASTER_MODE_WRITE, //统计写
	MASTER_MODE_ALL, //统计读写

可以通过设置来修改需要统计的master 和统计的模式

max_statistic_cnt： 最大能够统计的次数，只要是内存限制。默认是1000次，可以通过echo 2000 > max_statistic_cnt 来修改

sampling_rate：采样的间隔时间， 默认是1s 统计一次，也可以通过echo 2000 > sampling_rate 修改。

statistic_cnt：当前统计了多少次，只读。通过cat statistic_cnt 读取

result:统计的结果，只读 通过cat result 获取
结果说明：
       master id:   mode   bandwidth(M byte)     percent of total(%)        master id:   mode   bandwidth(M byte)    percent of total(%)
             ALL:    RW               400                         13            IDLE:    RW               433                         85 
             ALL:    RW               612                         21            IDLE:    RW              1178                         60 
             ALL:    RW               714                         24            IDLE:    RW               989                         66 
             ALL:    RW               616                         21            IDLE:    RW              1165                         60 
             ALL:    RW               635                         22            IDLE:    RW              1128                         61 
             ALL:    RW               667                         23            IDLE:    RW              1067                         63 
             ALL:    RW               625                         21            IDLE:    RW              1145                         61 
             ALL:    RW               655                         22            IDLE:    RW              1096                         62 
             ALL:    RW               649                         22            IDLE:    RW              1110                         62 
master id : 指统计的是那个master 如： all 表明所有的master ，IDLE 标示统计DDR IDEL 的时间
MODE ： 指是统计的模式， R ，W ， RW 等3中模式
bandwidth：统计的master在指定时间内的带宽，M 为单位 400M byte
percent of total： 统计的百分比，是当前master 的带宽和系统理论总带宽的比， IDLE 是个例外，当统计IDLE的时候，此时的百分比换算为了非IDLE的cycle 和系统DDR总的cycle 之间的比例

使用流程：
1.设置相关参数
2.开始统计 
3.停止统计
4.查看结果
  
  
如：需要统计DE 的带宽和GPU的带宽，采样率是2s采样一次，最大统计次数为 500次
echo 5 > /sys/kernel/debug/dmm/master0/id 
echo 0 > /sys/kernel/debug/dmm/master0/mode 
echo 6 > /sys/kernel/debug/dmm/master1/id
echo 2 > /sys/kernel/debug/dmm/master1/mode

echo 2000 > /sys/kernel/debug/dmm/sampling_rate
echo 500 > /sys/kernel/debug/dmm/max_statistic_cnt

echo 1 > /sys/kernel/debug/dmm/enable
操作UI 一段时间后
。。。。。。
echo 0 > /sys/kernel/debug/dmm/enable

cat /sys/kernel/debug/dmm/result > /data/result.txt
查看结果



默认情况下统计的是 所有master 的带宽和 IDLE 的情况，采样率是1s ，最大统计次数是1000次

如： 
echo 1 > /sys/kernel/debug/dmm/enable
操作UI 一段时间后
。。。。。。
echo 0 > /sys/kernel/debug/dmm/enable

cat /sys/kernel/debug/dmm/result > /data/result.txt  就是按照上面的默认设置进行统计。
