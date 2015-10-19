package com.android.launcher2;

import java.util.ArrayList;
import java.util.List;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;


/**
  *
  *ParseHidePackage
  *to parse the hide activity in xml
  *
  ************************************
  *      
  *ActionsCode(author:lizihao, type:new_method)
  */
public class ParseHidePackage extends DefaultHandler {

	private String mPackageName ;
	private StringBuffer mString = new StringBuffer() ;
	private List<String> mPackageList ;
	
	public List<String> getPackageList(){
		return mPackageList;
	}
	
	@Override
	public void characters(char[] ch, int start, int length)
			throws SAXException {
		
		mString.append(ch, start, length);
		
		super.characters(ch, start, length);
	}

	@Override
	public void endDocument() throws SAXException {
		
		super.endDocument();
	}

	@Override
	public void endElement(String uri, String localName, String qName)
			throws SAXException {
		
		if(localName.equals("packagename")){
			mPackageName = mString.toString().trim();
			mString.setLength(0);
			mPackageList.add(mPackageName);
		}
		
		super.endElement(uri, localName, qName);
	}

	@Override
	public void startDocument() throws SAXException {
		
		//init list
		mPackageList = new ArrayList<String>();
		
		super.startDocument();
	}

	@Override
	public void startElement(String uri, String localName, String qName,
			Attributes attributes) throws SAXException {
		
		if(localName.equals("packagename")){
			mPackageName = null;
		}
		
		super.startElement(uri, localName, qName, attributes);
	}
}
