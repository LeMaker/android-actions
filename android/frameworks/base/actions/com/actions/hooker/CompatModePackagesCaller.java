package com.actions.hooker;

import android.util.Log;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import com.actions.ActionsEnv;

/**
 * hook for package info: 
 * example: white list of app's run in compatible mode 
 * @hide
 */
public class CompatModePackagesCaller{
        private static Class mClassName=null;
        private static Method mMethod=null;
        private static final String TAG="CompatModePackagesCaller";
        private static void loadHookPackageClass() throws Exception{
            mClassName=Class.forName("com.android.server.am.HookCompatModePackages");
            mMethod=mClassName.getMethod("restoreCompatModes", new Class[] {Object.class});   
        }
        /** internal use only
        * @hide
        */
        public static void restoreCompatModes(Object cmp){
            if(!ActionsEnv.OPT_COMPAT_PACKAGE){
                return;
            }
            try{
                if(mClassName==null){
                    loadHookPackageClass();
                }
                mMethod.invoke(mClassName,cmp);
             }catch(Exception e){
                Log.d(TAG, "exception "+ e);
             }
        }
}
    