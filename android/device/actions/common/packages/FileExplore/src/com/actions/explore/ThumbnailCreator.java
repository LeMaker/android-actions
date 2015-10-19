/*
  @author lizihao@actions-semi.com
*/

package com.actions.explore;

import java.lang.ref.SoftReference;
import java.util.ArrayList;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Handler;
import android.util.Log;
import android.widget.ImageView;
import android.media.ExifInterface;

public class ThumbnailCreator {
	private int mWidth;
	private int mHeight;
	private ArrayList<Bitmap> mCacheBitmap;
	private SoftReference<Bitmap> mThumb;

	public ThumbnailCreator(int width, int height) {
		mWidth = width;
		mHeight = height;
		mCacheBitmap = new ArrayList<Bitmap>(100);
	}
	
	public Bitmap hasBitmapCached(int index) {
		if(mCacheBitmap.isEmpty())
			return null;
		
		try {
			return mCacheBitmap.get(index);
			
		} catch (IndexOutOfBoundsException e) {
			e.printStackTrace();
			return null;
		}		
	}
	
	public void clearBitmapCache() {
		mCacheBitmap.clear();
	}
	
	public void setBitmapToImageView(final String imageSrc, 
									 final Handler handle, 
									 final ImageView icon,
									 final boolean isJPG,
									 final int index) {

		Thread thread = new Thread() {
			public void run() {
				synchronized (this) {
					if(isJPG) {
						try{
							ExifInterface mExif = new ExifInterface(imageSrc);
							if(mExif != null) {
								byte[] thumbData = mExif.getThumbnail();
								if(thumbData == null)
								{
									BitmapFactory.Options options = new BitmapFactory.Options();
							        options.inJustDecodeBounds = true;
							        Bitmap mBitmap;
							        mBitmap = BitmapFactory.decodeFile(imageSrc, options); 

							        int be = (int)( Math.max(options.outWidth,options.outHeight) / 64);
							        if (be <= 0)
							            be = 1;
							        options.inSampleSize = be;
							        
							        options.inJustDecodeBounds = false;
							        
							        mBitmap=BitmapFactory.decodeFile(imageSrc,options);
							        mThumb = new SoftReference<Bitmap>(Bitmap.createScaledBitmap(
					 						  mBitmap,
					 						  mWidth,
					 						  mHeight,
					 						  false));
								}
								else
								{
									Bitmap bmp = BitmapFactory.decodeByteArray(thumbData, 0, thumbData.length);
									if(bmp != null)
									{
										mThumb = new SoftReference<Bitmap>(Bitmap.createScaledBitmap(
												bmp,
												mWidth,
												mHeight,
												false));
									}
									else
										mThumb = null;
								}
							} 
						}
						catch (Exception e){
							Log.d(imageSrc,"can't get exif");
						}
					}
					else {
						try{
							BitmapFactory.Options options = new BitmapFactory.Options();
					        options.inJustDecodeBounds = true;
					        Bitmap mBitmap;
					        mBitmap = BitmapFactory.decodeFile(imageSrc, options); 

					        Log.d(imageSrc, String.valueOf(options.outWidth) + ":" + String.valueOf(options.outHeight) + "is not a jpg file");
					        int be = (int)( Math.max(options.outWidth,options.outHeight) / 64);
					        if (be <= 0)
					            be = 1;
					        options.inSampleSize = be;
					        
					        options.inJustDecodeBounds = false;
					        
					        mBitmap=BitmapFactory.decodeFile(imageSrc,options);
					        if(mBitmap != null)
					        {
					        	mThumb = new SoftReference<Bitmap>(Bitmap.createScaledBitmap(
			 						  mBitmap,
			 						  mWidth,
			 						  mHeight,
			 						  false));
					        }
					        else
					        {
					        	mThumb = null;
					        }
						}catch(Exception e){
							
						}
					}
					if(mThumb == null)
					{
						return;
					}
					//mCacheBitmap.add(index,mThumb.get());
					int size = mCacheBitmap.size();
					int i;
					if(size <= index){
						for (i=size;i<=index;i++) {
							mCacheBitmap.add(null);
						}
					}
					mCacheBitmap.set(index,mThumb.get());
					Log.d("Thumbnail",String.valueOf(index) + ":set index success");
					final Bitmap src = mThumb.get();
					handle.post(new Runnable() {
						public void run() {
							icon.setImageBitmap(src);
						}
					});
				}
			}
		};
		
		thread.start();
	}
}