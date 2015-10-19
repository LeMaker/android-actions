/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.calculator2;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.app.Activity;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.view.ViewPager;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.view.View.OnLongClickListener;
import android.view.ViewAnimationUtils;
import android.view.ViewGroupOverlay;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.widget.Button;
import android.widget.TextView;
import java.io.File;
import android.os.FileUtils;
import java.io.IOException;
import android.util.Log;
import android.os.SystemProperties;

import android.content.Intent;
import android.content.ComponentName;
import java.util.List;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;

import com.android.calculator2.CalculatorEditText.OnTextSizeChangeListener;
import com.android.calculator2.CalculatorExpressionEvaluator.EvaluateCallback;

public class Calculator extends Activity
        implements OnTextSizeChangeListener, EvaluateCallback, OnLongClickListener {
	
	private static final String LOG_TAG = "Calculator";
    private static final String NAME = Calculator.class.getName();

    // instance state keys
    private static final String KEY_CURRENT_STATE = NAME + "_currentState";
    private static final String KEY_CURRENT_EXPRESSION = NAME + "_currentExpression";
    
    //ActionsCode(authro:lizihao, comment:define value)
    private String mProductLineTestMode = "\u03c0315";
    private String mPCBATestMode = "\u03c0314";
    private String mAgingTestMode = "\u03c0316";
    private String mLoggerTestMode = "\u03c0317";
    private String mActionsCameraMode = "\u03c0318";
    private String mOpenDualLogo = "\u03c0312";
    private String mCloseDualLogo = "\u03c0313";
    private String mDisplaySettings = "\u03c0319";
    private String mBatteryDebug = "\u03c0320";
	
	private static final String SWITCH_LOGO_FILE = "/sys/miscinfo/infos/bf";
    private static final String BOOT_PIC1 = "/misc/boot_logo.bmp.gz";
    private static final String BOOT_PIC2 = "/misc/boot_logo_1.bmp.gz";
            
    /**
     * Constant for an invalid resource id.
     */
    public static final int INVALID_RES_ID = -1;

    private enum CalculatorState {
        INPUT, EVALUATE, RESULT, ERROR
    }

    private final TextWatcher mFormulaTextWatcher = new TextWatcher() {
        @Override
        public void beforeTextChanged(CharSequence charSequence, int start, int count, int after) {
        }

        @Override
        public void onTextChanged(CharSequence charSequence, int start, int count, int after) {
        }

        @Override
        public void afterTextChanged(Editable editable) {
            setState(CalculatorState.INPUT);
            mEvaluator.evaluate(editable, Calculator.this);

            //ActionsCode(authro:lizihao, comment:check test mode
            checkTestMode(editable);
        }
    };

    private final OnKeyListener mFormulaOnKeyListener = new OnKeyListener() {
        @Override
        public boolean onKey(View view, int keyCode, KeyEvent keyEvent) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_NUMPAD_ENTER:
                case KeyEvent.KEYCODE_ENTER:
                    if (keyEvent.getAction() == KeyEvent.ACTION_UP) {
                        onEquals();
                    }
                    // ignore all other actions
                    return true;
            }
            return false;
        }
    };

    private final Editable.Factory mFormulaEditableFactory = new Editable.Factory() {
        @Override
        public Editable newEditable(CharSequence source) {
            final boolean isEdited = mCurrentState == CalculatorState.INPUT
                    || mCurrentState == CalculatorState.ERROR;
            return new CalculatorExpressionBuilder(source, mTokenizer, isEdited);
        }
    };

    private CalculatorState mCurrentState;
    private CalculatorExpressionTokenizer mTokenizer;
    private CalculatorExpressionEvaluator mEvaluator;

    private View mDisplayView;
    private CalculatorEditText mFormulaEditText;
    private CalculatorEditText mResultEditText;
    private ViewPager mPadViewPager;
    private View mDeleteButton;
    private View mClearButton;
    private View mEqualButton;

    private Animator mCurrentAnimator;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_calculator);

        mDisplayView = findViewById(R.id.display);
        mFormulaEditText = (CalculatorEditText) findViewById(R.id.formula);
        mResultEditText = (CalculatorEditText) findViewById(R.id.result);
        mPadViewPager = (ViewPager) findViewById(R.id.pad_pager);
        mDeleteButton = findViewById(R.id.del);
        mClearButton = findViewById(R.id.clr);

        mEqualButton = findViewById(R.id.pad_numeric).findViewById(R.id.eq);
        if (mEqualButton == null || mEqualButton.getVisibility() != View.VISIBLE) {
            mEqualButton = findViewById(R.id.pad_operator).findViewById(R.id.eq);
        }

        mTokenizer = new CalculatorExpressionTokenizer(this);
        mEvaluator = new CalculatorExpressionEvaluator(mTokenizer);

        savedInstanceState = savedInstanceState == null ? Bundle.EMPTY : savedInstanceState;
        setState(CalculatorState.values()[
                savedInstanceState.getInt(KEY_CURRENT_STATE, CalculatorState.INPUT.ordinal())]);
        mFormulaEditText.setText(mTokenizer.getLocalizedExpression(
                savedInstanceState.getString(KEY_CURRENT_EXPRESSION, "")));
        mEvaluator.evaluate(mFormulaEditText.getText(), this);

        mFormulaEditText.setEditableFactory(mFormulaEditableFactory);
        mFormulaEditText.addTextChangedListener(mFormulaTextWatcher);
        mFormulaEditText.setOnKeyListener(mFormulaOnKeyListener);
        mFormulaEditText.setOnTextSizeChangeListener(this);
        mDeleteButton.setOnLongClickListener(this);
    }

    @Override
    protected void onSaveInstanceState(@NonNull Bundle outState) {
        // If there's an animation in progress, end it immediately to ensure the state is
        // up-to-date before it is serialized.
        if (mCurrentAnimator != null) {
            mCurrentAnimator.end();
        }

        super.onSaveInstanceState(outState);

        outState.putInt(KEY_CURRENT_STATE, mCurrentState.ordinal());
        outState.putString(KEY_CURRENT_EXPRESSION,
                mTokenizer.getNormalizedExpression(mFormulaEditText.getText().toString()));
    }

    private void setState(CalculatorState state) {
        if (mCurrentState != state) {
            mCurrentState = state;

            if (state == CalculatorState.RESULT || state == CalculatorState.ERROR) {
                mDeleteButton.setVisibility(View.GONE);
                mClearButton.setVisibility(View.VISIBLE);
            } else {
                mDeleteButton.setVisibility(View.VISIBLE);
                mClearButton.setVisibility(View.GONE);
            }

            if (state == CalculatorState.ERROR) {
                final int errorColor = getResources().getColor(R.color.calculator_error_color);
                mFormulaEditText.setTextColor(errorColor);
                mResultEditText.setTextColor(errorColor);
                getWindow().setStatusBarColor(errorColor);
            } else {
                mFormulaEditText.setTextColor(
                        getResources().getColor(R.color.display_formula_text_color));
                mResultEditText.setTextColor(
                        getResources().getColor(R.color.display_result_text_color));
                getWindow().setStatusBarColor(
                        getResources().getColor(R.color.calculator_accent_color));
            }
        }
    }

    @Override
    public void onBackPressed() {
        if (mPadViewPager == null || mPadViewPager.getCurrentItem() == 0) {
            // If the user is currently looking at the first pad (or the pad is not paged),
            // allow the system to handle the Back button.
            super.onBackPressed();
        } else {
            // Otherwise, select the previous pad.
            mPadViewPager.setCurrentItem(mPadViewPager.getCurrentItem() - 1);
        }
    }

    @Override
    public void onUserInteraction() {
        super.onUserInteraction();

        // If there's an animation in progress, end it immediately to ensure the state is
        // up-to-date before the pending user interaction is handled.
        if (mCurrentAnimator != null) {
            mCurrentAnimator.end();
        }
    }

    public void onButtonClick(View view) {
        switch (view.getId()) {
            case R.id.eq:
                onEquals();
                break;
            case R.id.del:
                onDelete();
                break;
            case R.id.clr:
                onClear();
                break;
            case R.id.fun_cos:
            case R.id.fun_ln:
            case R.id.fun_log:
            case R.id.fun_sin:
            case R.id.fun_tan:
                // Add left parenthesis after functions.
                mFormulaEditText.append(((Button) view).getText() + "(");
                break;
            default:
                mFormulaEditText.append(((Button) view).getText());
                break;
        }
    }

    @Override
    public boolean onLongClick(View view) {
        if (view.getId() == R.id.del) {
            onClear();
            return true;
        }
        return false;
    }

    @Override
    public void onEvaluate(String expr, String result, int errorResourceId) {
        if (mCurrentState == CalculatorState.INPUT) {
            mResultEditText.setText(result);
        } else if (errorResourceId != INVALID_RES_ID) {
            onError(errorResourceId);
        } else if (!TextUtils.isEmpty(result)) {
            onResult(result);
        } else if (mCurrentState == CalculatorState.EVALUATE) {
            // The current expression cannot be evaluated -> return to the input state.
            setState(CalculatorState.INPUT);
        }

        mFormulaEditText.requestFocus();
    }

    @Override
    public void onTextSizeChanged(final TextView textView, float oldSize) {
        if (mCurrentState != CalculatorState.INPUT) {
            // Only animate text changes that occur from user input.
            return;
        }

        // Calculate the values needed to perform the scale and translation animations,
        // maintaining the same apparent baseline for the displayed text.
        final float textScale = oldSize / textView.getTextSize();
        final float translationX = (1.0f - textScale) *
                (textView.getWidth() / 2.0f - textView.getPaddingEnd());
        final float translationY = (1.0f - textScale) *
                (textView.getHeight() / 2.0f - textView.getPaddingBottom());

        final AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.playTogether(
                ObjectAnimator.ofFloat(textView, View.SCALE_X, textScale, 1.0f),
                ObjectAnimator.ofFloat(textView, View.SCALE_Y, textScale, 1.0f),
                ObjectAnimator.ofFloat(textView, View.TRANSLATION_X, translationX, 0.0f),
                ObjectAnimator.ofFloat(textView, View.TRANSLATION_Y, translationY, 0.0f));
        animatorSet.setDuration(getResources().getInteger(android.R.integer.config_mediumAnimTime));
        animatorSet.setInterpolator(new AccelerateDecelerateInterpolator());
        animatorSet.start();
    }

    private void onEquals() {
        if (mCurrentState == CalculatorState.INPUT) {
            setState(CalculatorState.EVALUATE);
            mEvaluator.evaluate(mFormulaEditText.getText(), this);
        }
    }

    private void onDelete() {
        // Delete works like backspace; remove the last character from the expression.
        final Editable formulaText = mFormulaEditText.getEditableText();
        final int formulaLength = formulaText.length();
        if (formulaLength > 0) {
            formulaText.delete(formulaLength - 1, formulaLength);
        }
    }

    private void reveal(View sourceView, int colorRes, AnimatorListener listener) {
        final ViewGroupOverlay groupOverlay =
                (ViewGroupOverlay) getWindow().getDecorView().getOverlay();

        final Rect displayRect = new Rect();
        mDisplayView.getGlobalVisibleRect(displayRect);

        // Make reveal cover the display and status bar.
        final View revealView = new View(this);
        revealView.setBottom(displayRect.bottom);
        revealView.setLeft(displayRect.left);
        revealView.setRight(displayRect.right);
        revealView.setBackgroundColor(getResources().getColor(colorRes));
        groupOverlay.add(revealView);

        final int[] clearLocation = new int[2];
        sourceView.getLocationInWindow(clearLocation);
        clearLocation[0] += sourceView.getWidth() / 2;
        clearLocation[1] += sourceView.getHeight() / 2;

        final int revealCenterX = clearLocation[0] - revealView.getLeft();
        final int revealCenterY = clearLocation[1] - revealView.getTop();

        final double x1_2 = Math.pow(revealView.getLeft() - revealCenterX, 2);
        final double x2_2 = Math.pow(revealView.getRight() - revealCenterX, 2);
        final double y_2 = Math.pow(revealView.getTop() - revealCenterY, 2);
        final float revealRadius = (float) Math.max(Math.sqrt(x1_2 + y_2), Math.sqrt(x2_2 + y_2));

        final Animator revealAnimator =
                ViewAnimationUtils.createCircularReveal(revealView,
                        revealCenterX, revealCenterY, 0.0f, revealRadius);
        revealAnimator.setDuration(
                getResources().getInteger(android.R.integer.config_longAnimTime));

        final Animator alphaAnimator = ObjectAnimator.ofFloat(revealView, View.ALPHA, 0.0f);
        alphaAnimator.setDuration(
                getResources().getInteger(android.R.integer.config_mediumAnimTime));
        alphaAnimator.addListener(listener);

        final AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.play(revealAnimator).before(alphaAnimator);
        animatorSet.setInterpolator(new AccelerateDecelerateInterpolator());
        animatorSet.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animator) {
                groupOverlay.remove(revealView);
                mCurrentAnimator = null;
            }
        });

        mCurrentAnimator = animatorSet;
        animatorSet.start();
    }

    private void onClear() {
        if (TextUtils.isEmpty(mFormulaEditText.getText())) {
            return;
        }

        final View sourceView = mClearButton.getVisibility() == View.VISIBLE
                ? mClearButton : mDeleteButton;
        reveal(sourceView, R.color.calculator_accent_color, new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                mFormulaEditText.getEditableText().clear();
            }
        });
    }

    private void onError(final int errorResourceId) {
        if (mCurrentState != CalculatorState.EVALUATE) {
            // Only animate error on evaluate.
            mResultEditText.setText(errorResourceId);
            return;
        }

        reveal(mEqualButton, R.color.calculator_error_color, new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                setState(CalculatorState.ERROR);
                mResultEditText.setText(errorResourceId);
            }
        });
    }

    private void onResult(final String result) {
        // Calculate the values needed to perform the scale and translation animations,
        // accounting for how the scale will affect the final position of the text.
        final float resultScale =
                mFormulaEditText.getVariableTextSize(result) / mResultEditText.getTextSize();
        final float resultTranslationX = (1.0f - resultScale) *
                (mResultEditText.getWidth() / 2.0f - mResultEditText.getPaddingEnd());
        final float resultTranslationY = (1.0f - resultScale) *
                (mResultEditText.getHeight() / 2.0f - mResultEditText.getPaddingBottom()) +
                (mFormulaEditText.getBottom() - mResultEditText.getBottom()) +
                (mResultEditText.getPaddingBottom() - mFormulaEditText.getPaddingBottom());
        final float formulaTranslationY = -mFormulaEditText.getBottom();

        // Use a value animator to fade to the final text color over the course of the animation.
        final int resultTextColor = mResultEditText.getCurrentTextColor();
        final int formulaTextColor = mFormulaEditText.getCurrentTextColor();
        final ValueAnimator textColorAnimator =
                ValueAnimator.ofObject(new ArgbEvaluator(), resultTextColor, formulaTextColor);
        textColorAnimator.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator valueAnimator) {
                mResultEditText.setTextColor((int) valueAnimator.getAnimatedValue());
            }
        });

        final AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.playTogether(
                textColorAnimator,
                ObjectAnimator.ofFloat(mResultEditText, View.SCALE_X, resultScale),
                ObjectAnimator.ofFloat(mResultEditText, View.SCALE_Y, resultScale),
                ObjectAnimator.ofFloat(mResultEditText, View.TRANSLATION_X, resultTranslationX),
                ObjectAnimator.ofFloat(mResultEditText, View.TRANSLATION_Y, resultTranslationY),
                ObjectAnimator.ofFloat(mFormulaEditText, View.TRANSLATION_Y, formulaTranslationY));
        animatorSet.setDuration(getResources().getInteger(android.R.integer.config_longAnimTime));
        animatorSet.setInterpolator(new AccelerateDecelerateInterpolator());
        animatorSet.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                mResultEditText.setText(result);
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                // Reset all of the values modified during the animation.
                mResultEditText.setTextColor(resultTextColor);
                mResultEditText.setScaleX(1.0f);
                mResultEditText.setScaleY(1.0f);
                mResultEditText.setTranslationX(0.0f);
                mResultEditText.setTranslationY(0.0f);
                mFormulaEditText.setTranslationY(0.0f);

                // Finally update the formula to use the current result.
                mFormulaEditText.setText(result);
                setState(CalculatorState.RESULT);

                mCurrentAnimator = null;
            }
        });

        mCurrentAnimator = animatorSet;
        animatorSet.start();
    }
    
    /**
      *ActionsCode(author:lizihao, type:new_method)
    */
    void checkTestMode(Editable editor) {
		if (editor.toString().equals(mBatteryDebug)) {
            PackageManager pm = getPackageManager();
            Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
            List<ResolveInfo> resolveInfos = pm.queryIntentActivities(mainIntent, PackageManager.GET_UNINSTALLED_PACKAGES);
            for(ResolveInfo refo : resolveInfos){
                if(refo.activityInfo.packageName.equals("com.actions.batterydebugtools")){
					startBatteryDebugTest();
                    break;
                }
            }
        } else if (editor.toString().equals(mDisplaySettings)) {
            PackageManager pm = getPackageManager();
            Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
            List<ResolveInfo> resolveInfos = pm.queryIntentActivities(mainIntent, PackageManager.GET_UNINSTALLED_PACKAGES);
            for(ResolveInfo refo : resolveInfos){
                if(refo.activityInfo.packageName.equals("com.actions.displaysettings")){
                    startDisplaySettingsTest();
                    break;
                }
            }
        } else if (editor.toString().equals(mProductLineTestMode)) {
            PackageManager pm = getPackageManager();
            Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
            List<ResolveInfo> resolveInfos = pm.queryIntentActivities(mainIntent, PackageManager.GET_UNINSTALLED_PACKAGES);
            for(ResolveInfo refo : resolveInfos){
                if(refo.activityInfo.packageName.equals("com.test.ProductLineTest")){
                    startProductLineTest();
                    break;
                }
            }
        } else if(editor.toString().equals(mPCBATestMode)) {
            PackageManager pm = getPackageManager();
            Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
            List<ResolveInfo> resolveInfos = pm.queryIntentActivities(mainIntent, PackageManager.GET_UNINSTALLED_PACKAGES);
            for(ResolveInfo refo : resolveInfos){
                if(refo.activityInfo.packageName.equals("com.test.pcbatest")){
                    startPCBATest();
                    break;
                }
            }
        } else if(editor.toString().equals(mAgingTestMode)) {
            PackageManager pm = getPackageManager();
            Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
            List<ResolveInfo> resolveInfos = pm.queryIntentActivities(mainIntent, PackageManager.GET_UNINSTALLED_PACKAGES);
            for(ResolveInfo refo : resolveInfos){
                if(refo.activityInfo.packageName.equals("com.actions.agingtest")){
                    startAgingTest();
                    break;
                }
            }
        } else if(editor.toString().equals(mLoggerTestMode)) {
            PackageManager pm = getPackageManager();
            Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
            List<ResolveInfo> resolveInfos = pm.queryIntentActivities(mainIntent, PackageManager.GET_UNINSTALLED_PACKAGES);
            for(ResolveInfo refo : resolveInfos){
                if(refo.activityInfo.packageName.equals("com.actions.logcat")){
                    startLogTool();
                    break;
                }
            }
        }else if(editor.toString().equals(mActionsCameraMode)){
            PackageManager pm = getPackageManager();
            Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
            List<ResolveInfo> resolveInfos = pm.queryIntentActivities(mainIntent, PackageManager.GET_UNINSTALLED_PACKAGES);
            for(ResolveInfo refo : resolveInfos){
                if(refo.activityInfo.packageName.equals("com.actions.camera2")){
                    startActionsCamera();
                    break;
                }
            }
        //ActionsCode(phchen, new feature:dual logo), 
        } else if(editor.toString().equals(mOpenDualLogo)){
            if (switchLogoEnabled() == true) {
            	setSwitchLogoValue(1);
            }
        } else if(editor.toString().equals(mCloseDualLogo)){
			if (switchLogoEnabled() == true) {
            	setSwitchLogoValue(0);
            }
        } 
                
    }
    
    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    public boolean switchLogoEnabled(){
        int supportduallogo = SystemProperties.getInt("ro.boot.supportduallogo", 0);
        int openDuallogo = supportduallogo & 0x0f;
        supportduallogo = supportduallogo & 0x0f0;
        if ((openDuallogo > 0) && ((supportduallogo == 32) || (supportduallogo == 48)) 
        && (new File(BOOT_PIC1).exists() || new File(BOOT_PIC2).exists())) {
            return true;
        }
        return false;
    }
    
    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    public static int getSwitchLogoValue(){
        String info = "0";
        int value = 0;

        try {
            info = FileUtils.readTextFile(new File(SWITCH_LOGO_FILE), 0, null).trim();
        } catch (IOException e) {
            Log.e(LOG_TAG, "failed to read " + SWITCH_LOGO_FILE);
        }

        if(info.charAt(0) == '1'){
            value = 1;
        }
        return value;
    }    
    
    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    public static void setSwitchLogoValue(int value){
        String info = "0";
        if(value == 1){
            info = "1";
        }

        try {
            FileUtils.stringToFile(SWITCH_LOGO_FILE, info);
        } catch (IOException e) {
            Log.e(LOG_TAG, "failed to write to" + SWITCH_LOGO_FILE);
        }
    }    

    /**
      *ActionsCode(author:lizekun, type:new_method)
    */
    void startDisplaySettingsTest(){
        Intent intent = new Intent("android.intent.action.MAIN");
        intent.addCategory("android.intent.category.LAUNCHER");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setComponent(new ComponentName("com.actions.displaysettings", "com.actions.displaysettings.FirstActivity"));
        startActivity(intent);
    }

    void startBatteryDebugTest(){
        Intent intent = new Intent("android.intent.action.MAIN");
        intent.addCategory("android.intent.category.LAUNCHER");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setComponent(new ComponentName("com.actions.batterydebugtools", "com.actions.batterydebugtools.BatterDebugToolsActivity"));
        startActivity(intent);
    }
    
    /**
      *ActionsCode(author:lizihao, type:new_method)
    */
    void startProductLineTest(){
        Intent intent = new Intent("android.intent.action.MAIN");
        intent.addCategory("android.intent.category.LAUNCHER");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setComponent(new ComponentName("com.test.ProductLineTest", "com.test.ProductLineTest.ProductLineTestActivity"));
        startActivity(intent);
    }
    
    /**
      *ActionsCode(author:lizihao, type:new_method)
    */
    void startPCBATest(){
        Intent intent = new Intent("android.intent.action.MAIN");
        intent.addCategory("android.intent.category.LAUNCHER");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setComponent(new ComponentName("com.test.pcbatest", "com.test.pcbatest.PcbaTestActivity"));
        startActivity(intent);
    }
    
    /**
      *ActionsCode(author:lizihao, type:new_method)
    */
    void startAgingTest(){
        Intent intent = new Intent("android.intent.action.MAIN");
        intent.addCategory("android.intent.category.LAUNCHER");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setComponent(new ComponentName("com.actions.agingtest", "com.actions.agingtest.AgingTestActivity"));
        startActivity(intent);
    }

        /**
      *ActionsCode(author:Jiang, type:new_method)
    */
    void startLogTool(){
        Intent intent = new Intent("android.intent.action.MAIN");
        intent.addCategory("android.intent.category.LAUNCHER");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setComponent(new ComponentName("com.actions.logcat", "com.actions.logcat.MainActivity"));
        startActivity(intent);
    }

        /**
      *ActionsCode(author:daihongbo, type:new_method)
    */
    void startActionsCamera(){
        Intent intent = new Intent("android.intent.action.MAIN");
        intent.addCategory("android.intent.category.ACTIONSCAMERA");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
    }    
}
