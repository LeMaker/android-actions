package com.mot.dm.dbtool;

import java.util.*;

public class Record
    implements Comparable {
  public boolean containsMMS = false;
  public boolean containsBrowser = false;
  public boolean containsJavaApp = false;
  public boolean containsIM = false;

  public int mcc = 0;
  public int mnc = 0;
  public int mncLen;
  public int account_type;
  public String operator_name;
  public String key;
  public int operator_name_offset;
  public byte[] application_settings;
  public int application_settings_offset;

  public ArrayList arrXMLs = new ArrayList();
  public ArrayList arrIDs = new ArrayList();

  /* public void genCompareStr() {
     String str_mcc = paddingToSize(this.mcc + "");
     String str_mnc = paddingToSize(this.mnc + "");
     compareStr = str_mcc + str_mnc + operator_name + account_type;
   }*/

  private static String paddingToSize(String str) {
    int len = str.length();
    for (int i = len; i < 5; i++) {
      str = "0" + str;
    }
    return str;
  }

  public int setBlockApplicationSettings(byte[] metaInfo, byte[] data) throws
      Exception {
    application_settings = new byte[metaInfo.length + data.length];
    for (int i = 0; i < metaInfo.length; i++) {
      application_settings[i] = metaInfo[i];
    }
    int metaLength = metaInfo.length;
    for (int i = 0; i < data.length; i++) {
      application_settings[metaLength + i] = data[i];
    }
    return application_settings.length;
  }
// change here KEy generation.....
  public final static String createKey(String mcc, String mnc, String name,
                                       String type) throws
      Exception {
    StringBuffer sb_key = new StringBuffer();
    //sb_key.append(paddingToSize(mcc + ""));
    sb_key.append(mcc);
    sb_key.append(":");
    //sb_key.append(paddingToSize(mnc + ""));
    sb_key.append(mnc);
    sb_key.append(":");
    sb_key.append(name);
    sb_key.append(":");
    sb_key.append(type);
    return sb_key.toString();
  }

  public int compareTo(Object o) throws ClassCastException {

    Record record = (Record) o;
    if (mcc != record.mcc) {
      return mcc < record.mcc ? -1 : 1;
    }

    int currMnc = (mncLen == 3) ? (mnc + 1000) : mnc;
    int recordMnc = (record.mncLen == 3) ? (record.mnc + 1000) : record.mnc;

    if (currMnc != recordMnc) {
      return currMnc < recordMnc ? -1 : 1;
    }

    //int i = operator_name.compareTo(record.operator_name);
    // if( i != 0 )
    //   return i;
    if (this.operator_name_offset != record.operator_name_offset) {
      return operator_name_offset < record.operator_name_offset ? -1 : 1;
    }

    if (this.account_type != record.account_type) {
      return account_type < record.account_type ? -1 : 1;
    }

    return 0;
  }
}
