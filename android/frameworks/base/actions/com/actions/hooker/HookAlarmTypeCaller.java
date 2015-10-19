package com.actions.hooker;

import android.util.Slog;
import android.content.Context;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * hook for alarm process
 *  
 * @hide
 */
 
public class HookAlarmTypeCaller {
    private static Class mClassName=null;        
    private static Method mMethod=null;  
    private static Object mInstance;   
    private static final String TAG="HookAlarmTypeCaller";   
    private static void loadHookPackageClass() throws Exception{            
        mClassName=Class.forName("com.android.server.HookAlarmType");            
        mMethod=mClassName.getMethod("overridenAlarmType", new Class[] {Context.class, int.class});    
        mInstance=mClassName.newInstance();
    }       
    /**
     * internal use only
     * @hide
     */
    public static int  callHookAlarmType(Context context, int type){           
        int retType=type;
        try{                
            if(mClassName==null){                    
                loadHookPackageClass();
            }                
           retType=((Integer)(mMethod.invoke(mInstance, context,type))).intValue();            
            }catch(Exception e){                
                Slog.d(TAG, "exception "+ e);            
        }         
        return retType;
    }
}   