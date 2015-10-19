package com.actions.parsexml;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import android.util.Log;

import com.actions.model.UpdateInfo;
import com.actions.utils.Utilities;


/*
 * parse the server update info xml class
 * 
 * out: infos_full and infos_diff
 * */
public class UpdateInfoContentHandler extends DefaultHandler {

	private static final String TAG = "ota";
	private static final boolean ACTDEBUG = true;
	private static final boolean DUMP = true;
	
	private String tagName = null;
	private UpdateInfo mUpdateInfo = null;
	List<String> mUpdateMessage = null;
	private boolean flag_full = false;
	
	private List<UpdateInfo> infos_full = null;
    private List<UpdateInfo> infos_diff = null;
	
	@Override
	public void startDocument() throws SAXException {
		if(ACTDEBUG) Log.d(TAG,"startDocument");
		infos_full = new ArrayList<UpdateInfo>();
		infos_diff = new ArrayList<UpdateInfo>();
		mUpdateInfo = new UpdateInfo();
		mUpdateMessage = new ArrayList<String>();
		super.startDocument();		
	}

	@Override
	public void endDocument() throws SAXException {
		if(ACTDEBUG) Log.d(TAG,"endDocument");
		
		mUpdateInfo = null;    
		tagName = null;	
		mUpdateMessage = null;
		
		if(DUMP)dump();
		
		super.endDocument();
	}

	@Override
	public void startElement(String uri, String localName, String qName,
			Attributes attributes) throws SAXException {
		if(ACTDEBUG) Log.d(TAG,"startElement "+uri + " "+ localName + " " +qName);
		this.tagName = localName;
    }

	@Override
	public void endElement(String uri, String localName, String qName)
			throws SAXException {
		if(ACTDEBUG) Log.d(TAG,"endElement "+uri + " "+ localName + " " +qName);
		
		if(localName.equals(Utilities.UpdateInfo)){
			mUpdateInfo.setUpdateMessage(mUpdateMessage);
			mUpdateMessage = null;
        	mUpdateMessage = new ArrayList<String>(); 
			
			if(mUpdateInfo.getUpdateType().equalsIgnoreCase("All-download-OTA")){
				if(ACTDEBUG) Log.d(TAG,"infos_full.add(mUpdateInfo)");
				
				infos_full.add(mUpdateInfo);
			}else if(mUpdateInfo.getUpdateType().equalsIgnoreCase("Recent-version-OTA")){
				if(ACTDEBUG) Log.d(TAG,"infos_diff.add(mUpdateInfo)");
				infos_diff.add(mUpdateInfo);
			}
			mUpdateInfo = null;
        	mUpdateInfo = new UpdateInfo();			
    	}
		this.tagName = "";
	}

	@Override
	public void characters(char[] ch, int start, int length)
			throws SAXException {
		if(ACTDEBUG) Log.d(TAG,"characters "+ch.toString() + " "+ start + " " +length);
		String temp = new String(ch, start, length);
		temp = temp.trim();		
		if(ACTDEBUG) Log.d(TAG,"temp "+temp);
		
		if(this.tagName.toLowerCase().equals("oldversion")){
		    mUpdateInfo.setOldVersion(temp);
		} else if(this.tagName.toLowerCase().equals("newversion")){
		    mUpdateInfo.setNewVersion(temp);
		} else if(this.tagName.toLowerCase().equals("downloadurl")){
		    mUpdateInfo.setDownloadUrl(temp);
		} else if(this.tagName.toLowerCase().equals("filesize")){
		    mUpdateInfo.setFileSize(temp);
		} else if(this.tagName.toLowerCase().equals("md5")){
		    mUpdateInfo.setMd5(temp);
		} else if(this.tagName.toLowerCase().equals("msg")){
		    mUpdateMessage.add(temp);
		}
		
		if(temp.equalsIgnoreCase("All-download-OTA")){
			if(ACTDEBUG) Log.d(TAG,"flag_full = true ");
			flag_full = true;
		}
		if(temp.equalsIgnoreCase("Recent-version-OTA")){
			if(ACTDEBUG) Log.d(TAG,"flag_full = false ");
			flag_full = false;
		}
		
		if(flag_full){
			mUpdateInfo.setUpdateType("All-download-OTA");
		}else{
			mUpdateInfo.setUpdateType("Recent-version-OTA");
		}
	}
	
    public List<UpdateInfo> getFullInfos() {
		return infos_full;
	}
    public List<UpdateInfo> getDiffInfos() {
		return infos_diff;
	}
    private void printUpdateInfo(UpdateInfo mInfo){
    	Log.d(TAG," UpdateType :"+mInfo.getUpdateType());
    	Log.d(TAG," OldVersion :"+mInfo.getOldVersion());
		Log.d(TAG," getNewVersion :"+mInfo.getNewVersion());
		Log.d(TAG," DownloadUrl :"+mInfo.getDownloadUrl());
		Log.d(TAG," FileSize :"+mInfo.getFileSize());
		Log.d(TAG," Md5 :"+mInfo.getMd5());
    }
    private void dump(){
    	Iterator<UpdateInfo> it = infos_full.iterator();
		Log.d(TAG," FULL ");
		while(it.hasNext()){				
			printUpdateInfo(it.next());
		}
		it = infos_diff.iterator();
		Log.d(TAG," DIFF ");
		while(it.hasNext()){				
			printUpdateInfo(it.next());
		}
    }
}
