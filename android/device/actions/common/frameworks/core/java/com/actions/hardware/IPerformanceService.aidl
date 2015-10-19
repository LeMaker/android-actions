package com.actions.hardware;

interface IPerformanceService
{
    boolean notifier(int cmd, int pid, String payload);
    boolean enableAutoPolicy();
    boolean disableAutoPolicy();
    boolean boostProcesses(int core);   
    boolean restoreProcesses();  
    boolean enbleAutoAdjustBacklight();
    boolean disableAutoAdjustBacklight();
    
    boolean massStorageOptimizeBegin() ;
    boolean massStorageOptimizeEnd() ;
    
    boolean cleanAllVmCaches(); 
    boolean cleanAllBackgroundApps();
    boolean syncDisk(); 
    
    boolean setCpuFreqRange(IBinder binder, int min, int max);
    boolean setCpuPerformanceLevel(IBinder binder, int level, int core, boolean boost);
    boolean setGpuPerformanceLevel(IBinder binder, int level);
    boolean restoreCpuFreqRange(IBinder binder);
    boolean restoreCpuPerformanceLevel(IBinder binder);
    boolean restoreGpuPerformanceLevel(IBinder binder);
    
    
}
