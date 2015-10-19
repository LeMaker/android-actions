#define TS_FEC_PACKET_SIZE 204
#define TS_DVHS_PACKET_SIZE 192
#define TS_PACKET_SIZE 188
#define TS_MAX_PACKET_SIZE 204

#define NULL 0

#include "format_check.h"

static int analyze(const unsigned char *buf, int size, int packet_size, int *index){
    int stat[TS_MAX_PACKET_SIZE];
    int i;
    int x=0;
    int best_score=0;

    memset(stat, 0, packet_size*sizeof(int));

    for(x=i=0; i<size-3; i++){
        if(buf[i] == 0x47 && !(buf[i+1] & 0x80) && (buf[i+3] & 0x30)){
            stat[x]++;
            if(stat[x] > best_score){
                best_score= stat[x];
                if(index) *index= x;
            }
        }

        x++;
        if(x == packet_size) x= 0;
    }

    return best_score;
}

/* autodetect fec presence. Must have at least 1024 bytes  */
static int get_packet_size(const unsigned char *buf, int size ,int * index)
{
    int score, fec_score, dvhs_score;
	int index1 = 0,index2 = 0,index3 = 0;
    if (size < (TS_FEC_PACKET_SIZE * 5 + 1))
        return -1;

    score    = analyze(buf, size, TS_PACKET_SIZE, &index1);
    dvhs_score    = analyze(buf, size, TS_DVHS_PACKET_SIZE, &index2);
    fec_score= analyze(buf, size, TS_FEC_PACKET_SIZE, &index3);
   //printf("score: %d, dvhs_score: %d, fec_score: %d \n", score, dvhs_score, fec_score);

    if(score > fec_score && score > dvhs_score){
		if(index) *index = index1;
		return TS_PACKET_SIZE;
	}else if(dvhs_score > score && dvhs_score > fec_score){
		if(index) *index = index2;
		return TS_DVHS_PACKET_SIZE;
    }else if(score < fec_score && dvhs_score < fec_score){
    	if(index) *index = index3;
		return TS_FEC_PACKET_SIZE;
    }
    else 
		return -1;
}

int ts_check(storage_io_t *input)
{
	unsigned char tsbuf[4096];
	int tssize = 0;
	int readnum;
	int index = 0 ,i = 0;
	input->seek(input, 0, SEEK_SET);
	readnum = input->read(tsbuf, 1, 4096, input);
	if (readnum != 4096)
		return 0;
	tssize = get_packet_size(tsbuf, 4096,&index);
	actal_error("tssize %d,index %d",tssize,index);
	if (tssize != -1){
		//ActionsCode(author:rongxing, add judgement condition for format recognition of ts files)
		for(i =index ;i<4096;i+=tssize){
			if(tsbuf[i] != 0x47){
				actal_error("tsbuf[i] %d,i %d",tsbuf[i],i);
				return 0;
			}
		}
		if ((tsbuf[0] == 'B')&&(tsbuf[1] == 'M')){  /*ActionsCode(author:sunchengzhi, bugfix BUG00124842 for bmp being recognized as ts format)*/
			return 0;
		}
		return 1;
	}
	return 0;
}

