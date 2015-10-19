package com.actions.hooker;

import android.util.Log;
import android.content.Context;
import android.view.View;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Constructor;
import java.io.PrintWriter;

/**
 * hook for ViewInflateThread
 *  
 * @hide
 */
 
public class ViewInflateThreadCaller {
    private static Class mClassName=null;        
    private static Constructor mCons=null;
    private static Method mCacheWhiteListView=null; 
    private static Method mCacheView=null;
    private static Method mGetCachedView=null;
    private static Method mDump=null;
    
    private  Object mInstance;   
    private static final String TAG="ViewInflateThreadCaller";   
    private static void loadHookPackageClass() throws Exception{            
        mClassName=Class.forName("com.android.internal.policy.impl.ViewInflateThread");            
        mCons = mClassName.getDeclaredConstructor(Context.class);
        mCacheWhiteListView = mClassName.getMethod("cacheWhiteListView");  
        mCacheView = mClassName.getMethod("cacheView", 
            new Class[] {String.class, int.class,  int.class, int.class});  
        mGetCachedView = mClassName.getMethod("getCachedView", 
            new Class[] {String.class, int.class,  int.class}); 
        mDump = mClassName.getMethod("dump", 
            new Class[] {PrintWriter.class, String.class});  
         return ;
    } 

     /**
     * internal use only
     * @hide
     */
     
    public  ViewInflateThreadCaller(Context context){
        try{
             if(mClassName==null){                    
                loadHookPackageClass();
            }
        mInstance=mCons.newInstance(context);
       
        }catch(Exception e){                
                Log.d(TAG, "exception "+ e);  
                new Exception().printStackTrace();
        }   
    }

     /**
     * internal use only
     * @hide
     */
    public void cacheWhiteListView(){
        try{
           mCacheWhiteListView.invoke(mInstance);
        }catch(Exception e){                
                Log.d(TAG, "exception "+ e);            
        }   
    }

     /**
     * internal use only
     * @hide
     */
    public void cacheView(String pkg, int theme, int id, int delay ){
        try{
           mCacheView.invoke(mInstance,pkg,theme,id, delay);
        }catch(Exception e){                
                Log.d(TAG, "exception "+ e);            
        }   
    }

     /**
     * internal use only
     * @hide
     */
    public View  getCachedView(String pkg, int theme, int id ){
        View v=null;
        try{
           v=(View )mGetCachedView.invoke(mInstance,pkg,theme,id);
        }catch(Exception e){                
                Log.d(TAG, "exception "+ e);            
        }   
        return v;
    }

   /**
     * internal use only
     * @hide
     */  
   public void dump(PrintWriter pw, String prefix) {
        try{
              mDump.invoke(mInstance,pw,prefix);
            }catch(Exception e){                
                    Log.d(TAG, "exception "+ e);            
            }   
       }
}   