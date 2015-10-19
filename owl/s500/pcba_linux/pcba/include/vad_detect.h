
#ifndef VAD_DETECT_H
#define VAD_DETECT_H

typedef struct VAD_State
{
	int   vad_flag;             //0表示当前是无声段，1表示当前是有声段

    int   vad_threshold;        //声控或分曲模式中的静音检测门限值  
	
	int   vad_delay;            //声控或分曲模式中的静音检测延迟时间

	int   threshold_mode;       //确定门限值的方式，为0表示由AP设定，为1表示取开始16帧能量值做平均

	int   frame_count;          //记录当前帧数
	int   noise_count;
	int   delay_count;

	int   threshold_updata;  //判定为无声段时，是否需要进行噪声能量更新

	int   voice_judge;          

}VAD_State;

void vad_init(VAD_State * vad_ptr);
void set_vad_delay(VAD_State * vad_ptr, int time, int rate, int frame_size);
void set_vad_threshold(VAD_State * vad_ptr, int level);
void get_VAD_Flag(VAD_State * vad_ptr, int energy);


#endif

