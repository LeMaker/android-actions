package com.actions.ota;

import com.actions.utils.Utilities;

import android.content.Context;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceManager;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;

public class SettingActivity extends PreferenceActivity implements OnPreferenceChangeListener{

	private CheckBoxPreference mCheckBoxPreference = null;
	private ListPreference mListPreference = null;
	private Context mContext = this;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		setTheme(R.style.AppTheme) ;
		super.onCreate(savedInstanceState);
		setTitle(getResources().getText(R.string.apk_title));
		addPreferencesFromResource(R.xml.list);
		
		findView();
	}
	
	public void findView(){
		mCheckBoxPreference = (CheckBoxPreference) findPreference("setcheckbox");
		mCheckBoxPreference.setOnPreferenceChangeListener(this);
		
		mListPreference = (ListPreference) findPreference("setfrequency");
		mListPreference.setOnPreferenceChangeListener(this);
		
		String summary = PreferenceManager.getDefaultSharedPreferences(this).getString(mListPreference.getKey()
				, Utilities.readCheckFrequency(mContext) + "");
		mListPreference.setSummary(getResources().getString(R.string.checkfrequency_summary)
				+ summary + getResources().getString(R.string.day));
		
		if(Utilities.readAutoCheck(mContext)){
			mCheckBoxPreference.setChecked(Utilities.readAutoCheck(mContext));
		} else{
			mListPreference.setEnabled(false);
		}
		
	}

	@Override
	public boolean onPreferenceChange(Preference preference, Object newValue) {
		if(preference == mCheckBoxPreference){
			Utilities.writeAutoCheck(mContext, (Boolean)newValue);
			if((Boolean)newValue){
				mListPreference.setEnabled(true);
			} else{
				mListPreference.setEnabled(false);
			}
		} else if(preference == mListPreference){
			mListPreference.setSummary(getResources().getString(R.string.checkfrequency_summary)
					+ (String) newValue + getResources().getString(R.string.day));
			Utilities.writeCheckFrequency(mContext, Integer.valueOf((String)newValue).intValue());
		}
		return true;
	}

}
