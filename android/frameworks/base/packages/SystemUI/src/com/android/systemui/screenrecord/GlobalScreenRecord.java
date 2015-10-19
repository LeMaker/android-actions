/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.systemui.screenrecord;

import android.app.PendingIntent;
import android.app.Notification;
import android.app.NotificationManager;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.MediaCodec;
import android.media.MediaCodec.BufferInfo;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.content.res.Resources;

import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;
import java.nio.ByteBuffer;
import android.util.DisplayMetrics;
import android.util.Slog;
import android.os.Environment;
import android.os.SystemClock;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.location.Location;
import android.provider.MediaStore;
import android.provider.MediaStore.MediaColumns;
import android.provider.MediaStore.Video;
import java.io.IOException;
import java.io.File;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.android.systemui.R;
import android.net.Uri;

import android.os.PowerManager;
import android.widget.Toast;

class GlobalScreenRecord {
	
	private static final String TAG = "GlobalScreenRecord";	
	
	private static final boolean DEBUG = false;
	public static final String VIDEO_BASE_URI = "content://media/external/video/media";
	private static final String SCREENRECORD_DIR_NAME = "ScreenRecords";	
	private static final String SCREENRECORD_FILE_NAME_TEMPLATE = "%s.mp4";	
	private static final String DISPLAY_NAME = "Screen Record";	
	private static final int SCREENRECORD_NOTIFICATION_ID = 789;
	private static final int BIT_RATE = 6000000;	
	private static final int FRAME_RATE = 29;	
	private static final int I_FRAME_INTERVAL = 10;	
	
	private final Handler mHandler = new Handler();
	
	private Context mContext;
	
	private final int mScreenWidth;	
	private final int mScreenHeight;	
	private final int mScreenDpi;	

	private final DisplayManager mDisplayManager;	
	private final WindowManager mWindowManager;
	private ContentResolver mContentResolver;
	private NotificationManager mNotificationManager;
	
	private static long mRecordingStartTime;
	
    private String mRecordVideoFileName;		
    private String mRecordVideoFilePath;
	private static File mScreenRecordDir = null;
	private String mScreenRecordDirName;
	private String mVideoFilename;
	
	private OutputStream myDebugFile;

	private String mCurrentVideoFilename;
    private Uri mCurrentVideoUri;
    private boolean mCurrentVideoUriFromMediaSaved;
    private static ContentValues mCurrentVideoValues;
	
    private ScreenRecordThread mScreenRecordThread;	
    private PowerManager.WakeLock mScreenRecordWakeLock;
    private boolean mIsRecording = false;

    private final Runnable mFinishedCallback = new Runnable() {
        @Override
        public void run() {
        	//ensure save video file be processed once the write video process thread already be finished.
            saveVideoFile(); 
        }
    }; 

    private class VideoSaveTask extends AsyncTask <Void, Void, Uri> {
    	private static final int SCREENSHOT_NOTIFICATION_ID = 789;
	    private static final String TAG = "VideoSaveTask";	
        private String path;
        private long duration;
        private ContentValues values;
        private NotificationManager notifyer;
        private ContentResolver resolver;
        private Context context;

        public VideoSaveTask(Context context , String path, long duration, ContentValues values,
                NotificationManager notifyer, ContentResolver r) {
            this.path = path;
            this.duration = duration;
            this.values = new ContentValues(values);
            this.notifyer = notifyer;
            this.resolver = r;
            this.context = context;
        }

        @Override
        protected Uri doInBackground(Void... v) {
            values.put(Video.Media.SIZE, new File(path).length());
            values.put(Video.Media.DURATION, duration);
            Uri uri = null;
            try {
                Uri videoTable = Uri.parse(VIDEO_BASE_URI);
                uri = resolver.insert(videoTable, values);

                // Rename the video file to the final name. This avoids other
                // apps reading incomplete data.  We need to do it after we are
                // certain that the previous insert to MediaProvider is completed.
                String finalName = values.getAsString(
                        Video.Media.DATA);
                if (new File(path).renameTo(new File(finalName))) {
                    path = finalName;
                }
                if (uri != null) {
                    resolver.update(uri, values, null, null);
                } else {
                    throw new Exception("uri == null");
                }
                
            } catch (Exception e) {
                // We failed to insert into the database. This can happen if
                // the SD card is unmounted.
                Slog.e(TAG, "failed to add video to media store", e);
                GlobalScreenRecord.notifyScreenRecordError(mContext,mNotificationManager);
                uri = null;
            } finally {
                Slog.v(TAG, "Current video URI: " + uri);
            }
            if (mScreenRecordDir != null) {
                clearRecordDir(mScreenRecordDir.getPath());
            }
            return uri;
        }

        @Override
        protected void onPostExecute(Uri uri) {
			  if(notifyer != null && context != null && uri != null){
				Resources r = context.getResources();
				// Create the intent to show the screenshot in gallery
		        Intent launchIntent = new Intent(Intent.ACTION_VIEW);
		        launchIntent.setDataAndType(uri, "video/mp4");
		        launchIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            
				Notification.Builder  mNotificationBuilder = new Notification.Builder(context)
		            .setTicker(r.getString(R.string.screenrecord_saving_ticker) + (" "))
		            .setContentTitle(r.getString(R.string.screenrecord_saving_title))
		            .setContentText(r.getString(R.string.screenrecord_saving_text))
		            .setContentIntent(PendingIntent.getActivity(mContext, 0, launchIntent, 0))
		            .setSmallIcon(R.drawable.stat_notify_image)
		            .setWhen(System.currentTimeMillis());
		       
		        Notification n = mNotificationBuilder.build();
		        n.flags |= Notification.FLAG_AUTO_CANCEL;
		        mNotificationManager.cancel(SCREENSHOT_NOTIFICATION_ID);
        		mNotificationManager.notify(SCREENSHOT_NOTIFICATION_ID, n);
			}
        }
    }
    
	private final class ScreenRecordThread extends Thread {
		
		private static final int TIMEOUT_USEC = 1000000;		
		
		private final int mWidth;		
		private final int mHeight;		
		private final int mDensityDpi;		
		private volatile boolean mQuitting;		
		private int videoTrackIndex = 0;
		
		public ScreenRecordThread(int width, int height, int densityDpi) {			
			mWidth = width;			
			mHeight = height;			
			mDensityDpi = densityDpi;			
			if(DEBUG){				
				Slog.i(TAG, "new ScreenRecordThread  mWidth " + mWidth + " mHeight " + mHeight + " mDensityDpi " + mDensityDpi);
			}
		}
		
		@Override
		public void run() {
						
			// config meida format  		   
		    MediaFormat format = MediaFormat.createVideoFormat("video/avc",	mWidth, mHeight);			
			format.setInteger(MediaFormat.KEY_COLOR_FORMAT,	MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);			
			format.setInteger(MediaFormat.KEY_BIT_RATE, BIT_RATE);			
			format.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);			
			format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL,	I_FRAME_INTERVAL);		
            
      // creat media codec 
      MediaCodec codec = null;
      try {
			 	codec = MediaCodec.createEncoderByType("video/avc");		
			} catch (IOException e) {	
				Slog.e(TAG, " codec can't create ", e);
				GlobalScreenRecord.notifyScreenRecordError(mContext,mNotificationManager);
				return;
      };
			codec.configure(format, null, null,	MediaCodec.CONFIGURE_FLAG_ENCODE);			
			
			// creat media muxer 
		    MediaMuxer muxer = null;		    			
			try {				
				muxer = new MediaMuxer(mVideoFilename, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);				
			} catch (IOException e) {
				Slog.e(TAG, " muxer can't open ", e);
				GlobalScreenRecord.notifyScreenRecordError(mContext,mNotificationManager);
				try {     
                	codec.release();  
            	} catch (Exception er) {
                	er.printStackTrace();
            	};
				return;
			}
			
			// get surface from codec 			
			Surface surface = codec.createInputSurface();		
			
			// new vidtural display used the surface form codec 
			VirtualDisplay virtualDisplay = mDisplayManager
					.createVirtualDisplay(DISPLAY_NAME, mWidth, mHeight,
							mDensityDpi, surface,  DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC);	
			
			if (virtualDisplay != null) {           
	            //start codec
				codec.start();	
				mRecordingStartTime = SystemClock.uptimeMillis();
				            
				stream(codec, muxer);
								
				// send signal end of stream 
				codec.signalEndOfInputStream();						

				if(DEBUG){
					if(myDebugFile != null){			
						try{   
							myDebugFile.close(); 
						}catch(Exception ex){   
							ex.printStackTrace();   
						}   
					}
				}
								
				// release virtual display
				virtualDisplay.release();
			}else{
				GlobalScreenRecord.notifyScreenRecordError(mContext,mNotificationManager);
			}	
			
			// codec and muxer relase 
            try {
                muxer.stop();
            } catch (Exception e) {
                e.printStackTrace();
            }
            try {
                codec.stop();       
            } catch (Exception e) {
                e.printStackTrace();
            };	     

            try {
                muxer.release(); 
            } catch (Exception e) {
                e.printStackTrace();
            };
            try {     
                codec.release();  
            } catch (Exception e) {
                e.printStackTrace();
            };	

			mHandler.post(mFinishedCallback);
		}

		public void quit() {
			mQuitting = true;
		}

		/*
		 *BUGFIX:BUG00232335 
		 *catch MediaCodec exception and stop screen record
		 ***********************************
		 *ActionsCode(author:fanguoyong,change_code)
		 */				 
        private void stream(MediaCodec codec, MediaMuxer muxer) {
            BufferInfo info = new BufferInfo();
            ByteBuffer[] buffers = null;
            long TimeUs = 0;
            int index = -1;
            while (!mQuitting) {
            	  //ActionsCode(author:fanguoyong,bugfix)
            	try {
                	 index = codec.dequeueOutputBuffer(info, TIMEOUT_USEC);
                }catch(IllegalStateException e) {
                	 Slog.e(TAG, " MediaCodec dequeueOutputBuffer error: ", e);
					 GlobalScreenRecord.notifyScreenRecordError(mContext,mNotificationManager);
					 return;
                }
                if(DEBUG){
                   Slog.d(TAG, "index:" + index);
            	}

                 
                if (index >=0) {
                    if ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                        info.size = 0;    
                    }

                    if (info.size > 0) {
                        if (buffers == null) {
                            buffers = codec.getOutputBuffers();
                        }
                        ByteBuffer buffer = buffers[index];
                        buffer.limit(info.offset + info.size);
                        buffer.position(info.offset);   
                        
                        if(DEBUG){
                            byte[] data = new byte[info.size];
                            try{
                                buffer.get(data, 0, info.size);   
                                myDebugFile.write(data);   
                                myDebugFile.flush();
                            }catch(Exception ex){   
                                ex.printStackTrace();   
                            }   
                        }   
                        if(info.presentationTimeUs != 0){   
                            
                            if(DEBUG){
                                Slog.d(TAG, "buffer info presentationTimeUs ~~~~~ " +  info.presentationTimeUs);
                            }
                            muxer.writeSampleData(videoTrackIndex, buffer, info);   
                        }   
                        codec.releaseOutputBuffer(index, false);   
                                            
                    }
                     
                    if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                        Slog.d(TAG, "Received end-of-stream");
                        mQuitting = true;

                    }
                } else if (index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {                  
                    buffers = null;
                    Slog.w("TAG", "Codec output buffers changed .");
                } else if(index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED){
                     // config muxer            
                    if (muxer != null) {                
                        MediaFormat newFormat = codec.getOutputFormat();
                        videoTrackIndex = muxer.addTrack(newFormat);
                        if(DEBUG){
                            Slog.i(TAG, "muxer.addTrack videoTrackIndex " +  videoTrackIndex);
                        }
                        muxer.setOrientationHint(0);
                        muxer.start();                  
                    }
                    Slog.w("TAG", "Codec out format changed .");
                } else if (index == MediaCodec.INFO_TRY_AGAIN_LATER) {
                    //Slog.w("TAG", "Codec dequeue buffer timed out.");
                } else {
                    Slog.e("TAG", "Codec dequeue buffer error break mQuitting.");
                    break;
                }
            }
        }
    }

	private void releaseScreenRecord() {
		if (mScreenRecordThread != null) {
			mScreenRecordThread.quit();
			mScreenRecordThread = null;
		}		
	}
	
	private String convertOutputFormatToMimeType(int outputFileFormat) {
        if (outputFileFormat == MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4) {
            return "video/mp4";
        }
        return "video/3gpp";
    }
    	
	private String convertOutputFormatToFileExt(int outputFileFormat) {
        if (outputFileFormat == MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4) {
            return ".mp4";
        }
        return ".3gp";
    }
    
    private String createName(){
    	String imageDate = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss")
    	.format(new Date(System.currentTimeMillis()));
		mRecordVideoFileName = String.format(SCREENRECORD_FILE_NAME_TEMPLATE, imageDate);
		
		return mRecordVideoFileName;
    }
    
	private void generateVideoFilename(int outputFileFormat) {
        long dateTaken = System.currentTimeMillis();
        // Used when emailing.
		String title = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss").format(new Date(dateTaken));
		mRecordVideoFileName = String.format(SCREENRECORD_FILE_NAME_TEMPLATE, title);
        mRecordVideoFilePath = new File(mScreenRecordDir, mRecordVideoFileName).getAbsolutePath();
        
        String mime = convertOutputFormatToMimeType(outputFileFormat);
        String tmpPath = mRecordVideoFilePath + ".tmp";
        
        mCurrentVideoValues = new ContentValues(9);
        mCurrentVideoValues.put(Video.Media.TITLE, title);
        mCurrentVideoValues.put(Video.Media.DISPLAY_NAME, mRecordVideoFileName);       


        mCurrentVideoValues.put(Video.Media.DATE_TAKEN, dateTaken);
        mCurrentVideoValues.put(MediaColumns.DATE_MODIFIED, dateTaken / 1000);
        mCurrentVideoValues.put(Video.Media.MIME_TYPE, mime);
        mCurrentVideoValues.put(Video.Media.DATA, mRecordVideoFilePath);
        
        mCurrentVideoValues.put(Video.Media.RESOLUTION,
                Integer.toString(mScreenWidth) + "x" +
                Integer.toString(mScreenHeight));
                
        
        mVideoFilename = tmpPath;
                
        if(DEBUG){
			Slog.i(TAG, " new record file ....." + mVideoFilename);
			try{  
	        	myDebugFile = new FileOutputStream(mRecordVideoFilePath + ".data");   
	        }catch(Exception ex){   
				ex.printStackTrace();   
			}			
		}
    }

	public boolean fileIsExists(String file) {                
		try{                        
			File f=new File(file);
			if(!f.exists()){                                
				return false;
			}
		}catch (Exception e) {
			// TODO: handle exception
			return false;                
		}                
		return true;        
	}	
    
    private void saveVideoFile() {
		
		mHandler.removeCallbacks(mFinishedCallback);
		
        long duration = SystemClock.uptimeMillis() - mRecordingStartTime;
        if (duration > 0) {

        } else {
            Slog.w(TAG, "Video duration <= 0 : " + duration);
        }       
        if(DEBUG){
			Slog.i(TAG, " save record file ....." + mVideoFilename);
		}
		//Check if .tmp file is exist to save!
		if(!fileIsExists(mVideoFilename)) {
			Slog.e(TAG, "no file to save!");
			GlobalScreenRecord.notifyScreenRecordError(mContext,mNotificationManager);
			return;
		}
		
		new VideoSaveTask(mContext,mVideoFilename, duration, mCurrentVideoValues, mNotificationManager, mContentResolver).execute();
    }


	/**
	 * @param context
	 *            everything needs a context :(
	 */
	public GlobalScreenRecord(Context context) {
		
		mContext =  context;
		
		//get widow manager for get display info 
		mWindowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);		
		Display mDisplay = mWindowManager.getDefaultDisplay();
		DisplayMetrics mDisplayMetrics = new DisplayMetrics();
		mDisplay.getRealMetrics(mDisplayMetrics);		
		mScreenWidth = mDisplayMetrics.widthPixels;
		mScreenHeight = mDisplayMetrics.heightPixels;
		mScreenDpi = mDisplayMetrics.widthPixels;
		
		// get display manager for new virtual display		
		mDisplayManager = (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
		
		// new a dir for record file  		
		if (mScreenRecordDir == null) {
			mScreenRecordDir = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES),
					SCREENRECORD_DIR_NAME);
			mScreenRecordDir.mkdirs();
		}
        PowerManager powerManager = (PowerManager)mContext.getSystemService(Context.POWER_SERVICE);
        mScreenRecordWakeLock = powerManager.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "mScreenRecordWakeLock");
        mScreenRecordWakeLock.setReferenceCounted(false);
        
		mContentResolver = context.getContentResolver();
		
		mNotificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);

	}

	/**
	 * start Takes a screen record of the current display 
	 */
    synchronized void startScreenRecord(Runnable finisher, boolean statusBarVisible,
	       boolean navBarVisible) {
	    if (mIsRecording) {
	    	if (finisher != null) {
            	finisher.run();   
        	} 
            return;
        } 
    	mScreenRecordDir = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES),
    			SCREENRECORD_DIR_NAME);
    	if (!mScreenRecordDir.isDirectory()){
    	    mScreenRecordDir.mkdirs();  
    	}

		//clear all .tmp file            
		if (mScreenRecordDir != null) {
        	clearRecordDir(mScreenRecordDir.getPath());
        }
		
        releaseScreenRecord();
        generateVideoFilename(MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);		
        mScreenRecordThread = new ScreenRecordThread(mScreenWidth,mScreenHeight,mScreenDpi);		
        mScreenRecordThread.start();
               
		GlobalScreenRecord.notifyScreenRecording(mContext,mNotificationManager);
        if (mScreenRecordWakeLock != null) {
            mScreenRecordWakeLock.acquire();
            Slog.i(TAG,"mScreenRecordWakeLock.acquire()...");
        }
        mIsRecording = true;
        if (finisher != null) {
            finisher.run();   
        } 	
		
        Slog.i(TAG,"startScreenRecord ...");
    }

  
	/**
     *  stop Takes a screen record of the current display 
     */
    synchronized void stopScreenRecord(Runnable finisher, boolean statusBarVisible,
        boolean navBarVisible) {    
        if (!mIsRecording) {
        	if (finisher != null) {
            	finisher.run();   
        	} 
            return;
        }   
        releaseScreenRecord();
        //saveVideoFile();   
         
        if (mScreenRecordWakeLock != null) {
            mScreenRecordWakeLock.release();
            Slog.i(TAG,"mScreenRecordWakeLock.release()...");
        }
        mIsRecording = false;
        if (finisher != null) {
            finisher.run();   
        } 	
        
        Slog.i(TAG,"stopScreenRecord ...");
    }

	private void clearRecordDir(String path) {
        if (path == null) {
            return;
        }
        File dir = new File(path);
        if (dir == null) {
            return;
        }
        File file[] = dir.listFiles();
        if (file == null) {
            return;
        }
        for (int i = 0; i < file.length; i++) {
        	//System.out.println(file[i].getName());
            if (!file[i].isFile()) {
            	continue;
            }
        	if (file[i].getName().contains(".tmp")
        			|| file[i].length() == 0) {
        		Slog.i(TAG,"delete:" + file[i].getName());
        		file[i].delete();
        		//fix BUG00206281
            System.gc();
        	}
        }
	}
	
	static void notifyScreenRecording(Context context, NotificationManager nManager) {
							
        Resources r = context.getResources();

        // Clear all existing notification, compose the new notification and show it
        Notification.Builder b = new Notification.Builder(context)
            .setTicker(r.getString(R.string.screenrecord_recording_title))
            .setContentTitle(r.getString(R.string.screenrecord_recording_title))
            .setContentText(r.getString(R.string.screenrecord_recording_text))
            .setSmallIcon(R.drawable.stat_notify_image)
            .setWhen(System.currentTimeMillis())
            .setAutoCancel(true);
        Notification n =
            new Notification.BigTextStyle(b)
                .bigText(r.getString(R.string.screenrecord_recording_title))
                .build();                         
        n.flags |= Notification.FLAG_NO_CLEAR;
        nManager.notify(SCREENRECORD_NOTIFICATION_ID, n);
	}
    
	static void notifyScreenRecordError(Context context,
			NotificationManager nManager) {
							
        Resources r = context.getResources();

        // Clear all existing notification, compose the new notification and show it
        Notification.Builder b = new Notification.Builder(context)
            .setTicker(r.getString(R.string.screenrecord_error_title))
            .setContentTitle(r.getString(R.string.screenrecord_error_title))
            .setContentText(r.getString(R.string.screenrecord_error_text))
            .setSmallIcon(R.drawable.stat_notify_image_error)
            .setWhen(System.currentTimeMillis())
            .setAutoCancel(true);
        Notification n =
            new Notification.BigTextStyle(b)
                .bigText(r.getString(R.string.screenrecord_error_title))
                .build();
                
        nManager.notify(SCREENRECORD_NOTIFICATION_ID, n);
	}
}
