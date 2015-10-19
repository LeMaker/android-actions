package com.android.gallery3d.util;

import java.io.Closeable;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.BitmapFactory.Options;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.net.Uri;
import android.util.Log;

public class BitmapUtils {
    private static final int DEFAULT_COMPRESS_QUALITY = 90;
    private static final String[] IMAGE_PROJECTION;
    private static final int INDEX_ORIENTATION = 0;
    private static final String TAG = "BitmapUtils";
    private final Context mContext;

    static {
        String[] arrayOfString = new String[1];
        arrayOfString[0] = "orientation";
        IMAGE_PROJECTION = arrayOfString;
    }

    public BitmapUtils(Context context) {
        mContext = context;
    }

    private void closeStream(Closeable closeable) {
        if (closeable == null)
            return;
        
        try {
            closeable.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return;
    }

    private static Bitmap createBitmap(Bitmap bitmap, Matrix matrix) {
        return Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
    }

    private Bitmap decodeBitmap(Uri uri, int width, int height) {
        InputStream is = null;
        Bitmap bitmap = null;
        try {
            Rect rect = getBitmapBounds(uri);
            int sampleSize = Math.min(Math.max(rect.width() / width, rect.height() / height),
                    Math.max(rect.width() / height, rect.height() / width));

            Options options = new BitmapFactory.Options();
            options.inSampleSize = Math.max(sampleSize, 1);
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            is = mContext.getContentResolver().openInputStream(uri);
            bitmap = BitmapFactory.decodeStream(is, null, options);
            closeStream(is);

            Bitmap tmpBitmap = null;
            if ((bitmap != null) && (bitmap.getConfig() != Bitmap.Config.ARGB_8888)) {
                tmpBitmap = bitmap.copy(Bitmap.Config.ARGB_8888, true);
                bitmap.recycle();
            }

            if (tmpBitmap != null) {
                float f = Math.max(Math.min(width / tmpBitmap.getWidth(), height / tmpBitmap.getHeight()),
                        Math.min(height / tmpBitmap.getWidth(), width / tmpBitmap.getHeight()));
                if (f < 1.0F) {
                    Matrix matrix = new Matrix();
                    matrix.setScale(f, f);
                    bitmap = createBitmap(tmpBitmap, matrix);
                    tmpBitmap.recycle();
                    return bitmap;
                } else {
                    return tmpBitmap;
                }
            }
        } catch (FileNotFoundException e) {
            Log.e("BitmapUtils", "FileNotFoundException: " + uri);
            closeStream(is);
        } finally {
            closeStream(is);
        }
        return null;

    }

    private Rect getBitmapBounds(Uri paramUri) {
        Rect rect = new Rect();
        InputStream is = null;
        try {
            is = mContext.getContentResolver().openInputStream(paramUri);
            Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            Bitmap bitmap = BitmapFactory.decodeStream(is, null, options);
            rect.right = options.outWidth;
            rect.bottom = options.outHeight;
            if (bitmap != null) 
                bitmap.recycle();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            closeStream(is);
        } finally {
            closeStream(is);
        }
        return rect;
    }

    
    private int getOrientation(Uri uri) {
        ContentResolver resolver = mContext.getContentResolver();
        Cursor cursor = resolver.query(uri, null, null, null, null);
        if (cursor == null) {
            return 0;
        }
        
        int orientation = cursor.getInt(cursor.getColumnIndexOrThrow("orientation"));
        cursor.close();
        return orientation;
    }

    public Bitmap getBitmap(Uri uri, int width, int height) {
        Bitmap tmp = decodeBitmap(uri, width, height);
        if (tmp == null) {
            return null;
        }
        
        int orientation = getOrientation(uri);
        Matrix matrix = new Matrix();
        matrix.setRotate(orientation);
        
        Bitmap bitmap = createBitmap(tmp, matrix);
        tmp.recycle();
        return bitmap;
    }

    public Bitmap getNotRotateBitmap(Uri uri, int width, int height) {
        return decodeBitmap(uri, width, height);
    }

    
    public File saveBitmap(Bitmap bitmap, String name, CompressFormat format) {
        File cacheDir = mContext.getCacheDir();
        File file = new File(cacheDir.getPath() + name);
        
        FileOutputStream fos = null;
        try {
            boolean created = file.createNewFile();
            if (created) {
                fos = new FileOutputStream(file);
                boolean compressed = bitmap.compress(format, 100, fos);
                if (compressed) {
                    return file;
                }
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }
}