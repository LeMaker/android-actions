package com.actions.explore;

/*@author lizihao@actions-semi.com*/

import android.app.ActionBar;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.Preference.OnPreferenceChangeListener;

public class SettingPreference extends PreferenceActivity implements OnPreferenceChangeListener {

	private ListPreference mTextColorListPreference = null;
	private CheckBoxPreference mHideFileCheckBoxPreference = null;
	private CheckBoxPreference mImagePreviewCheckBoxPreference = null;
	//private CheckBoxPreference mSpaceCheckBoxPreference = null;
	private ListPreference mSortTypeListPreference = null;
	
	private boolean mHiddenChanged = false;
	private boolean mColorChanged = false;
	private boolean mThumbnailChanged = false;
	private boolean mSortChanged = false;
	//private boolean mSpaceChanged = false;
	
	private boolean hidden_state;
	private boolean thumbnail_state;
	private int color_state, sort_state;/*, mSpaceState;*/
	private Intent is = new Intent();
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.preferences);
		findView();
		initExtras();
	}
	
	public void findView() {
		mTextColorListPreference = (ListPreference) findPreference("settextcolor");
		mTextColorListPreference.setOnPreferenceChangeListener(this);
		initTextColorListPreference(mTextColorListPreference);
		
		mHideFileCheckBoxPreference = (CheckBoxPreference) findPreference("sethidefile");
		mHideFileCheckBoxPreference.setOnPreferenceChangeListener(this);
		
		mImagePreviewCheckBoxPreference = (CheckBoxPreference) findPreference("setimagepreview");
		mImagePreviewCheckBoxPreference.setOnPreferenceChangeListener(this);
		
		//mSpaceCheckBoxPreference = (CheckBoxPreference) findPreference("setspace");
		//mSpaceCheckBoxPreference.setOnPreferenceChangeListener(this);
		
		mSortTypeListPreference = (ListPreference) findPreference("setsorttype");
		mSortTypeListPreference.setOnPreferenceChangeListener(this);
		initSortTypeListPreference(mSortTypeListPreference);
	}
	
	public void initTextColorListPreference(ListPreference mTextColorListPreference){
		CharSequence[] colorentries = {getResources().getString(R.string.Black),
				getResources().getString(R.string.Magenta),
				getResources().getString(R.string.Yellow),
				getResources().getString(R.string.Red),
				getResources().getString(R.string.Cyan),
				getResources().getString(R.string.Blue),
				getResources().getString(R.string.Green)};
		CharSequence[] colorentryValues = {getResources().getString(R.string.Black),
				getResources().getString(R.string.Magenta),
				getResources().getString(R.string.Yellow),
				getResources().getString(R.string.Red),
				getResources().getString(R.string.Cyan),
				getResources().getString(R.string.Blue),
				getResources().getString(R.string.Green)};
		mTextColorListPreference.setEntries(colorentries);
		mTextColorListPreference.setEntryValues(colorentryValues);
		mTextColorListPreference.setDialogIcon(R.drawable.color);
	}
	
	public void initSortTypeListPreference(ListPreference mSortTypeListPreference) {
		CharSequence[] sortentries = {getResources().getString(R.string.None), 
				getResources().getString(R.string.Alphabetical), 
				getResources().getString(R.string.Type)};
		CharSequence[] sortentryValues = {getResources().getString(R.string.None), 
				getResources().getString(R.string.Alphabetical), 
				getResources().getString(R.string.Type)};
		mSortTypeListPreference.setEntries(sortentries);
		mSortTypeListPreference.setEntryValues(sortentryValues);
		mSortTypeListPreference.setDialogIcon(R.drawable.filter);
	}
	
	public void initExtras() {
		Intent i = getIntent();
		hidden_state = i.getExtras().getBoolean("HIDDEN");
		thumbnail_state = i.getExtras().getBoolean("THUMBNAIL");
		color_state = i.getExtras().getInt("COLOR");
		sort_state = i.getExtras().getInt("SORT");
		//mSpaceState = i.getExtras().getInt("SPACE");
		
		if(color_state == Color.BLACK){
			mTextColorListPreference.setValue(getString(R.string.Black));
		} else if(color_state == Color.MAGENTA){
			mTextColorListPreference.setValue(getString(R.string.Magenta));
		} else if(color_state == Color.YELLOW){
			mTextColorListPreference.setValue(getString(R.string.Yellow));
		} else if(color_state == Color.RED){
			mTextColorListPreference.setValue(getString(R.string.Red));
		} else if(color_state == Color.CYAN){
			mTextColorListPreference.setValue(getString(R.string.Cyan));
		} else if(color_state == Color.BLUE){
			mTextColorListPreference.setValue(getString(R.string.Blue));
		} else if(color_state == Color.GREEN){
			mTextColorListPreference.setValue(getString(R.string.Green));
		} else {
			color_state = Color.BLACK;
			mTextColorListPreference.setValue(getString(R.string.Black));
		}
		mHideFileCheckBoxPreference.setChecked(hidden_state);
		mImagePreviewCheckBoxPreference.setChecked(thumbnail_state);
		//mSpaceCheckBoxPreference.setChecked(mSpaceState == View.VISIBLE);
		if(sort_state == 0){
			mSortTypeListPreference.setValue(getString(R.string.None));
		} else if(sort_state == 1){
			mSortTypeListPreference.setValue(getString(R.string.Alphabetical));
		} else if(sort_state == 2) {
			mSortTypeListPreference.setValue(getString(R.string.Type));
		} else {
			sort_state = 1;
			mSortTypeListPreference.setValue(getString(R.string.Alphabetical));
		}
		//mSortTypeListPreference.setDefaultValue(sort_state);
		
		// set result
		/*if(!mSpaceChanged)
			is.putExtra("SPACE", mSpaceState);*/
		if(!mHiddenChanged)
			is.putExtra("HIDDEN", hidden_state);
		if(!mColorChanged)
			is.putExtra("COLOR", color_state);
		if(!mThumbnailChanged)
			is.putExtra("THUMBNAIL", thumbnail_state);
		if(!mSortChanged)
			is.putExtra("SORT", sort_state);
		setResult(RESULT_CANCELED, is);
	}

	@Override
	public boolean onPreferenceChange(Preference preference, Object newValue) {
		// TODO Auto-generated method stub
		if(preference == mTextColorListPreference) {
			changeTextColor((String)newValue);
		} else if(preference == mHideFileCheckBoxPreference) {
			hidden_state = (Boolean)newValue;
			is.putExtra("HIDDEN", hidden_state);
			mHiddenChanged = true;
		} else if(preference == mImagePreviewCheckBoxPreference) {
			thumbnail_state = (Boolean)newValue;
			is.putExtra("THUMBNAIL", thumbnail_state);
			mThumbnailChanged = true;
		} /*else if(preference == mSpaceCheckBoxPreference) {
			if((Boolean)newValue) {
				mSpaceState = View.VISIBLE;
			} else {
				mSpaceState = View.GONE;
			}		
			mSpaceChanged = true;
			is.putExtra("SPACE", mSpaceState);		
		}*/ else if(preference == mSortTypeListPreference) {
			changeSortType((String)newValue);
		}
		return true;
	}
	
	public void changeTextColor(String newValue) {
		if(newValue.equals(getString(R.string.Black))){
			color_state = Color.BLACK;
			is.putExtra("COLOR", color_state);
			mColorChanged = true;
		} else if(newValue.equals(getString(R.string.Magenta))){
			color_state = Color.MAGENTA;
			is.putExtra("COLOR", color_state);
			mColorChanged = true;
		} else if(newValue.equals(getString(R.string.Yellow))){
			color_state = Color.YELLOW;
			is.putExtra("COLOR", color_state);
			mColorChanged = true;
		} else if(newValue.equals(getString(R.string.Red))){
			color_state = Color.RED;
			is.putExtra("COLOR", color_state);
			mColorChanged = true;
		} else if(newValue.equals(getString(R.string.Cyan))){
			color_state = Color.CYAN;
			is.putExtra("COLOR", color_state);
			mColorChanged = true;
		} else if(newValue.equals(getString(R.string.Blue))){
			color_state = Color.BLUE;
			is.putExtra("COLOR", color_state);
			mColorChanged = true;
		} else if(newValue.equals(getString(R.string.Green))){
			color_state = Color.GREEN;
			is.putExtra("COLOR", color_state);
			mColorChanged = true;
		}
	}
	
	public void changeSortType(String newValue) {
		if(newValue.equals(getString(R.string.None))){
			sort_state = 0;
			mSortChanged = true;
			is.putExtra("SORT", sort_state);
		} else if(newValue.equals(getString(R.string.Alphabetical))){
			sort_state = 1;
			mSortChanged = true;
			is.putExtra("SORT", sort_state);
		} else if(newValue.equals(getString(R.string.Type))){
			sort_state = 2;
			mSortChanged = true;
			is.putExtra("SORT", sort_state);
		}
	}

}
