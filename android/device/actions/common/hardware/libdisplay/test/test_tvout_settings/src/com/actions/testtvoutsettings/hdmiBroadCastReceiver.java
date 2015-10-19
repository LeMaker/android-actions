package com.actions.testtvoutsettings;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class hdmiBroadCastReceiver extends BroadcastReceiver {
    private final String TAG = "testTvoutSettins";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.e(TAG, "receive hdmi hot plug broadcast \n");
        abortBroadcast();
    }

}
