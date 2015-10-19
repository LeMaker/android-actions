/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.webgl;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.cts.util.NullWebViewUtils;
import android.os.Bundle;
import android.util.Log;
import android.webgl.cts.R;
import android.webkit.WebView;
import android.webkit.JavascriptInterface;
import android.webkit.WebViewClient;
import android.widget.Toast;
import java.lang.Override;
import java.io.InputStream;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * A simple activity for testing WebGL Conformance with WebView.
 */
public class WebGLActivity extends Activity {

    Semaphore mFinished = new Semaphore(0, false);
    Semaphore mDestroyed = new Semaphore(0, false);
    String mWebGlHarnessUrl;
    WebView mWebView;

    // The following members are synchronized.
    String mWebGLTestUrl;
    boolean mPassed = true;
    StringBuilder mMessage = new StringBuilder("\n");

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        mWebGlHarnessUrl = "file://" + getCacheDir() + "/harness.html";
        try {
            mWebView = new WebView(this);
        } catch (Exception e) {
            NullWebViewUtils.determineIfWebViewAvailable(this, e);
        }

        if (mWebView == null) {
            return;
        }

        mWebView.getSettings().setJavaScriptEnabled(true);
        mWebView.getSettings().setAllowFileAccessFromFileURLs(true);
        mWebView.getSettings().setMediaPlaybackRequiresUserGesture(false);
        mWebView.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView webView, String url) {
                return false;
            }
        });

        mWebView.addJavascriptInterface(new Object() {
            @JavascriptInterface
            public String getUrlToTest() {
                synchronized(WebGLActivity.this) {
                    return mWebGLTestUrl;
                }
            }

            @JavascriptInterface
            public void reportResults(String type, boolean success, String message) {
                synchronized(WebGLActivity.this) {
                    mMessage.append((success ? "PASS " : "FAIL ") + message + "\n");
                    mPassed &= success;
                }
            }

            @JavascriptInterface
            public void notifyFinished() {
                mFinished.release();
            }

            @JavascriptInterface
            public void alert(String string) {
                Log.i(mWebGLTestUrl, string);
            }
        }, "WebGLCallback");
        setContentView(mWebView);
    }

    public void navigateToTest(String url) throws Exception {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        synchronized(WebGLActivity.this) {
            mWebGLTestUrl = url;
        }

        // Load harness.html, which will load mWebGLTestUrl in an <iframe>.
        runOnUiThread(new Runnable() {
            public void run() {
                mWebView.loadUrl(mWebGlHarnessUrl);
            }
        });

        // Wait on test completion.
        boolean finished = mFinished.tryAcquire(60, TimeUnit.SECONDS);
        String message;
        synchronized(WebGLActivity.this) {
            message = mMessage.toString();
        }

        // Destroy the webview and wait for it.
        runOnUiThread(new Runnable() {
            public void run() {
                mWebView.destroy();
                finish();
                mDestroyed.release();
            }
        });
        mDestroyed.acquire();

        if (!finished)
            throw new Exception("\n" + url + "\n Test timed-out after 60 seconds: " + message);
        if(!mPassed)
            throw new Exception("\n" + url + "\n Test failed: " + message);
    }
}
