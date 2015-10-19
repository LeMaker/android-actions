package com.android.settings;

import android.app.Activity;
import android.os.Bundle;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.webkit.WebView;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import java.io.File;

public class G3information extends Activity{

	private static final String TAG = "G3information";
    private static final String DEFAULT_LICENSE_PATH = "/system/etc/NOTICE.html";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final String path = DEFAULT_LICENSE_PATH;

        final File file = new File(path);
        if (!file.exists() || file.length() == 0) {
            Log.e(TAG, "3G information file " + path + " does not exist");
            showErrorAndFinish();
            return;
        }

        // Kick off external viewer due to WebView security restrictions; we
        // carefully point it at HTMLViewer, since it offers to decompress
        // before viewing.
        final Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setDataAndType(Uri.fromFile(file), "text/html");
        intent.putExtra(Intent.EXTRA_TITLE, getString(R.string.settings_3g_dongle_activity_title));
        intent.addCategory(Intent.CATEGORY_DEFAULT);
        intent.setPackage("com.android.htmlviewer");

        try {
            startActivity(intent);
            finish();
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "Failed to find viewer", e);
            showErrorAndFinish();
        }
    }

    private void showErrorAndFinish() {
        Toast.makeText(this, R.string.settings_3g_information_activity_title, Toast.LENGTH_LONG)
                .show();
        finish();
    }
}
