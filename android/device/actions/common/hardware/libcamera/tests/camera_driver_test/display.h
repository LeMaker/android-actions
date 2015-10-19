
#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <semaphore.h>
#include <pthread.h>

typedef enum{
THREAD_RUN = 0,
THREAD_STOP,
}thread_state_t;

typedef enum{
MODE_DISP = 0,
MODE_SAVEFILE,
}process_t;


extern pthread_t q_thread_id; 
extern pthread_t dq_thread_id; 
extern thread_state_t thread_state;
extern process_t PROCESS_MODE;
extern int CAPTURE_NUM;
extern int frame_num;
extern pthread_mutex_t list_semi;
extern char *SAVEFILE_PATH;
extern char * CAMERA_TYPE;

extern void * dq_thread_routine();
extern void * q_thread_routine();





#endif //_DISPLAY_H
