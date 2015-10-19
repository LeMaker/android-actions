package com.actions.testtvoutscale;

import android.app.Activity;
import android.os.Bundle;
import com.actions.hardware.DisplayManager;
import android.os.RemoteException;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;

public class TestTvoutScale extends Activity implements OnClickListener {
    private final static String LOG_TAG = "testtvoutscale";

    private DisplayManager mDisplayManager = null;

    private EditText valueText = null;
    private Button okButton = null;
    private Button clearButton = null;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        Log.e(LOG_TAG, "enter TestTvoutScale's onCreate\n");
        mDisplayManager = new DisplayManager();
        if (mDisplayManager == null) {
            Log.e(LOG_TAG, "mDisplayManager == null");
        }

        mDisplayManager.testInit();

        valueText = (EditText) findViewById(R.id.edit_value);
        okButton = (Button) findViewById(R.id.button_ok);
        clearButton = (Button) findViewById(R.id.button_clear);
        okButton.setOnClickListener(this);
        clearButton.setOnClickListener(this);
        Log.i(LOG_TAG, "TestTvoutScale Activity Created");
    }

    @Override
    public void onClick(View v) {
        if (v.equals(okButton)) {
            String text = valueText.getText().toString();
            int index = text.indexOf(",");
            String xScale = text.substring(0, index);
            String yScale = text.substring(index + 1);

            int xVal = Integer.parseInt(xScale);
            int yVal = Integer.parseInt(yScale);

            mDisplayManager.setTvDisplayScale(xVal, yVal);

        } else if (v.equals(clearButton)) {
            String text = "";
            valueText.setText(text);
        }
    }
}
