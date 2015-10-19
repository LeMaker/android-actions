/*
 * Copyright (C) 2007 The Android Open Source Project
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
package com.actions;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.io.PrintWriter;


/**
 * Provides access to environment variables.
 */
public class ActionsEnv {
   
    /** {@hide} */
    public static final boolean DEBUG_APP_LAUNCH = true;
    public static final boolean OPT_APP_LAUNCH = true;
    public static final boolean OPT_STARTWINDOW_INFLATE = true;
    public static final boolean OPT_TEXTSHADOW = true;
    public static final boolean OPT_MOVE_WALLPAPER_TO_LAUNCHER = true;
    public static final boolean OPT_COMPAT_PACKAGE = true;
     /** {@hide} */
    public static final boolean OPT_STARTWINDOW_RGB565 = true;
    public static int getSupport(){
        return ActionsConfig.ACTIONS_FEATURE_KEYTUNE_DEFAULT_ON;
    }
    /** 
     *  dump all configs in ActionsConfig.java
     *  actions config is defined in android/build/actions_owl.config
     *
     *
     *{@hide} 
     */	
    public static void dumpFeature(PrintWriter pw ){
     try{
        Class ownerClass = Class.forName("com.actions.ActionsConfig");
        Field[] fs=ownerClass.getDeclaredFields();
        pw.println("dump ActionsConfig");
        pw.println("see also android/build/actions_owl.config");
       
            for(int i = 0 ; i < fs.length; i++){
                Field field = fs[i];
                if((field.getModifiers() & (Modifier.STATIC| Modifier.PUBLIC))
                    ==(Modifier.STATIC| Modifier.PUBLIC)){
                    Object property = field.get(ownerClass);
                    pw.println(field.getName() +" = " + property);	
                }
            }
         }catch(Exception e){
                e.printStackTrace();
      }
   }   
}
