package com.mot.dm.dbtool;

import java.util.*;
import java.io.*;

public class Generator {

  public static boolean REMOVE_WORKING_DIR = true;
  public static boolean USE_ZIP = true;
  public static boolean IS_INPUT_TXT_FILE;
  public static String ENCODING; // UTF-8 for csv and UTF-16 for txt

  public static String pathMMSFile = null;
  public static String pathBrowserFile = null;
  public static String pathJavaAppFile = null;
  public static String pathIMFile = null;

  public static ArrayList arrMMSLines = new ArrayList();
  public static ArrayList arrBrowserLines = new ArrayList();
  public static ArrayList arrJavaAppLines = new ArrayList();
  public static ArrayList arrIMLines = new ArrayList();

  //for one combine primary key for all settings
  public static HashMap hashXmlRecords = new HashMap();
  public static Record[] arraySortedRecords;
  public static StringBuffer operatorsNamesBuffer = new StringBuffer();
  public static ByteArray operatorsNamesBytes = new ByteArray();

  //### tables
  public static BlockTable operatorsNamesTable;
  public static BlockTable applicationsSettingsTable;
  public static BlockTable carrierIndexTable;
  public static BlockTable headerTable;

  public void proceed() throws Exception {
    Util.deleteDir(Const.WORKING_DIR);
    Util.deleteFile(Const.DB_FILE_NAME);
    Util.deleteFile(Const.ERROR_FILE_NAME);

    if (! (new File(Const.WORKING_DIR)).mkdir()) {
      throw new Exception(
          "Error!  Cannot create working directory for xml, wbxml and zip files");
    }

    fillArrLines();
    validateArrLines();
    createXmlMessages();
    Util.writeRecordsToFiles();
    Util.sortAllRecordsIntoArray();
    genenerateOperatorsNamesTable();
    Arrays.sort(arraySortedRecords); //re-sort again including newlly generated names offset
    // Util.printRecords(); // for debugging !!!!!!!!!!!!!
    genenerateApplicationsSettingsTable();
    genenerateCarrierIndexTable();
    genenerateHeaderTable();
    Validator.postValidate();
    writeTablesToFile();

    //clean working directory
    if (REMOVE_WORKING_DIR) {
      Util.deleteDir(Const.WORKING_DIR);
    }

    /// ------------- test --------------
    // Util.printRecords();
  }

  public void validateArrLines() throws Exception {

    String err = "";
    err += Worker.validateAndCreateMMSs(arrMMSLines);
    err += Worker.validateAndCreateBrowsers(arrBrowserLines);
    err += Worker.validateAndCreateJavaApps(arrJavaAppLines);
    err += Worker.validateAndCreateIMs(arrIMLines);

    if (err.length() > 0) {
      throw new Exception(err);
    }
  }

  public void createXmlMessages() throws Exception {
    Browser.createXmlMessages();
    MMS.createXmlMessages();
    JavaApp.createXmlMessages();

    // IM should be last to generate id_name for all other applications!!!
    // set flag to avoid duplicationNAP names before calling createXmlMessages()
    // The method setNAPFlagForIMAndValidate() should be changed for new  applications
    Validator.setNAPFlagForIMAndValidate();
    IM.createXmlMessages();
  }

  public void fillArrLines() throws Exception {
    if (pathMMSFile != null) {
      arrMMSLines = Util.readFile(pathMMSFile);
    }
    if (pathBrowserFile != null) {
      arrBrowserLines = Util.readFile(pathBrowserFile);
    }
    if (pathJavaAppFile != null) {
      arrJavaAppLines = Util.readFile(pathJavaAppFile);
    }
    if (pathIMFile != null) {
      arrIMLines = Util.readFile(pathIMFile);
    }
  }

  // set offset for Applications Settings for each record and generate Applications Setting Table
  public void genenerateApplicationsSettingsTable() throws Exception {
    int totalDataLength = 0;
    String err = ""; // collected all errors for setBlockApplicationsForOneRecord();

    // set offset and binary data for Applications Settings for each record
    for (int i = 0; i < arraySortedRecords.length; i++) {
      try {
        arraySortedRecords[i].application_settings_offset = totalDataLength;
        int currDataLength = setBlockApplicationsForOneRecord(
            arraySortedRecords[
            i]);
        totalDataLength += currDataLength;
      }
      catch (Exception ex) {
        err += ex.getMessage() + "\n";
      }
    }
    if (err.length() > 0) {
      throw new Exception(err);
    }

    // create appl settings table
    byte[] b;
    applicationsSettingsTable = new BlockTable(totalDataLength);
    for (int i = 0; i < arraySortedRecords.length; i++) {
      b = arraySortedRecords[i].application_settings;
      applicationsSettingsTable.appendData(b);
      arraySortedRecords[i].application_settings = null; // release memory !!!!

      ////////////////-- test --///////////////////
      /* int iii = ( ( ( (int) b[0]) & 0xff) << 16) +
           ( ( ( (int) b[1]) & 0xff) << 8) + ( ( (int) b[2]) & 0xff);
       System.out.println(arraySortedRecords[i].key.replaceAll(":", "_") +
                          " :  " + b.length + "  iii= " + (iii >>> 1) +
                          "  flag: " + (iii & 1));*/
      /////////////////////-- end test-- ///////////////////

    }

    Validator.validateMaxAppsSettingsLength(applicationsSettingsTable.length);
    int lastOffset = arraySortedRecords[arraySortedRecords.length -
        1].application_settings_offset;
    Validator.validateMaxAppsSettingsOffset(lastOffset);
  }

  // Choose between wbxml or zip (the smallest one) to be used;
  // adds 3 additional bytes with metadata and set binary data for the record
  // returns length for the application settings + 3 bytes
  public int setBlockApplicationsForOneRecord(Record record) throws Exception {
    String key = record.key;
    //String generalPath = Const.WORKING_DIR + Const.PATH_SEP +
    //    key.replaceAll(":", "_");
    String generalPath = Const.WORKING_DIR + Const.PATH_SEP +
        Util.generateFileNameFromKey(key);

    File w = new File(generalPath + ".wbxml");
    if (!w.exists()) {
      throw new Exception("Error: The file " + generalPath +
                          ".wbxml doesn't exist!");
    }

    FileInputStream in;
    boolean usedZip;
    // Choose between wbxml or zip (the smallest one) to be used in case if USE_ZIP
    // has not been set to false in function main().
    if (USE_ZIP) { //zip wbxml files
      File z = new File(generalPath + ".zip");
      if (!z.exists()) {
        throw new Exception("Error: The file " + generalPath +
                            ".zip doesn't exist!");
      }
      FileInputStream inw = new FileInputStream(w);
      FileInputStream inz = new FileInputStream(z);
      usedZip = inz.available() < inw.available();
      in = (usedZip) ? inz : inw;
    }
    else { // do not zip wbxml files
      usedZip = false;
      in = new FileInputStream(w);
    }

    byte[] appWbxmlSettings = new byte[in.available()];
    in.read(appWbxmlSettings);
    in.close();

    //get 3 bytes that provide info: for data length (23 bits) and compressed flag (1 bit)
    //and write it into metaInfo
    BlockTable metaInfo = new BlockTable( (Const.APP_BLOCK_LENGTH +
                                           Const.APP_COMPRESSWD_FLAG) / 8); // 3 bytes
    int appBlockSettingsLength = metaInfo.data.length + appWbxmlSettings.length;
    int compressFlag = (usedZip) ? 1 : 0;
    Util.addBitsToTable(metaInfo, appBlockSettingsLength,
                        Const.APP_BLOCK_LENGTH);
    Util.addBitsToTable(metaInfo, compressFlag,
                        Const.APP_COMPRESSWD_FLAG);
    record.setBlockApplicationSettings(metaInfo.data, appWbxmlSettings);

    Validator.validateAppBlockSettingsLength(appBlockSettingsLength,
                                             generalPath);

    return appBlockSettingsLength;
  }

  // generate operators names table with name length. assign offset for each record
  public static void genenerateOperatorsNamesTable() throws Exception {
    //operatorsNamesBytes
    HashMap hashNames = new HashMap();
    String name;

    for (int i = 0; i < arraySortedRecords.length; i++) {
      name = arraySortedRecords[i].operator_name;

      if (hashNames.containsKey(name)) {
        Integer offset = (Integer) hashNames.get(name);
        arraySortedRecords[i].operator_name_offset = offset.intValue();
      }
      else {

        byte[] nameByteArr = name.getBytes();
        // since operatorsNamesBytes.length are always even...
        int half_offset = operatorsNamesBytes.length() / 2;
        Validator.validateOperatorNameOffset(half_offset); //validate half_offset
        arraySortedRecords[i].operator_name_offset = half_offset;

        int nameLength = nameByteArr.length + 1; //length of name  + one byte to present this length.
        operatorsNamesBytes.addByte( (byte) nameLength);
        operatorsNamesBytes.addBytes(nameByteArr);
        if ( (operatorsNamesBytes.length() % 2) > 0) {
          //add one dummy byte to make it even
          operatorsNamesBytes.addByte( (byte) 0);
        }
        hashNames.put(name, new Integer(half_offset));
      }
    }

    //create operators Names Table
    operatorsNamesTable = new BlockTable(operatorsNamesBytes.length());
    operatorsNamesTable.appendData(operatorsNamesBytes.getBytes());

    Validator.validateOperatorNamesTableSize(operatorsNamesTable.length);

  }

  /*
//generate operators names table with name length. assign offset for each record
    public static void genenerateOperatorsNamesTable() throws Exception {
      HashMap hashNames = new HashMap();
      String name;
      boolean needAddDummyChar;
      for (int i = 0; i < arraySortedRecords.length; i++) {
        needAddDummyChar = false;
        name = arraySortedRecords[i].operator_name;

        if (hashNames.containsKey(name)) {
          Integer offset = (Integer) hashNames.get(name);
          arraySortedRecords[i].operator_name_offset = offset.intValue();
        }
        else {
          //length of name should be odd since we need to addd one byte which will
          //present this length. so length of name + one byte will be even.
          //then offset will be stored as half  offset/2

   int half_offset = operatorsNamesBuffer.toString().getBytes(ENCODING).length / 2;
          arraySortedRecords[i].operator_name_offset = half_offset;
          int nameLength = name.getBytes(ENCODING).length;
          if((nameLength % 2) == 0){
            //add one dummy char to make it odd
            needAddDummyChar = true;
            nameLength++;
          }
         // char c = (char) (name.getBytes().length + 1); //length of name  + one byte to present this length.
          char c = (char) (nameLength + 1); //length of name  + one byte to present this length.
          operatorsNamesBuffer.append(c);
          operatorsNamesBuffer.append(name);
          if(needAddDummyChar){
            operatorsNamesBuffer.append(0);
          }
          hashNames.put(name, new Integer(half_offset));
        }
        //validate real offset (half_offset * 2)
        Validator.validateOperatorNameOffset(arraySortedRecords[i].operator_name_offset * 2);
      }

      //create operators Names Table
      operatorsNamesTable = new BlockTable(operatorsNamesBuffer.toString().
                                           getBytes(ENCODING).length);
      operatorsNamesTable.appendData(operatorsNamesBuffer.toString().getBytes(ENCODING));

      Validator.validateOperatorNamesTableSize(operatorsNamesTable.length);

    }
   */

// generate carrier index table
  public static void genenerateCarrierIndexTable() throws Exception {
    //get size for all records and init carrierIndexTable
    int recordLengthBits = Const.MCC_BITS + Const.MNC_BITS +
        Const.OPERATOR_NAME_OFFSET_BITS + Const.ACC_TYPE_BITS +
        Const.APP_AVAIL_BITMAP_BITS + Const.APP_SETTINGS_OFFSET_BITS;
    int oneRecordLengthBytes = (recordLengthBits + 7) / 8;
    int tableLengthBytes = oneRecordLengthBytes * arraySortedRecords.length;
    carrierIndexTable = new BlockTable(tableLengthBytes);

    //write data from each record into arrier index table
    Record record;
    for (int i = 0; i < arraySortedRecords.length; i++) {
      record = arraySortedRecords[i];
      int appAvailBitmap = 0;
      //set mask for available applications
      if (record.containsBrowser) {
        appAvailBitmap |= Const.BROWSER_MASK;
      }
      if (record.containsIM) {
        appAvailBitmap |= Const.IM_MASK;
      }
      if (record.containsJavaApp) {
        appAvailBitmap |= Const.JAVA_MASK;
      }
      if (record.containsMMS) {
        appAvailBitmap |= Const.MMS_MASK;
      }
      //find value for mnc length: if mns 3 digits it is 1, otherwise - it is 0
      int mncLengthBitValue = (record.mncLen == 3) ? 1 : 0;

      Util.addBitsToTable(carrierIndexTable, record.mcc,
                          Const.MCC_BITS);
      Util.addBitsToTable(carrierIndexTable, mncLengthBitValue,
                          Const.MNC_LENGTH_BITS);
      Util.addBitsToTable(carrierIndexTable, record.mnc,
                          Const.MNC_BITS);
      Util.addBitsToTable(carrierIndexTable,
                          record.operator_name_offset,
                          Const.OPERATOR_NAME_OFFSET_BITS);
      Util.addBitsToTable(carrierIndexTable, record.account_type,
                          Const.ACC_TYPE_BITS);
      Util.addBitsToTable(carrierIndexTable, appAvailBitmap,
                          Const.APP_AVAIL_BITMAP_BITS);
      Util.addBitsToTable(carrierIndexTable,
                          record.application_settings_offset,
                          Const.APP_SETTINGS_OFFSET_BITS);
    }
  }

//### ==========  generate Header table ============  ###########
  public static void genenerateHeaderTable() throws Exception {
    int tableLengthBytes = (
        Const.HEAD_HEAD_SISE + Const.HEAD_OPERATOR_NAMES_TAB_SISE +
        Const.HEAD_CARRIER_INDEX_TAB_SISE + Const.HEAD_APP_SETTINGS_TAB_SIZE +
        Const.HEAD_DB_VERSION + 7) / 8;
    headerTable = new BlockTable(tableLengthBytes);
    //write header table
    Util.addBitsToTable(headerTable, tableLengthBytes,
                        Const.HEAD_HEAD_SISE);
    Util.addBitsToTable(headerTable, operatorsNamesTable.length,
                        Const.HEAD_OPERATOR_NAMES_TAB_SISE);
    Util.addBitsToTable(headerTable, carrierIndexTable.length,
                        Const.HEAD_CARRIER_INDEX_TAB_SISE);
    Util.addBitsToTable(headerTable,
                        applicationsSettingsTable.length,
                        Const.HEAD_APP_SETTINGS_TAB_SIZE);
    Util.addBitsToTable(headerTable, Const.HEAD_DB_VERSION,
                        Const.HEAD_DB_VERSION_TAB_SIZE);
  }

//### ==========  write main DB File and all tables ============  ###########
  public static void writeTablesToFile() throws Exception {
    // write sepparate files for the future testing
    Util.writeFile(Const.WORKING_DIR + Const.PATH_SEP + "header.dat",
                   headerTable.data);
    Util.writeFile(Const.WORKING_DIR + Const.PATH_SEP + "operators_names.dat",
                   operatorsNamesTable.data);
    Util.writeFile(Const.WORKING_DIR + Const.PATH_SEP + "app_settings.dat",
                   applicationsSettingsTable.data);
    Util.writeFile(Const.WORKING_DIR + Const.PATH_SEP + "carrier_index.dat",
                   carrierIndexTable.data);
    // write main DB file
    Util.writeDBFile();
  }

  public static void main(String[] args) {
    Const.HEAD_DB_VERSION = 1;

    for (int i = 0; i < args.length; i++) {
      if ("-mms".equals(args[i]) && args.length > i + 1) {
        pathMMSFile = args[++i];
        if (! (pathMMSFile.endsWith(".txt") || pathMMSFile.endsWith(".csv"))) {
          System.out.println(
              "Wrong input file type with MMS setting. Supported types: .txt or .csv");
          System.exit(1);
        }
      }
      else if ("-browser".equals(args[i]) && args.length > i + 1) {
        pathBrowserFile = args[++i];
        if (! (pathBrowserFile.endsWith(".txt") ||
               pathBrowserFile.endsWith(".csv"))) {
          System.out.println(
              "Wrong input file type with Browser setting. Supported types: .txt or .csv");
          System.exit(1);
        }
      }
      else if ("-java".equals(args[i]) && args.length > i + 1) {
        pathJavaAppFile = args[++i];
        if (! (pathJavaAppFile.endsWith(".txt") ||
               pathJavaAppFile.endsWith(".csv"))) {
          System.out.println(
              "Wrong input file type with Java setting. Supported types: .txt or .csv");
          System.exit(1);
        }
      }
      else if ("-im".equals(args[i]) && args.length > i + 1) {
        pathIMFile = args[++i];
        if (! (pathIMFile.endsWith(".txt") || pathIMFile.endsWith(".csv"))) {
          System.out.println(
              "Wrong input file type with IM setting. Supported types: .txt or .csv");
          System.exit(1);
        }
      }
      else if ("-nozip".equals(args[i])) {
        USE_ZIP = false;
      }
      else if ("-debug".equals(args[i])) {
        REMOVE_WORKING_DIR = false;
      }
      else if ("-dump".equals(args[i]) && args.length == 2) {
        DBDumper dbdumper = new DBDumper();
        System.out.println("============== Dumping DB File ============== ");
        dbdumper.dump(args[i + 1]);
        System.out.println(
            "============== End dumping DB File ============== \n");
        System.exit(0);
      }
      else {
        System.out.println(" Wrong parameters...");
        usage();
        System.exit(1);
      }
    }

    // validate input files and set file type (IS_INPUT_TXT_FILE)
    String err = Util.validateAndSetInputParms();
    if (err != null) {
      System.out.println(err);
      usage();
      System.exit(1);
    }

    Generator g = new Generator();
    try {
      g.proceed();
      System.out.println(
          "\n========================================================\n" +
          "DB generation finished successfully.\n" +
          "File '" + Const.DB_FILE_NAME + "' has been created.\n" +
          "========================================================\n");
    }
    catch (Exception e) {
      Util.deleteFile(Const.DB_FILE_NAME);

      System.out.println(
          "\n===================== Error ===============================");
      System.out.println(" DB generation failed.\n" +
                         " Please check file '" + Const.ERROR_FILE_NAME +
                         "' for more details\n");
      //System.out.println(e.getMessage());
      System.out.println(
          "===========================================================\n");
      //e.printStackTrace();
      try {
        Util.writeFile(Const.ERROR_FILE_NAME, e.getMessage());
      }
      catch (Exception ex) {
        ex.printStackTrace();
      }
    }
  }

  public static void usage() {
    System.out.println("\n usage:\n" +
                       "java com.mot.dm.dbtool.Generator [-mms <path>] [-browser <path>] [-java <path>] [-im <path>] \n" +
                       "or for test\n" +
                       "java com.mot.dm.dbtool.Generator -dump <path>/" +
                       Const.DB_FILE_NAME
                       );
  }
}
