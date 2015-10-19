package com.actions.explore;

import java.io.IOException;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ExifInterface;
import android.util.Log;

public class CropImage {
	private Context context;
	private Intent intent;
	private int outputX = 0;
	private int outputY = 0;
	private String srcImg = null;
	private static final String TAG = CropImage.class.getSimpleName();
	public CropImage(Context context,Intent intent, String srcImage){
		this.context = context;
		outputX = intent.getIntExtra("outputX", 48);
		outputY = intent.getIntExtra("outputY", 48);
		srcImg = srcImage;
		//Log.d(TAG,"srcImg:" + srcImg + "  outputX:" + outputX + "  outputY:" + outputY);
	}
	
	public Intent saveResourceToIntent(){
		Intent ret = new Intent();
		ThumbnailCreator creator = new ThumbnailCreator(outputX, outputY);
		Bitmap bmp = creator.createThumbnail(srcImg);
		if(bmp == null) Log.e(TAG, "saveResourceToIntent()  get thumbnail res fail");
		ret.putExtra("data", bmp);
		return ret;
	}
	
	private class ThumbnailCreator 
	{
		private int width;
		private int height;
		private static final String TAG = "ThumbnailCreator";
		public ThumbnailCreator(int width, int height)
		{
			this.width = width;
			this.height = height;
		}
		
		public Bitmap createThumbnail(String imageSrc) 
		{
			boolean isJPG = false;
			Bitmap thumbnail = null;
			try
			{
				String ext = imageSrc.substring(imageSrc.lastIndexOf(".") + 1);
				if(ext.equalsIgnoreCase("jpg") || ext.equalsIgnoreCase("jpeg"))
				{
					isJPG = true;
				}
			}catch(IndexOutOfBoundsException e)
			{
				e.printStackTrace();
				return null;
			}
			
			if(isJPG)
			{
				try {
					ExifInterface mExif = null;
					mExif = new ExifInterface(imageSrc);
					if(mExif != null)
					{
						byte[] thumbData = mExif.getThumbnail();
						if(thumbData == null)
						{
							thumbnail = createThumbnailByOptions(imageSrc);
						}
						else
						{
							thumbnail = BitmapFactory.decodeByteArray(thumbData, 0, thumbData.length);
						}
					}
					else
					{
						thumbnail = createThumbnailByOptions(imageSrc);
					}
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
					return null;
				}
			}
			else
			{
				thumbnail = createThumbnailByOptions(imageSrc);
			}
			return thumbnail;
		}
		
		private Bitmap createThumbnailByOptions(String imageSrc)
		{
			Bitmap thumb = null;
			BitmapFactory.Options options = new BitmapFactory.Options();
			options.inJustDecodeBounds = true;
			thumb = BitmapFactory.decodeFile(imageSrc, options);
			int be = (int) (Math.min(options.outWidth / width, options.outHeight / height));
			if(be <= 0)
				be = 1;
			options.inSampleSize = be;
			options.inJustDecodeBounds = false;
			thumb = BitmapFactory.decodeFile(imageSrc, options);
			if(thumb == null)
			{
				return null;
			}
			thumb = Bitmap.createScaledBitmap(thumb, width, height, false);
			Log.d("MediaProvider","image:" + String.valueOf(width) + "*" + String.valueOf(height));
			return thumb;
		}
	}
}