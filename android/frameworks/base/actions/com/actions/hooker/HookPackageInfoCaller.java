package com.actions.hooker;

import android.util.Log;
import android.content.pm.PackageParser;
import java.lang.reflect.Field;
import java.lang.reflect.Method;


/**
 * hook for package info: 
 * example: white list of app's run in compatible mode 
 * @hide
 */
public class HookPackageInfoCaller{
        private static Class mClassName=null;
        private static Method mMethod=null;
        private static final String TAG="HookPackageInfoCaller";
        private static void loadHookPackageClass() throws Exception{
            mClassName=Class.forName("com.android.server.pm.HookPackageInfo");
            mMethod=mClassName.getMethod("hookActivityInfo", new Class[] {PackageParser.Package.class});   
        }
        /** internal use only
        * @hide
        */
        public static void callHookActivityInfo(PackageParser.Package pkg){
            try{
                if(mClassName==null){
                    loadHookPackageClass();
                }
                mMethod.invoke(mClassName,pkg);
             }catch(Exception e){
                Log.d(TAG, "exception "+ e);
             }
        }
}
    