package com.actions.logcat;

import android.app.Activity;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.widget.TextView;
import com.actions.logcat.R;

public class AboutActivity extends Activity{

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.about);
		TextView logger = (TextView)findViewById(R.id.about_logger);
		TextView version = (TextView)findViewById(R.id.about_version);
		logger.setText(R.string.aboutlogger);
		try {
			version.setText("version:" + getPackageManager().getPackageInfo(getPackageName(), 0).versionName);
		} catch (NameNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
