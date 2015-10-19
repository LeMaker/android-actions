package com.actions.hardware;

import android.os.IBinder;
import android.os.IBinder.DeathRecipient;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import android.content.Context;
import android.content.Intent;
import com.actions.hardware.IPerformanceService;

public class PerformanceManager{
    private static final String TAG = "PerformanceManager";

    private IPerformanceService mPerformanceService;

    public PerformanceManager() {
        mPerformanceService = IPerformanceService.Stub.asInterface(
                      	ServiceManager.getService("performanceservice"));
        if (mPerformanceService == null) {
            Log.e(TAG, "error! can not get PerformanceService!");
        }
    }

    public boolean notifier(int cmd, int pid, String payload){
        try{
            return mPerformanceService.notifier(cmd, pid, payload);
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    public boolean enableAutoPolicy(){
        try{
            return mPerformanceService.enableAutoPolicy();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    public boolean disableAutoPolicy(){
        try{
            return mPerformanceService.disableAutoPolicy();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    public boolean boostProcesses(int core){
        try{
            return mPerformanceService.boostProcesses(core);
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    public boolean restoreProcesses(){
        try{
            return mPerformanceService.restoreProcesses();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    public boolean enbleAutoAdjustBacklight(){
        try{
            return mPerformanceService.enbleAutoAdjustBacklight();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    public boolean disableAutoAdjustBacklight(){
        try{
            return mPerformanceService.disableAutoAdjustBacklight();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }

    public boolean massStorageOptimizeBegin(){
        try{
            return mPerformanceService.massStorageOptimizeBegin();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    public boolean massStorageOptimizeEnd(){
        try{
            return mPerformanceService.massStorageOptimizeEnd();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }

    public boolean cleanAllVmCaches(){
        try{
            return mPerformanceService.cleanAllVmCaches();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    public boolean cleanAllBackgroundApps(){
        try{
            return mPerformanceService.cleanAllBackgroundApps();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    public boolean syncDisk(){
        try{
            return mPerformanceService.syncDisk();
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }

    public boolean setCpuFreqRange(IBinder binder, int min, int max) {
        try{
            return mPerformanceService.setCpuFreqRange(binder, min, max);
        }catch(Exception e){
            e.printStackTrace();
        }       
        return false;
    }
    public boolean setCpuPerformanceLevel(IBinder binder, int level, int core, boolean boost){
        try{
            return mPerformanceService.setCpuPerformanceLevel(binder, level, core, boost);
        }catch(Exception e){
            e.printStackTrace();
        }       
        return false;
    }
    public boolean setGpuPerformanceLevel(IBinder binder, int level){
        try{
            return mPerformanceService.setGpuPerformanceLevel(binder, level);
        }catch(Exception e){
            e.printStackTrace();
        }       
        return false;
    }
    public boolean restoreCpuFreqRange(IBinder binder){
        try{
            return mPerformanceService.restoreCpuFreqRange(binder);
        }catch(Exception e){
            e.printStackTrace();
        }       
        return false;
    }
    public boolean restoreCpuPerformanceLevel(IBinder binder){
        try{
            return mPerformanceService.restoreCpuPerformanceLevel(binder);
        }catch(Exception e){
            e.printStackTrace();
        }       
        return false;
    }
    public boolean restoreGpuPerformanceLevel(IBinder binder){
        try{
            return mPerformanceService.restoreGpuPerformanceLevel(binder);
        }catch(Exception e){
            e.printStackTrace();
        }       
        return false;
    }
}
