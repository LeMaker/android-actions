/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.am;

import android.content.Context;
import android.content.ComponentName;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.SystemProperties;
import java.util.ArrayList;


/**
 * Control wheather service can be restarted
 */
class ActiveServicePolicy extends IBackgroundControl.Stub {
     
         //prevent kill service to restart
    ArrayList<String> mBlockRestartServices
            = new ArrayList<String>();
    // prevent start new background process to host broadcast
    ArrayList<String> mBlockRestartBroadcast
            = new ArrayList<String>();
    boolean inited=false;
    boolean blockAll=false;
    Context mContext;
    ActiveServicePolicy(Context context){
        mContext = context;
        
    }
    public  void  init(){
        if(inited)
            return;
        inited=true;
        int persistValue = SystemProperties.getInt("persist.sys.no_bg", -1);
        boolean lowram = isLowRamDeviceStatic();
        boolean gmsrom=false;
        if(SystemProperties.get("ro.build.type", "user").equals("user")||
            SystemProperties.get("ro.adb.secure", "0").equals("1")){
            gmsrom = true;
        }
        if(persistValue<0){
            //first time boot
            if(lowram)
                forbiddenAll(1);
            else if(!gmsrom)
                forbiddenAll(1);
            else
                forbiddenAll(0);
        }else{
            forbiddenAll(persistValue);
        }
    }
    public static boolean isLowRamDeviceStatic() {
        return "true".equals(SystemProperties.get("ro.config.low_ram", "false"));
    }
    
    boolean  shouldBlock(String pkgname) {
        if(!inited ||pkgname == null )
            return false;
        boolean found;
        synchronized(mBlockRestartServices){
            found = mBlockRestartServices.contains(pkgname);
        }
        //oh, actions, you are the king
        if((pkgname !=null) && (pkgname.startsWith("com.actions")||
            pkgname.startsWith("com.android")))
            return false;
        return found ||blockAll;
    }
    
     boolean  shouldBlockReceiver(String pkgname) {
         if(!inited||pkgname == null)
            return false;
         boolean found;
        synchronized(mBlockRestartBroadcast){
            found = mBlockRestartBroadcast.contains(pkgname);
        }
        //oh, actions, you are the king
        if((pkgname !=null) && (pkgname.startsWith("com.actions")||
            pkgname.startsWith("com.android")))
            return false;
        return found || blockAll;
    }
    
    // cheasonxie add. allow su started by application
    boolean shouldBlockReceiver(ComponentName comp) {
        if(!inited ||comp == null )
            return false;
        if(!shouldBlockReceiver(comp.getPackageName()))
            return false;
        String className = comp.getClassName();
        if(className != null && className.equals("eu.chainfire.supersu.NativeAccessReceiver"))
            return false;
        return true;
    }


    public void forbiddenPkg(String name){
         if(!inited)
            return;
        System.out.println("forbiddenPkg " + name);
        
     }
     public  void forbiddenAll(int forbidden){
          if(!inited)
            return;
         System.out.println("forbiddenAll ");
         blockAll = (forbidden>0);
         int state;
         if(forbidden>0){
            state = PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
             SystemProperties.set("persist.sys.no_bg", "1");
          }else{
            state = PackageManager.COMPONENT_ENABLED_STATE_ENABLED;    
            SystemProperties.set("persist.sys.no_bg", "0");
          }
          final long origId = Binder.clearCallingIdentity();
         
          /*actions_code(jiangbin:BUG00305909-fix:setting purebackground do not forbid googlequicksearchbox for third search)*/
          /*disablePackage("com.google.android.googlequicksearchbox", state);*/
         /*end*/
          //disableComponent(new ComponentName("com.google.android.gms",
          //  "com.google.android.gms.wearable.service.WearableService"), state);
          Binder.restoreCallingIdentity(origId);
     }
    public  void disablePkg(String pkg, String class1){
        System.out.println("not iml");
    }

    public  int isForbiddenAll(){
         if(!inited)
            return 0;
        if(blockAll)
            return 1;
        return 0;
    }
     /**
      * used to disbale packages to save memory
      *  1. com.google.android.googlequicksearchbox -> Velvet, GoogleQuickSearchBox
      *  2. com.google.android.gms/.wearable.service.WearableService -> gmscore.apk
      */
     private void disableComponent(ComponentName component, int state ){
     PackageManager pm = mContext.getPackageManager();
        try {
            pm.setComponentEnabledSetting(component, 
               state , 0);
        } catch (Exception e) {
            System.out.println("faild to disableComponent " + component + e);
        }
        
     }

     private void disablePackage(String package1, int state){
        PackageManager pm = mContext.getPackageManager();
        try {
            pm.setApplicationEnabledSetting(package1, 
                state, 0);
        } catch (Exception e) {
             System.out.println("faild to disableComponent " + package1 +e);
        } 
     }
 }
   
