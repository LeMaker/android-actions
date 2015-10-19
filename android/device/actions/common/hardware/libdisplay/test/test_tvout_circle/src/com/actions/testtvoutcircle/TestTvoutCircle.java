package com.actions.testtvoutcircle;

import java.lang.Thread;
import com.actions.hardware.DisplayManager;
import android.content.Context;
import android.app.Activity;
import android.os.Looper;
import android.os.Handler;
import android.os.Message;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.os.PowerManager;
import android.widget.Button;
import android.widget.TextView;
import android.graphics.Color;

public class TestTvoutCircle extends Activity implements OnClickListener {
    private final static String LOG_TAG = "TestTvoutCircle";
    
    public static final int CVBS_PRESS_TEST = 0;
    public static final int HDMI_PRESS_TEST = 1;
    public static final int LCD_PRESS_TEST = 3;

    private static final String displayMode = "0;1;17;33;3;19;35";
    private static final String cvbsFormat = "pal;ntsc";
    private static String hdmiFormatVid;
    private static String hdmiFormatDetail;
    private static final String displayer = "cvbs;hdmi;lcd0";
    
    private boolean circleStartClicked = false;
    private PowerManager.WakeLock mWakeLock = null;

    String displayModeArrays[] = displayMode.split(";");
    String cvbsFormatArrays[] = cvbsFormat.split(";");
    String hdmiFormatVidArrays[];
    String hdmiFormatDetailArrays[];
    String displayerArrays[] = displayer.split(";");

    int displayModeIndex = 1;// if modify displaymod,modify here
    int cvbsFormatIndex = 0;
    int hdmiFormatIndex = 0;
    int displayerIndex = 0;

    private DisplayManager mDisplayManager;

    private TextView textView = null;
    private Button circleStartButton = null;
    private Button circleStopButton = null;
    
    private Handler mainHandler = new Handler() {
        public void handleMessage(Message msg) {
            Log.e(LOG_TAG, "enter main thread's handleMessage\n");
                switch (msg.what) {
                    case CVBS_PRESS_TEST: {
                         if (cvbsFormatIndex == 0) {
                         	 Log.e(LOG_TAG, "enter main thread's handleMessage,cvbsFormatIndex:0\n");
                             textView.setText("PAL");
                         } else {
                             textView.setText("NTSC");
                         }
                         
                         cvbsFormatIndex++;

                    } break;
                    
                    case HDMI_PRESS_TEST: {
                        textView.setText(hdmiFormatDetailArrays[hdmiFormatIndex]);
                        hdmiFormatIndex++;
                    } break;
                    
                    case LCD_PRESS_TEST: {
                       textView.setText("LCD");
                       cvbsFormatIndex = 0;
                       hdmiFormatIndex = 0;
                    } break;
                }
                
                Message childMsg = new Message();
                childMsg.what = childThread.TVOUT_PRESS_TEST;
                childThread.childHandler.sendMessageDelayed(childMsg, 5000);
        }
    };
    
    public LooperThread childThread;
    
    class LooperThread extends Thread {
        public static final int TVOUT_PRESS_TEST = 0;
        
        public Handler childHandler;
        
        public void run() {
            Looper.prepare();
            Log.e(LOG_TAG, "enter LooperThread's run\n");
            childHandler = new Handler() {
                public void handleMessage(Message msg) {
                Log.e(LOG_TAG, "enter child's thread handleMessage\n");
                    switch (msg.what) {
                        case TVOUT_PRESS_TEST: {
                            pressTest();
                        } break;
                    }
                }
            };
            
            Looper.loop();
            
        }
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        Log.e(LOG_TAG, "enter TestTvoutCircle's onCreate\n");
        textView = (TextView) findViewById(R.id.textview);
        textView.setTextColor(Color.RED);
        textView.setTextSize(50);
        textView.setBackgroundColor(Color.BLUE);
        circleStartButton = (Button) findViewById(R.id.button_circle_start);
        circleStopButton = (Button) findViewById(R.id.button_circle_stop);

        circleStartButton.setOnClickListener(this);
        circleStopButton.setOnClickListener(this);
        
         //Acquire the full wake lock to keep the device up
        PowerManager pm = (PowerManager) this.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "TestTvoutCircle");
        mWakeLock.setReferenceCounted(false);

        mDisplayManager = new DisplayManager();
        if (mDisplayManager == null) {
            Log.e(LOG_TAG, "mDisplayManager == null");
        }

        getHdmiVidList();

        childThread = new LooperThread();
        childThread.start();

        Log.i(LOG_TAG, "Hello Activity Created");
    }
    
      @Override
    protected void onResume() {
        Log.v(LOG_TAG, "onResume, acquire wakelock");
        super.onResume();
        mWakeLock.acquire();
    }
    
     protected void onPause() {
        Log.v(LOG_TAG, "onPause, release wakelock");
        mWakeLock.release();
        super.onPause();
    }

    private void switchToTv(String devName) {
        try {
            mDisplayManager.setOutputDisplayer(devName);
        } catch (Exception e) {
            e.printStackTrace();
        }

    }
        
    private void pressTest() {

        if (cvbsFormatIndex < cvbsFormatArrays.length) {   
            Message mainMsg = new Message();
            mainMsg.what = CVBS_PRESS_TEST;
            mainHandler.sendMessage(mainMsg);
            configure(displayModeIndex, cvbsFormatArrays[cvbsFormatIndex], -1, "cvbs&&lcd0"); 
        } else if (hdmiFormatIndex < hdmiFormatVidArrays.length) {
            Message mainMsg = new Message();
             mainMsg.what = HDMI_PRESS_TEST;
             mainHandler.sendMessage(mainMsg);
            configure(displayModeIndex, "", Integer.parseInt(hdmiFormatVidArrays[hdmiFormatIndex]), "hdmi&&lcd0");
        } else {
            Message mainMsg = new Message();
            mainMsg.what = LCD_PRESS_TEST;
            mainHandler.sendMessage(mainMsg);
            configure(0, "", -1, "lcd0");
        }

    }
        
    private int getHdmiVidList() {
        int i;
        String hdmiCap = mDisplayManager.getHdmiCap();
        if (hdmiCap == null || hdmiCap.isEmpty()) {
            return -1;
        }
        Log.d(LOG_TAG, "hdmiCap=" + hdmiCap);
        String[] hdmiCapArray = hdmiCap.split(";");
        for (i = 0; i < hdmiCapArray.length; i++) {
            String oneLine[] = hdmiCapArray[i].split(",");

            if (hdmiFormatVid == null) {
            	hdmiFormatDetail = oneLine[0] + ";";
                hdmiFormatVid = oneLine[1] + ";";
            } else if (i == hdmiCapArray.length - 1) {
            	hdmiFormatDetail += oneLine[0];
                hdmiFormatVid += oneLine[1];
            } else {
            	hdmiFormatDetail += oneLine[0] + ";";
                hdmiFormatVid += oneLine[1] + ";";

            }
        }
        
        hdmiFormatVidArrays = hdmiFormatVid.split(";");
        hdmiFormatDetailArrays = hdmiFormatDetail.split(";");

        return 0;

    }

    private void configure(int mode, String cvbsFormat, int hdmiVid, String devices) {

        mDisplayManager.setDisplaySingleMode(mode);

        if (cvbsFormat != null && !cvbsFormat.equals(""))
            mDisplayManager.setFormat(cvbsFormat);

        if (hdmiVid >= 0)
            mDisplayManager.setHdmiVid(hdmiVid);

        if (devices != null && !devices.equals(""))
            switchToTv(devices);
    }

    @Override
    public void onClick(View v) {
        if (v.equals(circleStartButton)) {
            if (!circleStartClicked) {
                Log.i(LOG_TAG, "circle start button clicked!");
                Message msg = new Message();
                msg.what = childThread.TVOUT_PRESS_TEST;
                childThread.childHandler.sendMessage(msg);
                circleStartClicked = true;
            }

        } else if (v.equals(circleStopButton)) {
            if (circleStartClicked) {
            	Log.i(LOG_TAG, "circle stop button clicked!");
                mainHandler.removeMessages(CVBS_PRESS_TEST);
                mainHandler.removeMessages(HDMI_PRESS_TEST);
                mainHandler.removeMessages(LCD_PRESS_TEST);
                childThread.childHandler.removeMessages(childThread.TVOUT_PRESS_TEST);
                circleStartClicked = false;
            }
        }

    }
}