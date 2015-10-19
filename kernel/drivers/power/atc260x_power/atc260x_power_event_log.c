#include <linux/kfifo.h>
#include <linux/time.h>

#define LOG_LINE_MAX_SIZE 48
#define LOG_BUF_SIZE (30*48*512)
static char logbuf[LOG_BUF_SIZE];
static int cur=0;

static int log_event(const char *content, unsigned int len){
    int nextfreepos;
    if(!content){
        return -1;
    }
    nextfreepos = cur+len;
    if(nextfreepos<=LOG_BUF_SIZE){
        memcpy(logbuf+cur, content, len);
        cur=nextfreepos%LOG_BUF_SIZE;
        return 0;
    }
    pr_warn("log_event buffer full");
    memcpy(logbuf+cur, content, LOG_BUF_SIZE-cur);
    nextfreepos= len-(LOG_BUF_SIZE-cur);
	nextfreepos = nextfreepos % LOG_BUF_SIZE;
    memcpy(logbuf, content+LOG_BUF_SIZE-cur, nextfreepos);
    cur=nextfreepos;
    return 0;
} 


int log_event_none(const char *header){
    char linebuf[LOG_LINE_MAX_SIZE];
    struct timeval time;
    struct tm tm;
    int reallen;
    memset(linebuf, 0, sizeof(linebuf));
    do_gettimeofday(&time);
    time_to_tm(time.tv_sec, 0, &tm);
    reallen=snprintf(linebuf, LOG_LINE_MAX_SIZE, "%d.%d-%d:%d:%d %s\n",
        tm.tm_mon, tm.tm_mday, tm.tm_hour,
        tm.tm_min, tm.tm_sec, header);
    return log_event(linebuf, LOG_LINE_MAX_SIZE);
} 

int log_event_int(const char *header, int intval){
    char linebuf[LOG_LINE_MAX_SIZE];
    struct timeval time;
    struct tm tm;
    int reallen;
    memset(linebuf, 0, sizeof(linebuf));
    do_gettimeofday(&time);
    time_to_tm(time.tv_sec, 0, &tm);
    reallen=snprintf(linebuf, LOG_LINE_MAX_SIZE, "%d.%d-%d:%d:%d %s %d \n",
        tm.tm_mon, tm.tm_mday, tm.tm_hour,
        tm.tm_min, tm.tm_sec, header, intval);
    return log_event(linebuf, LOG_LINE_MAX_SIZE);
} 

int log_event_int_int(const char *header, int intval, int newval){
    char linebuf[LOG_LINE_MAX_SIZE];
    struct timeval time;
    struct tm tm;
    int reallen;
    memset(linebuf, 0, sizeof(linebuf));
    do_gettimeofday(&time);
    time_to_tm(time.tv_sec, 0, &tm);
    reallen=snprintf(linebuf, LOG_LINE_MAX_SIZE, "%d.%d-%d:%d:%d %s %d %d\n",
        tm.tm_mon, tm.tm_mday, tm.tm_hour,
        tm.tm_min, tm.tm_sec, header, intval,newval);
    return log_event(linebuf, LOG_LINE_MAX_SIZE);
} 


int log_event_string(const char *header, const char *content){
    char linebuf[LOG_LINE_MAX_SIZE];
    int reallen;
    struct timeval time;
    struct tm tm;
    memset(linebuf, 0, sizeof(linebuf));
    do_gettimeofday(&time);
    time_to_tm(time.tv_sec, 0, &tm);
    reallen=snprintf(linebuf, LOG_LINE_MAX_SIZE, "%d.%d-%d:%d:%d %s %s \n",
        tm.tm_mon, tm.tm_mday, tm.tm_hour,
        tm.tm_min, tm.tm_sec, header, content);
    return log_event(linebuf, LOG_LINE_MAX_SIZE);
}

void log_event_dump(void){
    char linebuf[LOG_LINE_MAX_SIZE];
    int i;
    for(i=0; i<LOG_BUF_SIZE/LOG_LINE_MAX_SIZE; i++){
        memcpy(linebuf, logbuf+i*LOG_LINE_MAX_SIZE, LOG_LINE_MAX_SIZE);
        pr_info("%s", linebuf);
    }
}

void log_event_init(void){
    cur=0; 
    memset(logbuf, 0, LOG_BUF_SIZE);
}

void log_event_exit(void){
    
}

