package com.bumptech.glide.manager;

import android.annotation.TargetApi;
import android.app.Fragment;
import android.util.Log;

import com.bumptech.glide.RequestManager;

@TargetApi(11)
public class RequestManagerFragment extends Fragment {
    private RequestManager requestManager;
    private static String TAG = "RequestManagerFragment";

    public void setRequestManager(RequestManager requestManager) {
        this.requestManager = requestManager;
    }

    public RequestManager getRequestManager() {
        return requestManager;
    }

    @Override
    public void onStart() {
        super.onStart();
        if (requestManager != null) {
            requestManager.onStart();
        }
    }

    @Override
    public void onStop() {
        super.onStop();
        if (requestManager != null) {
            try {
                requestManager.onStop();
            } catch (RuntimeException e) {
                Log.e(TAG, "exception during onStop", e);
            }
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (requestManager != null) {
            try {
                requestManager.onDestroy();
            } catch (RuntimeException e) {
                Log.e(TAG, "exception during onDestroy", e);
            }
        }
    }
}
