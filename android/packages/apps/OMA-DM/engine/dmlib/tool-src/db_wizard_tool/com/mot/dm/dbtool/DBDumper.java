package com.mot.dm.dbtool;

import java.io.*;

public class DBDumper {
  byte[] buffer;
  byte[] header;
  byte[] operatorNames;
  byte[] carrierIndexTable;

// input counters - # bits/bytes
  int byteCount = 0;
  int bitCount = 0;

//output counters: #bits/bytes
  int outBytes = 0;
  int outBits = 0;

  public void dump(String dbPath) {
    try {
      buffer = readFile(dbPath);
      initByteArrays();
      printRecords();
    }
    catch (Exception e) {
      e.printStackTrace();
    }
  }

  public void printRecords() throws Exception {

    int recordsNum = carrierIndexTable.length / 9;

    for (int i = 0; i < recordsNum; i++) {
      String mcc;
      int mncLengthFlag;
      String mnc;
      int nameOffset;
      int nameLength;
      String name;
      int paymentType;
      String paymentTypeStr;
      int applicationAvailability;
      String usedApplications = "";

      mcc = getBits(Const.MCC_BITS, carrierIndexTable) + "";
      mncLengthFlag = getBits(Const.MNC_LENGTH_BITS, carrierIndexTable);
      mnc = getBits(Const.MNC_BITS, carrierIndexTable) + "";
      nameOffset = getBits(Const.OPERATOR_NAME_OFFSET_BITS, carrierIndexTable) *
          2;
      paymentType = getBits(Const.ACC_TYPE_BITS, carrierIndexTable);
      applicationAvailability = getBits(Const.APP_AVAIL_BITMAP_BITS,
                                        carrierIndexTable);
      getBits(Const.APP_SETTINGS_OFFSET_BITS, carrierIndexTable); //dummy call to shift counters

      //padding "0" to mnc if required
      int mncLength = (mncLengthFlag == 1) ? 3 : 2;
      while (mnc.length() < mncLength) {
        mnc = "0" + mnc;
      }

      //get operator name
      nameLength = ( ( (int) operatorNames[nameOffset]) & 0xFF);
      nameOffset++; //move to the begining of name
      nameLength--; // remove one (first) byte which contains size info
      byte[] byteName = new byte[nameLength];
      for (int j = 0; j < nameLength; j++) {
        byteName[j] = operatorNames[nameOffset + j];
      }
      name = new String(byteName, "UTF-8");

      //set payment type
      if (paymentType == Const.PREPAID_INT) {
        paymentTypeStr = Const.PREPAID;
      }
      else if (paymentType == Const.POSTPAID_INT) {
        paymentTypeStr = Const.POSTPAID;
      }
      else {
        paymentTypeStr = Const.BOTH;
      }

      //set applications availability
      if ( (applicationAvailability & Const.BROWSER_MASK) != 0) {
        usedApplications += "Browser ";
      }
      if ( (applicationAvailability & Const.MMS_MASK) != 0) {
        usedApplications += "MMS ";
      }

      if ( (applicationAvailability & Const.JAVA_MASK) != 0) {
        usedApplications += "Java ";
      }

      if ( (applicationAvailability & Const.IM_MASK) != 0) {
        usedApplications += "IM ";
      }

      //print one line
      System.out.println(
          "mcc: " + mcc + "     " +
          "mnc: " + mnc + "     " +
          "operator: " + name + "     " +
          "payment: " + paymentTypeStr + "     " +
          "applications: " + usedApplications
          );
    }
  }

  public void initByteArrays() throws Exception {
    int counter = 0;
    int max = 14;
    int index = 0;
    header = new byte[max];
    for (int i = 0; i < max; i++) {
      header[index++] = buffer[i];
      counter++;
    }

    int operatorTableSize = ( ( ( (int) header[1]) & 0xFF) << 24) +
        ( ( ( (int) header[2]) & 0xFF) << 16) +
        ( ( ( (int) header[3]) & 0xFF) << 8) +
        ( ( (int) header[4]) & 0xFF);

    operatorNames = new byte[operatorTableSize];
    max = counter + operatorTableSize;
    index = 0;
    for (int i = counter; i < max; i++) {
      operatorNames[index++] = buffer[i];
      counter++;
    }
    //String s = new String(operatorNames);
    //System.out.println(s);

    int carrierIndexTableSize = ( ( ( (int) header[5]) & 0xFF) << 24) +
        ( ( ( (int) header[6]) & 0xFF) << 16) +
        ( ( ( (int) header[7]) & 0xFF) << 8) +
        ( ( (int) header[8]) & 0xFF);
    carrierIndexTable = new byte[carrierIndexTableSize];
    max = counter + carrierIndexTableSize;
    index = 0;
    for (int i = counter; i < max; i++) {
      carrierIndexTable[index++] = buffer[i];
      counter++;
    }
    if ( (carrierIndexTableSize % 9) != 0) {
      throw new Exception(
          "One record in the Carrier Index Table has incorrect length.");
    }

  }

  public int getBits(int nBits, byte[] byteArr) {
    int res = 0;

    for (int i = nBits - 1; i >= 0; i--) {
      res |= (byteArr[outBytes] & (1 << (7 - outBits))) == 0 ? 0 : 1 << i;
      outBits++;
      if (outBits == 8) {
        outBits = 0;
        outBytes++;
      }
    }
    return res;
  }

  public byte[] readFile(String dbPath) throws Exception {
    FileInputStream is = new FileInputStream(dbPath);
    byte[] bytes = new byte[is.available()];
    is.read(bytes);
    is.close();
    return bytes;
  }
}
