package com.mot.dm.dbtool;

import java.util.*;
import java.io.*;
import java.util.zip.*;

public class Util {

  public static Value[] attStrToArrValue(String[] result_str) {
    Value[] vals = new Value[result_str.length];
    for (int i = 0; i < vals.length; i++) {
      Value val = new Value(result_str[i], i);
      vals[i] = val;
    }
    return vals;
  }

  public static String getIdNameFromRecords(String id, String key) throws
      Exception {
    int count = 0;
    String tmp;
    String tmp_id = id;
    boolean isUnique = false;
    Record rec = (Record) Generator.hashXmlRecords.get(key);
    if (rec == null) {
      return id;
    }
    // add count to the end of id to make it unique.
    while (!isUnique) {
      isUnique = true;
      for (int i = 0; i < rec.arrIDs.size(); i++) {
        tmp = (String) (rec.arrIDs.get(i));
        if (tmp.equalsIgnoreCase(tmp_id)) {
          isUnique = false;
          tmp_id = id + ++count;
          break;
        }
      }
    }
    return tmp_id;
  }

  public static void addXmlToRecord(String xml, String key, String id_name,
                                    String source) throws Exception {
    Record rec = (Record) Generator.hashXmlRecords.get(key);
    if (rec == null) {
      rec = new Record();
      rec.key = key;
      parseKeyToRecord(key, rec);
      //  rec.genCompareStr();
    }
    rec.arrIDs.add(id_name);
    rec.arrXMLs.add(xml);
    if ("BROWSER".equalsIgnoreCase(source)) {
      rec.containsBrowser = true;
    }
    else if ("MMS".equalsIgnoreCase(source)) {
      rec.containsMMS = true;
    }
    else if ("JAVA".equalsIgnoreCase(source)) {
      rec.containsJavaApp = true;
    }
    else if ("IM".equalsIgnoreCase(source)) {
      rec.containsIM = true;
    }
    else {
      throw new Exception(
          "Error! Unsupported application type during record creation.");
    }
    Generator.hashXmlRecords.put(key, rec);
  }

  //set mnc, mcc, provider_name and prepaid values for a record from combine key
  public static void parseKeyToRecord(String key, Record record) throws
      Exception {
    StringTokenizer st = new StringTokenizer(key, ":");
    if (st.countTokens() != 4) {
      throw new Exception("Error: The key '" + key + "' is wrong.");
    }
    String tmp;
    tmp = st.nextToken().trim();
    record.mcc = Integer.parseInt(tmp);
    tmp = st.nextToken().trim();
    record.mncLen = tmp.length();
    record.mnc = Integer.parseInt(tmp);
    tmp = st.nextToken().trim();
    record.operator_name = tmp;
    tmp = st.nextToken().trim();
    if (Const.PREPAID.equalsIgnoreCase(tmp)) {
      record.account_type = Const.PREPAID_INT;
    }
    else if (Const.POSTPAID.equalsIgnoreCase(tmp)) {
      record.account_type = Const.POSTPAID_INT;
    }
    else {
      record.account_type = Const.BOTH_INT;
    }
  }

  //creates xml, wbxml, zip files
  public static void writeRecordsToFiles() throws Exception {
    String omacp_xml;
    byte[] omacp_wbxml;
    Iterator keys = Generator.hashXmlRecords.keySet().iterator();
    while (keys.hasNext()) {
      String key = (String) keys.next();
      // create file name
      //String fileGenName = key.replaceAll(":", "_"); ///rem
      //fileGenName = key.replaceAll(" ", "_"); ///rem
      String fileGenName = generateFileNameFromKey(key);

      String fileXml = fileGenName + ".xml";
      String fileWbxml = fileGenName + ".wbxml";
      String fileZip = fileGenName + ".zip";

      omacp_xml = recordToXml(key);
      writeFile(fileXml, omacp_xml);


      WbxmlEncoder encoder = new WbxmlEncoder();
      //omacp_wbxml = encoder.encode(omacp_xml); //old usage
      omacp_wbxml = encoder.encode(new File(fileXml));
      writeFile(fileWbxml, omacp_wbxml);

      copyFile(fileXml, Const.WORKING_DIR + Const.PATH_SEP + fileXml);
      copyFile(fileWbxml, Const.WORKING_DIR + Const.PATH_SEP + fileWbxml);

      if (Generator.USE_ZIP) {
        writeZipFile(fileWbxml, fileZip);
        copyFile(fileZip, Const.WORKING_DIR + Const.PATH_SEP + fileZip);
        (new File(fileZip)).delete();
      }

      (new File(fileXml)).delete();
      (new File(fileWbxml)).delete();
    }
  }

  public static String recordToXml(String key) throws Exception {
    StringBuffer allXmlForKey = new StringBuffer();
    Record record = (Record) Generator.hashXmlRecords.get(key);
    if (record != null) {
      for (int i = 0; i < record.arrXMLs.size(); i++) {
        allXmlForKey.append( (String) record.arrXMLs.get(i));
      }
    }
    String validXml = "<wap-provisioningdoc version=\"1.1\">\n" +
        allXmlForKey.toString() + "</wap-provisioningdoc>\n";
    return validXml;
  }

///############################################################################
  public static void sortAllRecordsIntoArray() throws Exception {
    Generator.arraySortedRecords = new Record[Generator.hashXmlRecords.size()];
    Iterator iterator = Generator.hashXmlRecords.keySet().iterator();
    Record record;
    int count = 0;
    while (iterator.hasNext()) {
      record = (Record) Generator.hashXmlRecords.get(iterator.next());
      Generator.arraySortedRecords[count] = record;
      count++;
    }
    Arrays.sort(Generator.arraySortedRecords);
  }

  public static void addBitsToTable(BlockTable table, int value,
                                    int bitsSize) {
    for (int i = bitsSize - 1; i >= 0; i--) {
      table.data[table.byteCount] |=
          (value & (1 << i)) != 0 ? 1 << (7 - table.bitCount) : 0;

      table.bitCount++;

      if (table.bitCount == 8) {
        table.bitCount = 0;
        table.byteCount++;
      }
    }
  }

  public static ArrayList readFile(String path) throws Exception {
    ArrayList arr = new ArrayList();
    File f = new File(path);
    if (!f.exists()) {
      throw new Exception("Error: The file " + f.getAbsolutePath() +
                          " doesn't exist!");
    }
    // txt files should be encoded with UTF-16; csv - with UTF-8
    Reader fr = new InputStreamReader(new FileInputStream(f),
                                      Generator.ENCODING);

    BufferedReader br = new BufferedReader(fr);
    String line;
    int count = 0;
    while ( (line = br.readLine()) != null) {
      count++;
      line = new String(line.getBytes("UTF-8"));
      line = line.trim();
      if (line.length() > 0) {
        if (Generator.IS_INPUT_TXT_FILE) { //validate and replace tabs with ","
          if (line.indexOf(',') >= 0) {
            throw new Exception("Error: The line#" + count +
                                " in the text file " + f.getAbsolutePath() +
                                " cannot contains comma ',' as a value.");
          }
          line = line.replaceAll("\t", ",");
        }
        else if (line.indexOf('\t') >= 0) {
          throw new Exception("Error: The line#" + count +
                              " in the csv file " + f.getAbsolutePath() +
                              " contains tabulation as a value.");
        }
        arr.add(line);
      }
    }
    br.close();
    fr.close();
    return arr;
  }

///WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
/*  public static void writeFile(String path, String content) throws
      Exception {
    Writer fw = new OutputStreamWriter(new FileOutputStream(path), "UTF-8"); //Generator.ENCODING
    fw.write(content);
    fw.flush();
    fw.close();
  }*/

  //old function
  public static void writeFile(String path, String content) throws Exception {
  writeFile(path, content.getBytes());
   }

 public static void writeFile(String path, byte[] content) throws Exception {
   File f = new File(path);
   FileOutputStream fos = new FileOutputStream(f);
   fos.write(content);
   fos.flush();
   fos.close();
 }

  public static void writeDBFile() throws Exception {
    File f = new File(Const.DB_FILE_NAME);
    FileOutputStream fos = new FileOutputStream(f);
    // write header
    fos.write(Generator.headerTable.data);
    // write operators names table
    fos.write(Generator.operatorsNamesTable.data);
    // write carrier index table
    fos.write(Generator.carrierIndexTable.data);
    // write applications settings table
    fos.write(Generator.applicationsSettingsTable.data);

    fos.flush();
    fos.close();
  }

  public static void deleteFile(String path) {
    File f = new File(path);
    if (f.exists()) {
      f.delete();
    }
  }

  public static void writeZipFile(String inputFileName,
                                  String outputZipFileName) throws Exception {

    FileInputStream in = new FileInputStream(inputFileName);
    int wbxmlLen = in.available();
    byte[] wbxml_content = new byte[wbxmlLen];
    in.read(wbxml_content);
    in.close();

    Deflater compressor = new Deflater();
    compressor.setLevel(Deflater.BEST_COMPRESSION);
    compressor.setInput(wbxml_content);
    compressor.finish();

    ByteArrayOutputStream bos = new ByteArrayOutputStream(wbxmlLen);
    byte[] buf = new byte[1024];
    while (!compressor.finished()) {
      int count = compressor.deflate(buf);
      bos.write(buf, 0, count);
    }
    bos.close();
    byte[] wbxml_zip = bos.toByteArray();
    FileOutputStream fos = new FileOutputStream(outputZipFileName);
    fos.write(wbxml_zip);
    fos.flush();
    fos.close();
    //System.out.println("in: " + wbxmlLen + "  out: " + wbxml_zip.length);
  }


  public static void copyFile(String fromFile, String toFile) throws Exception {
    FileInputStream fis = new FileInputStream(fromFile);
    FileOutputStream fos = new FileOutputStream(toFile);
    byte[] buf = new byte[1024];
    int i = 0;
    while ( (i = fis.read(buf)) != -1) {
      fos.write(buf, 0, i);
    }
    if (fis != null) {
      fis.close();
    }
    if (fos != null) {
      fos.close();
    }
  }

  public static void deleteDir(String path) throws Exception {
    File f = new File(path);
    if (f.exists() && f.isDirectory()) {
      File[] files = f.listFiles();
      for (int i = 0; i < files.length; i++) {
        files[i].delete();
      }
      f.delete();
    }
  }

  public static String str(String s) {
    return (s != null) ? s : "";
  }

  public static boolean strToBool(String s) {
    s = (str(s)).toUpperCase();
    if (s.equals("YES") || s.equals("TRUE") || s.equals("1")) {
      return true;
    }
    return false;
  }

  public static boolean isFileExists(String path) {
    return (path == null) ? false : ( (new File(path)).exists());
  }

  // validate input files and set Encoding
  public static String validateAndSetInputParms() {
    String mms = Generator.pathMMSFile;
    String browser = Generator.pathBrowserFile;
    String java = Generator.pathJavaAppFile;
    String im = Generator.pathIMFile;
    String fileExtention = "";
    String tmp;
    //check that at least one file is presenting
    if (!isFileExists(mms) && !isFileExists(browser) && !isFileExists(java) &&
        !isFileExists(im)) {
      return "Error: At least one correct file (mms or browser or java or IM) must be provided !!!";
    }
    //set files type
    if (mms != null) {
      tmp = mms.substring(mms.length() - 4);
      if (fileExtention.length() > 0 && !fileExtention.equals(tmp)) {
        return
            "Error: All input files should have the same type: txt or csv !!!";
      }
      fileExtention = tmp;
    }
    if (browser != null) {
      tmp = browser.substring(browser.length() - 4);
      if (fileExtention.length() > 0 && !fileExtention.equals(tmp)) {
        return
            "Error: All input files should have the same type: txt or csv !!!";
      }
      fileExtention = tmp;
    }
    if (java != null) {
      tmp = java.substring(java.length() - 4);
      if (fileExtention.length() > 0 && !fileExtention.equals(tmp)) {
        return
            "Error: All input files should have the same type: txt or csv !!!";
      }
      fileExtention = tmp;
    }

    if (im != null) {
      tmp = im.substring(im.length() - 4);
      if (fileExtention.length() > 0 && !fileExtention.equals(tmp)) {
        return
            "Error: All input files should have the same type: txt or csv !!!";
      }
      fileExtention = tmp;
    }
    if (fileExtention.equals(".txt")) {
      Generator.IS_INPUT_TXT_FILE = true;
      Generator.ENCODING = "UTF-16";
    }
    else {
      Generator.ENCODING = "UTF-8";
      Generator.IS_INPUT_TXT_FILE = false;
    }
    return null;
  }

  public static String generateFileNameFromKey(String key) throws Exception{
    //byte[] bb = key.getBytes("UTF-8");
    //for (int i = 0; i < bb.length; i++) {
    //  System.out.print(Integer.toHexString(bb[i]) + " ");
    //}
    StringTokenizer st = new StringTokenizer(key, ":");
    StringBuffer fileName = new StringBuffer();
    String tmp;
    tmp = st.nextToken(); //mnc
    fileName.append(tmp).append("_");

    tmp = st.nextToken(); //mcc
    fileName.append(tmp).append("_");

    //carrier name... replace unicode to hex if required.
    tmp = st.nextToken();
    byte b;
    String  hex = "";
    String tmpHex;
    boolean hexRequired = false;
    byte[] bytes = tmp.getBytes();
    for (int i = 0; i < bytes.length; i++) {
      b = bytes[i];
      if (! ( (b >= 48 && b <= 57) || (b >= 65 && b <= 90) ||
             (b >= 97 && b <= 122))) {
        hexRequired = true;
      }
      tmpHex = Integer.toHexString(((int)b) & 0xff ).toUpperCase();
      hex += tmpHex;
    }

    tmp = (hexRequired) ? hex.replaceAll(" ", "_") : tmp.replaceAll(" ", "_"); //carrier name...
    fileName.append(tmp).append("_");
    tmp = st.nextToken(); //payment type
    fileName.append(tmp);
//System.out.println(">" + fileName.toString() + "<");
    return fileName.toString();
  }

  ///// ================== for test only..... ===========================
  public static void printRecords() {
    for (int i = 0; i < Generator.arraySortedRecords.length; i++) {
      System.out.println(
          "  mcc=" + Generator.arraySortedRecords[i].mcc +
          "  mnc=" + Generator.arraySortedRecords[i].mnc +
          "  mncLen=" + Generator.arraySortedRecords[i].mncLen +
          "  offset=" +
          Generator.arraySortedRecords[i].operator_name_offset +
          "  payment=" +
          Generator.arraySortedRecords[i].account_type);
    }

  }

}
