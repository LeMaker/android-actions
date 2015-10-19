/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.settings;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.backup.IBackupManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.provider.SearchIndexableResource;
import android.provider.Settings;
import android.util.Log;

import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settings.search.Indexable;
import com.android.settings.search.Indexable.SearchIndexProvider;

import java.util.ArrayList;
import java.util.List;

import android.os.Handler;
import android.os.Message;
import android.app.ProgressDialog;

import java.io.File;
import java.util.concurrent.atomic.AtomicBoolean;

import android.util.Log;
import android.util.DisplayMetrics;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageParser;
import android.content.pm.IPackageDeleteObserver;
import android.provider.Settings.SettingNotFoundException;
/**
 * Gesture lock pattern settings.
 */
public class PrivacySettings extends SettingsPreferenceFragment implements
        DialogInterface.OnClickListener, Indexable {

	private static final String TAG = "PrivacySettings";
	
    // Vendor specific
    private static final String GSETTINGS_PROVIDER = "com.google.settings";
    private static final String BACKUP_CATEGORY = "backup_category";
    private static final String BACKUP_DATA = "backup_data";
    private static final String AUTO_RESTORE = "auto_restore";
    private static final String CONFIGURE_ACCOUNT = "configure_account";
    private static final String BACKUP_INACTIVE = "backup_inactive";
    private static final String PERSONAL_DATA_CATEGORY = "personal_data_category";
    private static final String DELETE_RUN_ONCE_APPS = "delete_run_once_apps";
    private static final String RUN_ONCE_PREFIX_NAME = "act1tinstall_";
    private static final String RUN_ONCE_DIR = "/data/app/";
    private static final String RUN_ONCE_HAVE_DELETE = "run_once_have_deleted";
    private static final int MSG_CREATE_PROGRESS = 0;
    private static final int MSG_CLOSE_PROGRESS = 1;
    private ProgressDialog mProgressDl;

    private Preference mDeleteRunOnce;
    PackageManager mPackageMan;
    private IBackupManager mBackupManager;
    private SwitchPreference mBackup;
    private SwitchPreference mAutoRestore;
    private Dialog mConfirmDialog;
    private PreferenceScreen mConfigure;
    private boolean mEnabled;

    private static final int DIALOG_ERASE_BACKUP = 2;
    private int mDialogType;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Don't allow any access if this is a secondary user
        mEnabled = Process.myUserHandle().isOwner();
        if (!mEnabled) {
            return;
        }

        addPreferencesFromResource(R.xml.privacy_settings);
        final PreferenceScreen screen = getPreferenceScreen();
        mBackupManager = IBackupManager.Stub.asInterface(
                ServiceManager.getService(Context.BACKUP_SERVICE));

        //ActionsCode(phchen, new feature),
        mPackageMan = getPackageManager();

        mBackup = (SwitchPreference) screen.findPreference(BACKUP_DATA);
        mBackup.setOnPreferenceChangeListener(preferenceChangeListener);

        mAutoRestore = (SwitchPreference) screen.findPreference(AUTO_RESTORE);
        mAutoRestore.setOnPreferenceChangeListener(preferenceChangeListener);

        mConfigure = (PreferenceScreen) screen.findPreference(CONFIGURE_ACCOUNT);

        ArrayList<String> keysToRemove = getNonVisibleKeys(getActivity());
        final int screenPreferenceCount = screen.getPreferenceCount();
        for (int i = screenPreferenceCount - 1; i >= 0; --i) {
            Preference preference = screen.getPreference(i);
            if (keysToRemove.contains(preference.getKey())) {
                screen.removePreference(preference);
            }
        }

        //ActionsCode(phchen, new feature),
        mDeleteRunOnce = findPreference(DELETE_RUN_ONCE_APPS);
        
        if(checkHaveDeleteStatus(getActivity().getContentResolver())) {
        	screen.removePreference(mDeleteRunOnce);
        }

        PreferenceCategory backupCategory = (PreferenceCategory) findPreference(BACKUP_CATEGORY);
        if (backupCategory != null) {
            final int backupCategoryPreferenceCount = backupCategory.getPreferenceCount();
            for (int i = backupCategoryPreferenceCount - 1; i >= 0; --i) {
                Preference preference = backupCategory.getPreference(i);
                if (keysToRemove.contains(preference.getKey())) {
                    backupCategory.removePreference(preference);
                }
            }
        }
        updateToggles();
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
    private boolean checkHaveDeleteStatus(ContentResolver cr) {
    	if(cr == null) {
    		return false;
    	}
    	try {
        	int haveDeleted = Settings.System.getInt(cr, RUN_ONCE_HAVE_DELETE);
        	return haveDeleted == 1;
        } catch (SettingNotFoundException e) { 	
        }
    	
    	File fDir = new File(RUN_ONCE_DIR);
    	String[] files = null;
    	if(fDir != null) {
    		files = fDir.list();
    	}
    	
        if (files != null) {
            for(int i = 0; i < files.length; i++) {
                if(isRunOnceRelationalFile(files[i]) == 0) {
                    Settings.System.putInt(cr, RUN_ONCE_HAVE_DELETE, 0);
                    return false;
                }
            }
        }
        return true;        
    }
    @Override
    public void onResume() {
        super.onResume();

        // Refresh UI
        if (mEnabled) {
            updateToggles();
        }
    }

    @Override
    public void onStop() {
        if (mConfirmDialog != null && mConfirmDialog.isShowing()) {
            mConfirmDialog.dismiss();
        }
        mConfirmDialog = null;
        mDialogType = 0;
        super.onStop();
    }

    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(author:phchen, change_code)
    */
    private OnPreferenceChangeListener preferenceChangeListener = new OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            if (!(preference instanceof SwitchPreference)) {
                return true;
            }
            boolean nextValue = (Boolean) newValue;
            boolean result = false;
            if (preference == mBackup) {
                if (nextValue == false) {
                    // Don't change Switch status until user makes choice in dialog
                    // so return false here.
                    showEraseBackupDialog();
                } else {
                    setBackupEnabled(true);
                    result = true;
                }
            } else if (preference == mAutoRestore) {
                try {
                    mBackupManager.setAutoRestore(nextValue);
                    result = true;
                } catch (RemoteException e) {
                    mAutoRestore.setChecked(!nextValue);
                }
            }
            return result;
        }
    };
    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        //ActionsCode(phchen, new feature : delete run once apps), 
        if (preference == mDeleteRunOnce) {
            new Thread(new Runnable(){
                public void run() {
                    mDeleteHandler.sendEmptyMessage(MSG_CREATE_PROGRESS);
                    deleteRunOnceApps(RUN_ONCE_DIR);
                    Settings.System.putInt(getActivity().getContentResolver(), RUN_ONCE_HAVE_DELETE, 1);
                    mDeleteHandler.sendEmptyMessage(MSG_CLOSE_PROGRESS);
                }
            }).start();	
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
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
    private Handler mDeleteHandler = new Handler(){
        public void handleMessage(Message msg) {
            switch(msg.what){
                case MSG_CREATE_PROGRESS:	
                    mProgressDl = ProgressDialog.show(getActivity(), "wait", "delete the files...");   
                    break;
                case MSG_CLOSE_PROGRESS:
                    if(mProgressDl != null) {
                        mProgressDl.dismiss();
                        mProgressDl = null;
                    }
                    break;
            }
        }
    };  
    
    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    private int isRunOnceRelationalFile(String filename) {
    	if(filename.indexOf(RUN_ONCE_PREFIX_NAME) == 0 && filename.endsWith(".apk")) {
    		return 0;
    	} else if(filename.indexOf(RUN_ONCE_PREFIX_NAME) == 0 && filename.endsWith(".odex")) {
    		return 1;
    	}
    	return -1;
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
    private String getPackageNameFormFile(String fileName) {
    	PackageParser packageParser = new PackageParser();
        DisplayMetrics metrics = new DisplayMetrics();
        metrics.setToDefaults();
        final File sourceFile = new File(fileName);
        PackageParser.Package pkg = null;
        try {
        	pkg = packageParser.parsePackage(sourceFile, PackageParser.PARSE_IGNORE_PROCESSES);
        } catch (Exception e) {
        	//throw Exception
        }
         if (pkg == null) {
             return null;
         }
		return pkg.packageName;
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
    private void deleteRunOnceApps(String dir) {
        File fDir = new File(dir);
        String[] files = null;
        if(fDir != null) {
            files = fDir.list();
        }

        if (files == null) {
            return;
        }

        PackageDeleteObserver observer = new PackageDeleteObserver();
        for (int i=0; i<files.length; i++) {
            File file = new File(fDir, files[i]);    
            int relation = isRunOnceRelationalFile(files[i]);
            if (relation == 0) {
                String packageName = getPackageNameFormFile(file.getPath());
                try {
                    ApplicationInfo mAppInfo = mPackageMan.getApplicationInfo(packageName, PackageManager.GET_UNINSTALLED_PACKAGES);
                    observer.reset();
                    mPackageMan.deletePackage(mAppInfo.packageName, observer, 0);
                    observer.waitForCompletion();
                } catch(PackageManager.NameNotFoundException e) {

                }    
            } 

            if(relation >= 0) {
                file.delete();
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
    class PackageDeleteObserver extends IPackageDeleteObserver.Stub {
        final AtomicBoolean mDone = new AtomicBoolean();
        int mResult;

        public void reset() {
            synchronized (mDone) {
                mDone.set(false);
            }
        }

        public void waitForCompletion() {
            synchronized (mDone) {
                while (mDone.get() == false) {
                    try {
                        mDone.wait();
                    } catch (InterruptedException e) { }
                }
            }
        }

        @Override
        public void packageDeleted(String packageName, int returnCode) throws RemoteException {
            synchronized (mDone) {
                mResult = returnCode;
                mDone.set(true);
                mDone.notifyAll();
            }
        }
    }
    
    private void showEraseBackupDialog() {
        mDialogType = DIALOG_ERASE_BACKUP;
        CharSequence msg = getResources().getText(R.string.backup_erase_dialog_message);
        // TODO: DialogFragment?
        mConfirmDialog = new AlertDialog.Builder(getActivity()).setMessage(msg)
                .setTitle(R.string.backup_erase_dialog_title)
                .setPositiveButton(android.R.string.ok, this)
                .setNegativeButton(android.R.string.cancel, this)
                .show();
    }

    /*
     * Creates toggles for each available location provider
     */
    private void updateToggles() {
        ContentResolver res = getContentResolver();

        boolean backupEnabled = false;
        Intent configIntent = null;
        String configSummary = null;
        try {
            backupEnabled = mBackupManager.isBackupEnabled();
            String transport = mBackupManager.getCurrentTransport();
            configIntent = mBackupManager.getConfigurationIntent(transport);
            configSummary = mBackupManager.getDestinationString(transport);
        } catch (RemoteException e) {
            // leave it 'false' and disable the UI; there's no backup manager
            mBackup.setEnabled(false);
        }
        mBackup.setChecked(backupEnabled);

        mAutoRestore.setChecked(Settings.Secure.getInt(res,
                Settings.Secure.BACKUP_AUTO_RESTORE, 1) == 1);
        mAutoRestore.setEnabled(backupEnabled);

        final boolean configureEnabled = (configIntent != null) && backupEnabled;
        mConfigure.setEnabled(configureEnabled);
        mConfigure.setIntent(configIntent);
        setConfigureSummary(configSummary);
    }

    private void setConfigureSummary(String summary) {
        if (summary != null) {
            mConfigure.setSummary(summary);
        } else {
            mConfigure.setSummary(R.string.backup_configure_account_default_summary);
        }
    }

    private void updateConfigureSummary() {
        try {
            String transport = mBackupManager.getCurrentTransport();
            String summary = mBackupManager.getDestinationString(transport);
            setConfigureSummary(summary);
        } catch (RemoteException e) {
            // Not much we can do here
        }
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        // Dialog is triggered before Switch status change, that means marking the Switch to
        // true in showEraseBackupDialog() method will be override by following status change.
        // So we do manual switching here due to users' response.
        if (mDialogType == DIALOG_ERASE_BACKUP) {
            // Accept turning off backup
            if (which == DialogInterface.BUTTON_POSITIVE) {
                setBackupEnabled(false);
            } else if (which == DialogInterface.BUTTON_NEGATIVE) {
                // Reject turning off backup
                setBackupEnabled(true);
            }
            updateConfigureSummary();
        }
        mDialogType = 0;
    }

    /**
     * Informs the BackupManager of a change in backup state - if backup is disabled,
     * the data on the server will be erased.
     * @param enable whether to enable backup
     */
    private void setBackupEnabled(boolean enable) {
        if (mBackupManager != null) {
            try {
                mBackupManager.setBackupEnabled(enable);
            } catch (RemoteException e) {
                mBackup.setChecked(!enable);
                mAutoRestore.setEnabled(!enable);
                return;
            }
        }
        mBackup.setChecked(enable);
        mAutoRestore.setEnabled(enable);
        mConfigure.setEnabled(enable);
    }

    @Override
    protected int getHelpResource() {
        return R.string.help_url_backup_reset;
    }

    /**
     * For Search.
     */
    public static final SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new PrivacySearchIndexProvider();

    private static class PrivacySearchIndexProvider extends BaseSearchIndexProvider {

        boolean mIsPrimary;

        public PrivacySearchIndexProvider() {
            super();

            mIsPrimary = UserHandle.myUserId() == UserHandle.USER_OWNER;
        }

        @Override
        public List<SearchIndexableResource> getXmlResourcesToIndex(
                Context context, boolean enabled) {

            List<SearchIndexableResource> result = new ArrayList<SearchIndexableResource>();

            // For non-primary user, no backup or reset is available
            if (!mIsPrimary) {
                return result;
            }

            SearchIndexableResource sir = new SearchIndexableResource(context);
            sir.xmlResId = R.xml.privacy_settings;
            result.add(sir);

            return result;
        }

        @Override
        public List<String> getNonIndexableKeys(Context context) {
            return getNonVisibleKeys(context);
        }
    }

    private static ArrayList<String> getNonVisibleKeys(Context context) {
        final ArrayList<String> nonVisibleKeys = new ArrayList<String>();
        final IBackupManager backupManager = IBackupManager.Stub.asInterface(
                ServiceManager.getService(Context.BACKUP_SERVICE));
        boolean isServiceActive = false;
        try {
            isServiceActive = backupManager.isBackupServiceActive(UserHandle.myUserId());
        } catch (RemoteException e) {
            Log.w(TAG, "Failed querying backup manager service activity status. " +
                    "Assuming it is inactive.");
        }
        if (isServiceActive) {
            nonVisibleKeys.add(BACKUP_INACTIVE);
        } else {
            nonVisibleKeys.add(AUTO_RESTORE);
            nonVisibleKeys.add(CONFIGURE_ACCOUNT);
            nonVisibleKeys.add(BACKUP_DATA);
        }
        if (UserManager.get(context).hasUserRestriction(
                UserManager.DISALLOW_FACTORY_RESET)) {
            nonVisibleKeys.add(PERSONAL_DATA_CATEGORY);
        }
        // Vendor specific
        if (context.getPackageManager().
                resolveContentProvider(GSETTINGS_PROVIDER, 0) == null) {
            nonVisibleKeys.add(BACKUP_CATEGORY);
        }
        return nonVisibleKeys;
    }
}