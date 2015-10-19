package com.actions.sensor.calib;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.hardware.SensorEvent;
import android.util.AttributeSet;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.WindowManager;

public class SensorHost extends View implements OnTouchListener {

	public final String TAG = "SensorHost";

	float vX = 0.0F;
	float vY = 0.0F;
	float vZ = 0.0F;
	Display mDisplay = null;
	Paint mPaint = null;
	RectF mOval = null;

	public SensorHost(Context ctx, AttributeSet attrs) {
		this(ctx, attrs, 0);
	}

	public SensorHost(Context ctx, AttributeSet attrs, int param) {
		super(ctx, attrs, param);
		setOnTouchListener(this);

		WindowManager wm = (WindowManager) ctx
				.getSystemService(Context.WINDOW_SERVICE);
		mDisplay = wm.getDefaultDisplay();

		mPaint = new Paint();
		mOval = new RectF();
	}

	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		int h = MeasureSpec.getSize(heightMeasureSpec);
		int w = MeasureSpec.getSize(widthMeasureSpec);
		int r = Math.min(w, h) * 4 / 5;
		setMeasuredDimension(r, r);
	}

	public void onDraw(Canvas c) {
		super.onDraw(c);

		int w = getWidth();
		int h = getHeight();

		c.translate(w / 2, h / 2);

		mPaint.setAntiAlias(true);
		mPaint.setColor(-1);
		mPaint.setStyle(Paint.Style.STROKE);
		mPaint.setAlpha(255);

		int r = Math.min(w, h);
		mOval.left = -r / 2;
		mOval.top = -r / 2;
		mOval.right = r / 2;
		mOval.bottom = r / 2;

		c.drawArc(mOval, 0, 360, false, mPaint);
		c.drawLine(-r / 2, 0.0F, r / 2, 0.0F, mPaint);
		c.drawLine(0.0F, -r / 2, 0.0F, r / 2, mPaint);

		int k = mDisplay.getRotation() * 90;
		float f2;
		if (k == 90) {
			f2 = vX;
			vX = (-vY);
			vY = f2;
		}
		if (k == 180) {
			vX = (-vX);
			vY = (-vY);
		}
		if (k == 270) {
			f2 = vX;
			vX = vY;
			vY = (-f2);
		}

		mPaint.setStyle(Paint.Style.FILL);
		c.drawCircle(-w / 2 * vX / 10.0F, h / 2 * vY / 10.0F, 20.0F - vZ,
				mPaint);
	}

	public void onSensorChanged(SensorEvent e) {
		if (e.values.length == 3) {
			vX = e.values[0];
			vY = e.values[1];
			vZ = e.values[2];
			invalidate();
		}
	}

	public boolean onTouch(View paramView, MotionEvent paramMotionEvent) {
		return false;
	}
}
