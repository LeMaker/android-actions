package com.android.gallery3d.gif;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;

import com.android.gallery3d.glrenderer.BitmapTexture;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class AnimationTexture {
    private static final String TAG = "AnimationTexture";

    private BitmapTexture mBitmapTexture = null;
    private InputStream mInputStream;
    private HandlerThread mHandlerThread;
    private AnimationHandler mHandler;
    public int mImageHeight = 0;
    public int mImageWidth = 0;
    private OnTextureChangedListener mChangedListener;

    public AnimationTexture() {
        mHandlerThread = new HandlerThread("MyHandlerThread");
        mHandlerThread.start();
    }
    
    public void setGifStream(InputStream is) {
        mInputStream = is;
    }
    
    public void setGifImage(String path) {
        FileInputStream fis = null;
        if (path != null) {
            try {
                fis = new FileInputStream(new File(path));
                setGifStream(fis);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
                try {
                    if (fis != null) {
                        fis.close();
                    }
                } catch (IOException e1) {
                    e1.printStackTrace();
                }
            }
        }
    }
    
    public static boolean isGifStream(InputStream is) {
        int supportSize = 1000;
    	String prefix = "";
        try {
            for (int i = 0; i < 6; i++) {
                prefix += (char) is.read();
            }
            int width = is.read() | (is.read() << 8);
            int height = is.read() | (is.read() << 8);

            int ramCapacity = 512;
            try {
            	Class properties = Class.forName("android.os.SystemProperties");
            	Method getIntMethod = properties.getMethod("getInt", new Class[] {String.class, int.class});
            	ramCapacity = (Integer) getIntMethod.invoke(properties, new Object[] {"system.ram.total", new Integer(512)});
            } catch (Exception e1) {
            	e1.printStackTrace();
            }

            supportSize = ramCapacity <= 512 ? 1000 : 1500;
            return (prefix.startsWith("GIF") 
                    && width <= supportSize
                    && height <= supportSize);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }
    
    public void start() {
        if (mHandler == null) {
            mHandler = new AnimationHandler(mHandlerThread.getLooper());
        }
        mHandler.sendEmptyMessage(AnimationHandler.START);
    }

    public void stop() {
        if (mHandler != null) {
            mHandler.sendEmptyMessage(AnimationHandler.STOP);
            mHandler = null;
        }
        recycleTexture();
    }
    
    private void recycleTexture() {
        if (mBitmapTexture != null) {
            if (mBitmapTexture.getBitmap() != null) {
                mBitmapTexture.getBitmap().recycle();
            }
            mBitmapTexture.recycle();
        }
    }
    
    private BitmapTexture getBitmapTexture(Bitmap bitmap) {
        if (bitmap == null || bitmap.isRecycled()) return mBitmapTexture;
    /*    
        if (mBitmapTexture != null 
                && mBitmapTexture.getBitmap() != null 
                && mBitmapTexture.getBitmap() != bitmap) {
            recycleTexture();
        }*/
        mBitmapTexture = new BitmapTexture(bitmap);
        return mBitmapTexture;
    }
    
    public void setOnTextureChangedListener(OnTextureChangedListener listener) {
        mChangedListener = listener;
    }
    
    public interface OnTextureChangedListener {
        public void onTextureChanged(BitmapTexture texture);
    }
    
    private class AnimationHandler extends Handler {
        public static final int START = 0;
        public static final int UPDATE = 1;
        public static final int STOP = 2;
        
        private GifDecoder mGifDecoder = null;

        public AnimationHandler(Looper looper) {
            super(looper);
        }

        public void handleMessage(Message msg) {
            switch (msg.what) {
            case START:
                if (mInputStream == null) {
                    Log.e(TAG, "set gif input stream first!");
                    break;
                }
                
                if (mGifDecoder != null) {
                    mGifDecoder.free();
                }
                Log.e(TAG, "gif animation START");
                mGifDecoder = new GifDecoder(mInputStream, new GifAction() {
                    public void parseOk(boolean parseStatus, int frameIndex) {
                        if (!parseStatus) {
                            Log.e(TAG, "fail to parse gif image!");
                        } else {
                            removeMessages(UPDATE);
                            sendEmptyMessage(UPDATE);
                        }
                    }
                });
                try {
                    mGifDecoder.run();
                } catch (OutOfMemoryError e) {
                    e.printStackTrace();
                    System.gc();
                }
                break;
            case UPDATE:
                long delay = 100L;
                GifFrame frame = mGifDecoder.next();
                if ((frame != null) && (frame.image != null)) {
                    Bitmap bitmap = frame.image.copy(frame.image.getConfig(), false);
                    mChangedListener.onTextureChanged(getBitmapTexture(bitmap));
                    if (frame.delay > delay)
                        delay = frame.delay;
                } else {
                    mGifDecoder.reset();
                }
                
                removeMessages(UPDATE);
                sendEmptyMessageDelayed(UPDATE, delay);
                break;
            case STOP:
                Log.e(TAG, "gif animation STOP");
                removeMessages(UPDATE);
                if (mGifDecoder != null)
                    mGifDecoder.free();
                mGifDecoder = null;
                break;
            default:
            }
        }
    }
}