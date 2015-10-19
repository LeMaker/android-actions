
package com.actions.utils;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.util.Log;

import com.actions.download.ContinueFTP;
import com.actions.model.UpdateInfo;
import com.actions.parsexml.UpdateInfoContentHandler;
import com.actions.userconfig.UserConfig;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class VersionUtils {
    private static final String TAG = "ota.VersionUtils";
    private static final boolean ACTDEBUG = true;
    private Context mContext;

    public VersionUtils(Context cxt) {
        this.mContext = cxt;
    }

    public String getXmlByHttp() {
        // down load update xml message from server
        String result = downloadXml(Utilities.getServerXmlUrl(), mContext);
        return result;
    }

    public String getXmlByFTP() {
        ContinueFTP mContinueFTPXml = new ContinueFTP();
        String result = null;
        try {
//            String host;
//            String dir = "";
//            host = UserConfig.ftpHost.toLowerCase();
//            if (host.startsWith("http://")) {
//                host = host.substring("http://".length());
//            } else if (host.startsWith("ftp://")) {
//                host = host.substring("ftp://".length());
//            }
//            if (host.indexOf('/') > 0) {
//                dir = host.substring(host.indexOf('/'));
//                host = host.substring(0, host.indexOf('/'));
//            }   
            
//            Log.d(TAG, "CheckVersion host=" + host);
//            Log.d(TAG, "CheckVersion xmlfile=" + dir + UserConfig.remoteXmlPath);
                     
            mContinueFTPXml.mFtpClient.setConnectTimeout(5 * 1000);
            if (mContinueFTPXml.connect(UserConfig.ftpHost, UserConfig.ftpPort,
                    UserConfig.ftpUserName, UserConfig.ftpPassword)) {
                InputStream inputStream = mContinueFTPXml.mFtpClient
                        .retrieveFileStream(UserConfig.remoteXmlPath);
                result = inputStream2String(inputStream);
//            if (mContinueFTPXml.connect(host, UserConfig.ftpPort,
//                    UserConfig.ftpUserName, UserConfig.ftpPassword)) {
//                InputStream inputStream = mContinueFTPXml.mFtpClient
//                        .retrieveFileStream(dir + UserConfig.remoteXmlPath);
//                result = inputStream2String(inputStream);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return result;
    }

    public UpdateInfo CheckVersion(String xml) {
        UpdateInfo mTargetUpdateInfo = null;
        UpdateInfoContentHandler mUpdateInfoContentHandler = null;
        if (xml != null && !xml.equals("")) {
            // parse the xml message
            mUpdateInfoContentHandler = Utilities.parse(xml);
        }
Log.d(TAG, "CheckVersion mUpdateInfoContentHandler=" + mUpdateInfoContentHandler);
        if (mUpdateInfoContentHandler != null){
            List<UpdateInfo> infos_full = mUpdateInfoContentHandler.getFullInfos();
            List<UpdateInfo> infos_diff = mUpdateInfoContentHandler.getDiffInfos();

            /**
             * input : infos_full infos_diff output: targetUpdateInfo_full targetUpdateInfo_diff
             */
            UpdateInfo targetUpdateInfo_full = getTargetFullUpdateInfo(infos_full);
            UpdateInfo targetUpdateInfo_diff = getTargetDiffUpdateInfo(infos_diff);

            /**
             * input:targetUpdateInfo_full targetUpdateInfo_diff output:mTargetUpdateInfo msg
             */
            // targetUpdateInfo_full pk targetUpdateInfo_diff
            if ((targetUpdateInfo_full == null) && (targetUpdateInfo_diff == null)) {
                // no update
                Log.d(TAG, "CheckVersion targetUpdateInfo_full=" + targetUpdateInfo_full);
                Log.d(TAG, "CheckVersion targetUpdateInfo_diff=" + targetUpdateInfo_diff);
            } else {
                if ((targetUpdateInfo_full != null) && (targetUpdateInfo_diff == null)) {
                    mTargetUpdateInfo = targetUpdateInfo_full;
                } else if ((targetUpdateInfo_full == null) && (targetUpdateInfo_diff != null)) {
                    mTargetUpdateInfo = targetUpdateInfo_diff;
                } else {
                    if (targetUpdateInfo_full.getNewVersionComp() > targetUpdateInfo_diff
                            .getNewVersionComp()) {
                        mTargetUpdateInfo = targetUpdateInfo_full;
                    } else {
                        mTargetUpdateInfo = targetUpdateInfo_diff;
                    }
                }
                Log.d(TAG, "CheckVersion targetUpdateInfo_full=" + targetUpdateInfo_full);
                Log.d(TAG, "CheckVersion targetUpdateInfo_diff=" + targetUpdateInfo_diff);
            }
        }
        SaveUpdateTime();
        if (mTargetUpdateInfo != null && mTargetUpdateInfo.isAvailability()) {
            return mTargetUpdateInfo;
        }
        return null;
    }

    /**
     * get the Full targetUpdateInfo
     * 
     * @param mUpdateInfoList
     * @return UpdateInfo
     */
    private UpdateInfo getTargetFullUpdateInfo(List<UpdateInfo> mUpdateInfoList) {
        UpdateInfo targetUpdateInfo = null;
        long tmpversion = 0l;
        Iterator<UpdateInfo> mList;

        mList = mUpdateInfoList.iterator();
        long mCurVersion = StringtoIntVersion(Utilities.mCurrentSystemVersion);

        while (mList.hasNext()) {
            String mversion = mList.next().getNewVersion();
            long mIntVersion = StringtoIntVersion(mversion);
            if (mIntVersion > tmpversion) {
                tmpversion = mIntVersion;
            }
        }
        mList = mUpdateInfoList.iterator();
        while (mList.hasNext() && (tmpversion != 0l)) {
            UpdateInfo tmp = mList.next();
            long mIntVersion = StringtoIntVersion(tmp.getNewVersion());
            if (mIntVersion == tmpversion) {
                targetUpdateInfo = tmp;
                break;
            }
        }
        // Util classes only provide datas rather than rules by caichsh
        // if(mCurVersion > tmpversion){
        // targetUpdateInfo = null;
        // }
        if (ACTDEBUG) {
            Log.d(TAG, "getTargetFullUpdateInfo : ");

            if (targetUpdateInfo == null) {
                Log.d(TAG, " null ");
            } else {
                printUpdateInfo(targetUpdateInfo);
            }
        }
        return targetUpdateInfo;
    }

    /**
     * get the Diff targetUpdateInfo
     * 
     * @param mUpdateInfoList
     * @return UpdateInfo
     */
    private UpdateInfo getTargetDiffUpdateInfo(List<UpdateInfo> mUpdateInfoList) {
        UpdateInfo targetUpdateInfo = null;
        Iterator<UpdateInfo> mList;
        long tmpversion = 0l;

        mList = mUpdateInfoList.iterator();
        long mCurVersion = StringtoIntVersion(Utilities.mCurrentSystemVersion);
        // ---debug---
        // if(ACTDEBUG) mCurVersion = 102l;
        // --- end debug---


        
        mList = mUpdateInfoList.iterator();
        while (mList.hasNext()) {								
            UpdateInfo tmp = mList.next();
            long mIntVersion = StringtoIntVersion(tmp.getOldVersion());	
            long NewVersion = StringtoIntVersion(tmp.getNewVersion());
            if (mIntVersion == tmpversion || NewVersion == tmpversion) {
                targetUpdateInfo = tmp;
                break;
            }
        }
        
        if (ACTDEBUG) {
            Log.d(TAG, "getTargetDiffUpdateInfo : mCurVersion = " + mCurVersion);

            if (targetUpdateInfo == null) {
                Log.d(TAG, " couldn't parser tag null ");
            } else {
                printUpdateInfo(targetUpdateInfo);
            }
        }
        return targetUpdateInfo;
    }

    /**
     * targetversion the version must end with "Vnumbers" or the automatic generated one. the
     * examples of version: [ro.device.model]Vnumbers: atm7059_hr820acV201501 Demo-eng 4.4.2 KOT49H
     * eng.laurawan.20150103.152646 release-keys xxxv1.0.4
     * 
     * @author laurawan
     * @param version
     * @return ret
     */
    public static long StringtoIntVersion(String version) {
        long ret = 0l;
        if (version == null)
            return ret;
        final String tmp = version;

        String mString = tmp;
        // debug
        // mString = "TAG_140723_BF3__TAG_GS702C_4420_140717";
        // mString = "Demo-eng 4.4.2 KOT49H eng.laurawan.20150103.152646 release-keys";

        mString = mString.trim();// no space @start or @end
        String regexp = ".*[v]{1}[0-9\\.\\s]+";
        Pattern mPattern = Pattern.compile(regexp, Pattern.CASE_INSENSITIVE);
        Matcher mMatcher = mPattern.matcher(mString);
        if (mMatcher.matches()) {
            if (ACTDEBUG)
                Log.d(TAG, mString + " matches  v/Vnumbers or v/V1.0.2");

            // ignore space
            String[] strs = mString.split("\\s");
            mString = "";
            for (int i = 0; i < strs.length; i++) {
                mString += strs[i];
            }
            // ignore '.'
            strs = mString.split("\\.");
            mString = "";
            for (int i = 0; i < strs.length; i++) {
                mString += strs[i];
            }
            mString = mString.toLowerCase();

            // get the 'v's index
            StringBuilder sb = new StringBuilder(mString);
            String str = sb.reverse().toString().toLowerCase();
            int mIndex = str.indexOf('v');
            mIndex = str.length() - mIndex - 1;
            // get the String after mIndex
            mString = mString.substring(mIndex + 1);

        } else if (Pattern.compile(".*[0-9]{8,}\\.[0-9]{6,}\\s.*").matcher(mString).matches()) {
            /**
             * Demo-eng 4.4.2 KOT49H eng.laurawan.20150103.152646 release-keys
             */
            if (ACTDEBUG)
                Log.d(TAG, "Auto-generated one:" + mString);
            // split
            String[] strs = mString.split("\\s");

            mString = strs[3];
            strs = mString.split("\\.");
            mString = "";
            for (int i = 0; i < strs.length; i++) {
                if (Pattern.compile("[0-9]{6,}").matcher(strs[i]).matches()) {
                    mString += strs[i];
                }
            }

        } else if (Pattern.compile("TAG_[a-zA-Z0-9]+_[a-zA-Z0-9]+_[0-9]{6,}").matcher(mString)
                .matches()) {
            /**
             * TAG_GS705A_5000_150126
             */
            if (ACTDEBUG)
                Log.d(TAG, mString + " matches  TAG_GS705A_5000_150126");
            String[] strs = mString.split("_");
            if (Pattern.compile("[0-9]+").matcher(strs[strs.length - 1]).matches()) {// ensure they
                                                                                     // are numbers
                mString = strs[strs.length - 1];
            }
            if (mString.length() == 6) {
               mString = "20" + mString + "000000";
            } else if (mString.length() == 8) {
               mString = mString + "000000";
            }
       }else if(Pattern.compile("TAG_[0-9]{6,}_[A-Z]{2,}[0-9]__TAG+_.*").matcher(mString).matches()){
            /**
             * TAG_140723_BF3__TAG_GS702C_4420_140717
             */
            if (ACTDEBUG)
                Log.d(TAG, mString + " matches  TAG_140723_BF3__TAG_GS702C_4420_140717");
            String[] strs = mString.split("_");

            if (Pattern.compile("[0-9]+").matcher(strs[1]).matches()) {// ensure they are numbers
                mString = strs[1];
            }
			
           if (mString.length() == 6) {
               mString = "20" + mString + "000000";
           }else if (mString.length() == 8) {
               mString = mString + "000000";
           }
        } else if (Pattern.compile(".*[0-9]{8,}.*").matcher(mString).matches()) {
            /**
             * Hr820ac-userdebug 5.0.2 LRX22G liuwenqi01061549 release-keys
             */
            if (ACTDEBUG)
                Log.d(TAG, mString
                        + " matches  Hr820ac-userdebug 5.0.2 LRX22G xxx01061549 release-keys");

            String[] strs = mString.split("[a-zA-Z\\s\\.]");
            mString = "";
            for (int i = 0; i < strs.length; i++) {
                if (Pattern.compile("[0-9]{8,}").matcher(strs[i]).matches()) {
                    mString += strs[i];
                }
            }
        } else {
            Log.w(TAG, mString + " matches nothing");
            mString = "0";
        }

        if (mString.length() > 18) {
            Log.w(TAG, " the version is too long :" + mString);
            mString = "0";
        }
        // String to Long
        ret = Long.parseLong(mString);
        if (ACTDEBUG)
            Log.d(TAG, "the ret is " + ret);

        return ret;
    }

    void SaveUpdateTime() {
        SharedPreferences preferences = mContext.getApplicationContext().getSharedPreferences(
                "time", mContext.MODE_PRIVATE);
        Editor editor = preferences.edit();
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy.MM.dd " + "HH:mm:ss");
        editor.putString("time", sdf.format(new Date()));
        editor.commit();
    }

    public void printUpdateInfo(UpdateInfo mInfo) {
        if (mInfo == null)
            return;
        Log.d(TAG, " UpdateType :" + mInfo.getUpdateType());
        Log.d(TAG, " OldVersion :" + mInfo.getOldVersion());
        Log.d(TAG, " getNewVersion :" + mInfo.getNewVersion());
        Log.d(TAG, " DownloadUrl :" + mInfo.getDownloadUrl());
        Log.d(TAG, " FileSize :" + mInfo.getFileSize());
        Log.d(TAG, " Md5 :" + mInfo.getMd5());
    }

    public String downloadXml(String urlStr, Context mContext) {
        StringBuffer sb = new StringBuffer();
        String line = null;
        InputStream is = null;
        InputStreamReader isr = null;
        BufferedReader buffer = null;
        HttpURLConnection urlConn = null;

        try {
            // new a URL class
            URL url = new URL(urlStr);
            // create http connect
            urlConn = (HttpURLConnection) url
                    .openConnection();

            // set http post method
            urlConn.setRequestMethod("GET");
            // read http resource
            urlConn.setDoInput(true);
            // set connect timeout
            urlConn.setConnectTimeout(6 * 10 * 1000);
            // set connect read timeout
            urlConn.setReadTimeout(6000);

            is = urlConn.getInputStream();
            isr = new InputStreamReader(is, "utf-8");
            // use IO to read data
            buffer = new BufferedReader(isr);
            while ((line = buffer.readLine()) != null) {
                sb.append(line);
            }
        } catch (Exception e) {
            e.printStackTrace();
            // set the download status is downloaded
            // Utilities.setDownloadStatus(mContext, Utilities.mFinishDownload);
            System.out.println("OTA Abnormal network connection");
            return "";
        } finally {
            try {
                if (buffer != null) {
                    buffer.close();
                }
                if (is != null) {
                    is.close();
                }
                if (isr != null) {
                    isr.close();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }

            if (urlConn != null) {
                urlConn.disconnect();
            }
        }
        return sb.toString();

    }

    /*
     * return -1: download file error return 0: download file success return 1: download file had
     * exist
     */
    /*
     * public int downFile(String urlStr, String fileName){ InputStream inputStream = null;
     * FileUtils fileUtils = new FileUtils(); if(fileUtils.isFileExist(fileName)){
     * System.out.println("--->Update File is Exist"); return 1; } else { try { inputStream =
     * getInputStreamFromUrl(urlStr); File resultFile = fileUtils.write2SDfromInputStream(fileName,
     * inputStream); if(resultFile == null){ return -1; }
     * System.out.println("--->File download ok"); } catch (MalformedURLException e) { // TODO
     * Auto-generated catch block e.printStackTrace(); } catch (IOException e) { // TODO
     * Auto-generated catch block e.printStackTrace(); } finally{ try { inputStream.close(); } catch
     * (IOException e) { // TODO Auto-generated catch block e.printStackTrace(); } } } return 0; }
     */
    private static String inputStream2String(InputStream is) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        int i = -1;
        Log.d(TAG, "inputStream2String is=" + is);
        if (is == null){
            return null;
        }
        while ((i = is.read()) != -1) {
            baos.write(i);
        }
        return baos.toString();
    }

    /*
     * get inputstream from url
     */
    public InputStream getInputStreamFromUrl(String urlStr) throws MalformedURLException,
            IOException {
        URL url = new URL(urlStr);
        HttpURLConnection urlConn = (HttpURLConnection) url.openConnection();
        InputStream inputStream = urlConn.getInputStream();
        return inputStream;
    }
}
