/*
 * control the outstanding of GPU, Avoiding the lcd flash when camera preview 
 */

#ifndef _CAMERA_GPU_OUTSTANDING__
#define _CAMERA_GPU_OUTSTANDING__

int setGPUOutstanding(int val);

int startGPUOutstanding();

int stopGPUOutstanding();

int clearGPUOutstanding();

#endif


