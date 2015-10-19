/*
 * Copyright (C) 2015 The Android Open Source Project
 * 
 * Fix Bug: sometimes the app releasing will be hung on forever,which is the current luent's loophole. 
 * Actions code(author:liyuan)
 */

package android.hardware.camera2.legacy;
public class ActiveRequestCount{
  public int count;
  
}