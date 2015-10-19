package com.actions.hooker;

import android.util.Log;
import android.view.ViewConfiguration;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import com.actions.ActionsEnv;

/**
 * hook for package info: 
 * example: white list of app's run in compatible mode 
 * @hide
 */
public class ActionsViewConfigurationCaller{
        private static Class mClassName=null;
        private static Method mMethod=null;
        private static final String TAG="ActionsViewConfigurationCaller";
        private static void loadHookPackageClass() throws Exception{
            mClassName=Class.forName("android.view.ActionsViewConfiguration");
            mMethod=mClassName.getMethod("tune", new Class[] {ViewConfiguration.class});   
        }
        /** internal use only
        * @hide
        */
        public static void tune(ViewConfiguration config){
            if(!ActionsEnv.OPT_TEXTSHADOW){
                return;
            }
            try{
                if(mClassName==null){
                    loadHookPackageClass();
                }
                mMethod.invoke(mClassName,config);
             }catch(Exception e){
                Log.d(TAG, "exception "+ e);
             }
        }
}
    