package com.actions.performance;

import android.os.IBinder;
import android.os.IBinder.DeathRecipient;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.Process;
import android.os.Parcel;
import android.util.Log;
import android.content.Context;
import android.content.Intent;

   /*
     * internal use only
     * @hide
    */
public class PerformanceManager{
    private static final String TAG = "PerformanceManager";

    private static IBinder mPerformanceService;
    private static PerformanceManager mPerfServer;
    public static PerformanceManager getInstance(){
        if(mPerfServer==null){
            mPerfServer=new PerformanceManager();
        }
        return mPerfServer;
    }
    private PerformanceManager() {
        mPerformanceService = ServiceManager.getService("performanceservice");
        if (mPerformanceService == null) {
            Log.e(TAG, "error! can not get PerformanceService!");
        }
    }

   
    /*
     * internal use only
     * @hide
     */
    public boolean appNotify(String name){
        if(mPerformanceService==null){
            Log.e(TAG, "performance service not connect!");
            return false;
        }
        try{
            Parcel data = Parcel.obtain();                
            data.writeInterfaceToken("com.actions.hardware.IPerformanceService");
            data.writeInt(0); 
            // cmd is 0!                
            data.writeInt(Process.myPid());                
            data.writeString(name);                
            mPerformanceService.transact(IBinder.FIRST_CALL_TRANSACTION,                                  
                data, null, 0);               
           data.recycle();
           return true;
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }

    /*
     * internal use only
     * @hide
     */
    public boolean appNotifyForground(String name){
        if(mPerformanceService==null){
            Log.e(TAG, "performance service not connect!");
            return false;
        }
        try{
            Parcel data = Parcel.obtain();                
            data.writeInterfaceToken("com.actions.hardware.IPerformanceService");
            data.writeInt(1); 
            // cmd is 0!                
            data.writeInt(Process.myPid());                
            data.writeString(name);                
            mPerformanceService.transact(IBinder.FIRST_CALL_TRANSACTION,                                  
                data, null, 0);               
           data.recycle();
           return true;
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    
   /*
     * internal use only
     * @hide
     */
    public boolean appStart(){
        if(mPerformanceService==null){
            Log.e(TAG, "performance service not connect!");
            return false;
        }
        try{
            Parcel data = Parcel.obtain();                
            data.writeInterfaceToken("com.actions.hardware.IPerformanceService");     
            mPerformanceService.transact(IBinder.FIRST_CALL_TRANSACTION+100,                                  
                data, null, 0);               
           data.recycle();
           return true;
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }

     /*
     * internal use only
     * @hide
     */
    public boolean appExit(){
        if(mPerformanceService==null){
            Log.e(TAG, "performance service not connect!");
            return false;
        }
        try{
            Parcel data = Parcel.obtain();                
            data.writeInterfaceToken("com.actions.hardware.IPerformanceService");     
            mPerformanceService.transact(IBinder.FIRST_CALL_TRANSACTION+101,                                  
                data, null, 0);               
           data.recycle();
           return true;
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }

      /*
     * internal use only
     * @hide
     */
    public boolean appSetName(String name, int pid){
        if(mPerformanceService==null){
            Log.e(TAG, "performance service not connect!");
            return false;
        }
        
         try{
            Parcel data = Parcel.obtain();                
            data.writeInterfaceToken("com.actions.hardware.IPerformanceService");
            data.writeInt(pid);                          
            data.writeString(name);                
            mPerformanceService.transact(IBinder.FIRST_CALL_TRANSACTION+103,                                  
                data, null, 0);               
           data.recycle();
           return true;
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }

      /*
     * internal use only
     * @hide
     */
    public boolean appRotate(){
        if(mPerformanceService==null){
            Log.e(TAG, "performance service not connect!");
            return false;
        }
        
        try{
            Parcel data = Parcel.obtain();                
            data.writeInterfaceToken("com.actions.hardware.IPerformanceService");     
            mPerformanceService.transact(IBinder.FIRST_CALL_TRANSACTION+102,                                  
                data, null, 0);               
           data.recycle();
           return true;
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
      
    /*
     * internal use only
     * @hide
     */
    public boolean appInflate(String srcFile, String outFile){
        if(mPerformanceService==null){
            Log.e(TAG, "performance service not connect!");
            return false;
        }
        
        try{
            Parcel data = Parcel.obtain();                
            data.writeInterfaceToken("com.actions.hardware.IPerformanceService");  
            data.writeString(srcFile);
	     data.writeString(outFile);
                
            mPerformanceService.transact(IBinder.FIRST_CALL_TRANSACTION+200,                                  
                data, null, 0);               
           data.recycle();
           return true;
        }catch(Exception e){
            e.printStackTrace();
        }
        return false;
    }
    
      
}
