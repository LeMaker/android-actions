package com.mot.dm.dbtool;

import java.util.*;

public class Worker {
  public static String validateAndCreateMMSs(ArrayList arrMMSLines) throws
      Exception {
    String err = "";
    String line;
    String[] result_str;
    int colIndex;
    //set array with headers
    if (arrMMSLines.size() > 0) {
      line = (String) arrMMSLines.get(0); // get headers
      line += ",dummy"; //for supporting method split.
      result_str = line.split(",");
      MMS.colsNum = result_str.length;
      for (int i = 0; i < result_str.length; i++) {
        MMS.arrMMSLineHeaders.add(result_str[i]);
      }
    }

    for (int i = 1; i < arrMMSLines.size(); i++) { //start from 1 to skip header
      line = (String) arrMMSLines.get(i);
      if ( (Util.str(line)).length() == 0) {
        continue;
      }
      line += ",dummy"; //for supporting method split.
      result_str = line.split(",");
      Value[] result = Util.attStrToArrValue(result_str);

      //checlk for length
      if (result.length != MMS.colsNum) {
        err += "MMS, line#" + (i + 1) + ": The number of tokens should be " +
            MMS.colsNum + "\n";
        continue;
      }
      MMS mms = new MMS();
      colIndex = MMS.arrMMSLineHeaders.indexOf("mcc");
      if (colIndex == -1) {
        throw new Exception("The colomn 'mcc' is missing in the MMS file");
      }
      mms.mcc = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("mnc");
      if (colIndex == -1) {
        throw new Exception("The colomn 'mnc' is missing in the MMS file");
      }
      mms.mnc = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("operator_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'operator_name' is missing in the MMS file");
      }
      mms.operator_name = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("country_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'country_name' is missing in the MMS file");
      }
      mms.country_name = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("ppp_auth_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_type' is missing in the MMS file");
      }
      mms.ppp_auth_type = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("ppp_auth_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_name' is missing in the MMS file");
      }
      mms.ppp_auth_name = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("ppp_auth_secret");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_secret' is missing in the MMS file");
      }
      mms.ppp_auth_secret = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("ap_requires_auth");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ap_requires_auth' is missing in the MMS file");
      }
      mms.ap_requires_auth = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("service_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'service_name' is missing in the MMS file");
      }
      mms.service_name = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("gprs_access_point_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'gprs_access_point_name' is missing in the MMS file");
      }
      mms.gprs_access_point_name = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("dns_1");
      if (colIndex == -1) {
        throw new Exception("The colomn 'dns_1' is missing in the MMS file");
      }
      mms.dns_1 = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("dns_2");
      if (colIndex == -1) {
        throw new Exception("The colomn 'dns_2' is missing in the MMS file");
      }
      mms.dns_2 = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("proxy");
      if (colIndex == -1) {
        throw new Exception("The colomn 'proxy' is missing in the MMS file");
      }
      mms.proxy = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("port");
      if (colIndex == -1) {
        throw new Exception("The colomn 'port' is missing in the MMS file");
      }
      mms.port = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("proxy_auth_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_auth_name' is missing in the MMS file");
      }
      mms.proxy_auth_name = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("proxy_auth_secret");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_auth_secret' is missing in the MMS file");
      }
      mms.proxy_auth_secret = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("proxy_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_type' is missing in the MMS file");
      }
      mms.proxy_type = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("proxy_requires_auth");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_requires_auth' is missing in the MMS file");
      }
      mms.proxy_requires_auth = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("mms_url");
      if (colIndex == -1) {
        throw new Exception("The colomn 'mms_url' is missing in the MMS file");
      }
      mms.mms_url = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("account_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'account_type' is missing in the MMS file");
      }
      mms.account_type = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("bearer");
      if (colIndex == -1) {
        throw new Exception("The colomn 'bearer' is missing in the MMS file");
      }
      mms.bearer = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("proxy_auth_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_auth_type' is missing in the MMS file");
      }
      mms.proxy_auth_type = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("proxy_addr_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_addr_type' is missing in the MMS file");
      }
      mms.proxy_addr_type = result[colIndex];

      colIndex = MMS.arrMMSLineHeaders.indexOf("nap_addr_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'nap_addr_type' is missing in the MMS file");
      }
      mms.nap_addr_type = result[colIndex];

      //validate all data
      String lineErr = mms.validateData();
      if (lineErr.length() > 0) {
        err += "MMS, data not valid, line #" + (i + 1) +
            ": \n" + lineErr + "\n";
      }
      MMS.arrMMSFromLines.add(mms);
    }
    return err;
  }

  public static String validateAndCreateBrowsers(ArrayList arrBrowserLines) throws
      Exception {
    String err = "";
    String line;
    String[] result_str;
    int colIndex;
    //set array with headers
    if (arrBrowserLines.size() > 0) {
      line = (String) arrBrowserLines.get(0); // get headers
      line += ",dummy"; //for supporting method split.
      result_str = line.split(",");
      Browser.colsNum = result_str.length;
      for (int i = 0; i < result_str.length; i++) {
        Browser.arrBrowserLineHeaders.add(result_str[i]);
      }
    }

    for (int i = 1; i < arrBrowserLines.size(); i++) { //start from 1 to skip header
      line = (String) arrBrowserLines.get(i);
      if ( (Util.str(line)).length() == 0) {
        continue;
      }
      line += ",dummy"; //for supporting method split.
      result_str = line.split(",");
      Value[] result = Util.attStrToArrValue(result_str);

      //checlk for length
      if (result.length != Browser.colsNum) {
        err += "Browser, line#" + (i + 1) +
            ": The number of tokens should be " + Browser.colsNum + "\n";
        continue;
      }

      Browser browser = new Browser();

      colIndex = Browser.arrBrowserLineHeaders.indexOf("mcc");
      if (colIndex == -1) {
        throw new Exception("The colomn 'mcc' is missing in the Browser file");
      }
      browser.mcc = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("mnc");
      if (colIndex == -1) {
        throw new Exception("The colomn 'mnc' is missing in the Browser file");
      }
      browser.mnc = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("operator_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'operator_name' is missing in the Browser file");
      }
      browser.operator_name = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("country_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'country_name' is missing in the Browser file");
      }
      browser.country_name = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("ppp_auth_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_type' is missing in the Browser file");
      }
      browser.ppp_auth_type = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("ppp_auth_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_name' is missing in the Browser file");
      }
      browser.ppp_auth_name = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("ppp_auth_secret");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_secret' is missing in the Browser file");
      }
      browser.ppp_auth_secret = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("ap_requires_auth");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ap_requires_auth' is missing in the Browser file");
      }
      browser.ap_requires_auth = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("service_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'service_name' is missing in the Browser file");
      }
      browser.service_name = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("gprs_access_point_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'gprs_access_point_name' is missing in the Browser file");
      }
      browser.gprs_access_point_name = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("dns_1");
      if (colIndex == -1) {
        throw new Exception("The colomn 'dns_1' is missing in the Browser file");
      }
      browser.dns_1 = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("dns_2");
      if (colIndex == -1) {
        throw new Exception("The colomn 'dns_2' is missing in the Browser file");
      }
      browser.dns_2 = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("homepage_url");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'homepage_url' is missing in the Browser file");
      }
      browser.homepage_url = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("proxy");
      if (colIndex == -1) {
        throw new Exception("The colomn 'proxy' is missing in the Browser file");
      }
      browser.proxy = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("port");
      if (colIndex == -1) {
        throw new Exception("The colomn 'port' is missing in the Browser file");
      }
      browser.port = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("proxy_auth_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_auth_name' is missing in the Browser file");
      }
      browser.proxy_auth_name = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("proxy_auth_secret");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_auth_secret' is missing in the Browser file");
      }
      browser.proxy_auth_secret = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("proxy_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_type' is missing in the Browser file");
      }
      browser.proxy_type = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("proxy_requires_auth");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_requires_auth' is missing in the Browser file");
      }
      browser.proxy_requires_auth = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("account_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'account_type' is missing in the Browser file");
      }
      browser.account_type = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("bearer");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'bearer' is missing in the Browser file");
      }
      browser.bearer = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("proxy_auth_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_auth_type' is missing in the Browser file");
      }
      browser.proxy_auth_type = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("proxy_addr_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_addr_type' is missing in the Browser file");
      }
      browser.proxy_addr_type = result[colIndex];

      colIndex = Browser.arrBrowserLineHeaders.indexOf("nap_addr_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'nap_addr_type' is missing in the Browser file");
      }
      browser.nap_addr_type = result[colIndex];

      //validate all data
      String lineErr = browser.validateData();
      if (lineErr.length() > 0) {
        err += "Browser, data not valid, line #" + (i + 1) +
            ": \n" + lineErr + "\n";
      }
      Browser.arrBrowserFromLines.add(browser);
    }
    return err;
  }

  public static String validateAndCreateJavaApps(ArrayList arrJavaAppLines) throws
      Exception {
    String err = "";
    String line;
    String[] result_str;
    int colIndex;
    //set array with headers
    if (arrJavaAppLines.size() > 0) {
      line = (String) arrJavaAppLines.get(0); // get headers
      line += ",dummy"; //for supporting method split.
      result_str = line.split(",");
      JavaApp.colsNum = result_str.length;
      for (int i = 0; i < result_str.length; i++) {
        JavaApp.arrJavaAppLineHeaders.add(result_str[i]);
      }
    }

    for (int i = 1; i < arrJavaAppLines.size(); i++) { //start from 1 to skip header
      line = (String) arrJavaAppLines.get(i);
      if ( (Util.str(line)).length() == 0) {
        continue;
      }
      line += ",dummy"; //for supporting method split.
      result_str = line.split(",");
      Value[] result = Util.attStrToArrValue(result_str);

      //checlk for length
      if (result.length != JavaApp.colsNum) {
        err += "JavaApp, line#" + (i + 1) +
            ": The number of tokens should be " + JavaApp.colsNum + "\n";
        continue;
      }

      JavaApp javaApp = new JavaApp();

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("mcc");
      if (colIndex == -1) {
        throw new Exception("The colomn 'mcc' is missing in the JavaApp file");
      }
      javaApp.mcc = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("mnc");
      if (colIndex == -1) {
        throw new Exception("The colomn 'mnc' is missing in the JavaApp file");
      }
      javaApp.mnc = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("operator_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'operator_name' is missing in the JavaApp file");
      }
      javaApp.operator_name = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("account_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'account_type' is missing in the JavaApp file");
      }
      javaApp.account_type = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("country_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'country_name' is missing in the JavaApp file");
      }
      javaApp.country_name = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("ppp_auth_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_type' is missing in the JavaApp file");
      }
      javaApp.ppp_auth_type = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("ppp_auth_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_name' is missing in the JavaApp file");
      }
      javaApp.ppp_auth_name = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("ppp_auth_secret");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_secret' is missing in the JavaApp file");
      }
      javaApp.ppp_auth_secret = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("ap_requires_auth");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ap_requires_auth' is missing in the JavaApp file");
      }
      javaApp.ap_requires_auth = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("service_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'service_name' is missing in the JavaApp file");
      }
      javaApp.service_name = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("gprs_access_point_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'gprs_access_point_name' is missing in the JavaApp file");
      }
      javaApp.gprs_access_point_name = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("dns_1");
      if (colIndex == -1) {
        throw new Exception("The colomn 'dns_1' is missing in the JavaApp file");
      }
      javaApp.dns_1 = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("dns_2");
      if (colIndex == -1) {
        throw new Exception("The colomn 'dns_2' is missing in the JavaApp file");
      }
      javaApp.dns_2 = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("nap_addr_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'nap_addr_type' is missing in the JavaApp file");
      }
      javaApp.nap_addr_type = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("bearer");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'bearer' is missing in the JavaApp file");
      }
      javaApp.bearer = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("proxy");
      if (colIndex == -1) {
        throw new Exception("The colomn 'proxy' is missing in the Java file");
      }
      javaApp.proxy = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("proxy_addr_type");
      if (colIndex == -1) {
        throw new Exception("The colomn 'proxy_addr_type' is missing in the Java file");
      }
      javaApp.proxy_addr_type = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("port");
      if (colIndex == -1) {
        throw new Exception("The colomn 'port' is missing in the Java file");
      }
      javaApp.port = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("proxy_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_type' is missing in the Java file");
      }
      javaApp.proxy_type = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("proxy_requires_auth");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_requires_auth' is missing in the Java file");
      }
      javaApp.proxy_requires_auth = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("proxy_auth_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_auth_name' is missing in the Java file");
      }
      javaApp.proxy_auth_name = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("proxy_auth_secret");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_auth_secret' is missing in the Java file");
      }
      javaApp.proxy_auth_secret = result[colIndex];

      colIndex = JavaApp.arrJavaAppLineHeaders.indexOf("proxy_auth_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'proxy_auth_type' is missing in the Java file");
      }
      javaApp.proxy_auth_type = result[colIndex];

      //validate all data
      String lineErr = javaApp.validateData();
      if (lineErr.length() > 0) {
        err += "JavaApp, data not valid, line #" + (i + 1) +
            ": \n" + lineErr + "\n";
      }
      JavaApp.arrJavaAppFromLines.add(javaApp);
    }
    return err;
  }

  public static String validateAndCreateIMs(ArrayList arrIMLines) throws
      Exception {
    String err = "";
    String line;
    String[] result_str;
    int colIndex;
    //set array with headers
    if (arrIMLines.size() > 0) {
      line = (String) arrIMLines.get(0); // get headers
      line += ",dummy"; //for supporting method split.
      result_str = line.split(",");
      IM.colsNum = result_str.length;
      for (int i = 0; i < result_str.length; i++) {
        IM.arrIMLineHeaders.add(result_str[i]);
      }
    }

    for (int i = 1; i < arrIMLines.size(); i++) { //start from 1 to skip header
      line = (String) arrIMLines.get(i);
      if ( (Util.str(line)).length() == 0) {
        continue;
      }
      line += ",dummy"; //for supporting method split.
      result_str = line.split(",");
      Value[] result = Util.attStrToArrValue(result_str);

      //checlk for length
      if (result.length != IM.colsNum) {
        err += "IM, line#" + (i + 1) +
            ": The number of tokens should be " + IM.colsNum + "\n";
        continue;
      }

      IM im = new IM();

      colIndex = IM.arrIMLineHeaders.indexOf("mcc");
      if (colIndex == -1) {
        throw new Exception("The colomn 'mcc' is missing in the IM file");
      }
      im.mcc = result[colIndex];
      im.mcc.pos = i+1;

      colIndex = IM.arrIMLineHeaders.indexOf("mnc");
      if (colIndex == -1) {
        throw new Exception("The colomn 'mnc' is missing in the IM file");
      }
      im.mnc = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("operator_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'operator_name' is missing in the IM file");
      }
      im.operator_name = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("account_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'account_type' is missing in the IM file");
      }
      im.account_type = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("country_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'country_name' is missing in the IM file");
      }
      im.country_name = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("Brand_instance_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'Brand_instance_name' is missing in the IM file");
      }
      im.Brand_instance_name = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("IspProfileName");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'IspProfileName' is missing in the IM file");
      }
      im.IspProfileName = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("gprs_access_point_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'gprs_access_point_name' is missing in the IM file");
      }
      im.gprs_access_point_name = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ap_requires_auth");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ap_requires_auth' is missing in the IM file");
      }
      im.ap_requires_auth = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ppp_auth_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_type' is missing in the IM file");
      }
      im.ppp_auth_type = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ppp_auth_name");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_name' is missing in the IM file");
      }
      im.ppp_auth_name = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ppp_auth_secret");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ppp_auth_secret' is missing in the IM file");
      }
      im.ppp_auth_secret = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("nap_addr_type");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'nap_addr_type' is missing in the IM file");
      }
      im.nap_addr_type = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("bearer");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'bearer' is missing in the IM file");
      }
      im.bearer = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerAddr");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerAddr' is missing in the IM file");
      }
      im.ServerAddr = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerPort");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerPort' is missing in the IM file");
      }
      im.ServerPort = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ClientID");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ClientID' is missing in the IM file");
      }
      im.ClientID = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("PrivateKey1");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'PrivateKey1' is missing in the IM file");
      }
      im.PrivateKey1 = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("PrivateKey2");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'PrivateKey2' is missing in the IM file");
      }
      im.PrivateKey2 = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerPostURL");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerPostURL' is missing in the IM file");
      }
      im.ServerPostURL = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("LoginAutoResetConversation");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'LoginAutoResetConversation' is missing in the IM file");
      }
      im.LoginAutoResetConversation = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerCirSupported");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerCirSupported' is missing in the IM file");
      }
      im.ServerCirSupported = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerLoginMethod");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerLoginMethod' is missing in the IM file");
      }
      im.ServerLoginMethod = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerProxyAddress");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerProxyAddress' is missing in the IM file");
      }
      im.ServerProxyAddress = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerProxyPassword");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerProxyPassword' is missing in the IM file");
      }
      im.ServerProxyPassword = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerProxyUserName");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerProxyUserName' is missing in the IM file");
      }
      im.ServerProxyUserName = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerTransportType");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerTransportType' is missing in the IM file");
      }
      im.ServerTransportType = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerSMSDestinationAddress");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerSMSDestinationAddress' is missing in the IM file");
      }
      im.ServerSMSDestinationAddress = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServSMSCirSup");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServSMSCirSup' is missing in the IM file");
      }
      im.ServSMSCirSup = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("SvTCPCIRKeepTim");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'SvTCPCIRKeepTim' is missing in the IM file");
      }
      im.SvTCPCIRKeepTim = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("SvTCPCIRSup");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'SvTCPCIRSup' is missing in the IM file");
      }
      im.SvTCPCIRSup = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("ServerCloseSocketAfterResponse");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'ServerCloseSocketAfterResponse' is missing in the IM file");
      }
      im.ServerCloseSocketAfterResponse = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("AutoRefreshContactList");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'AutoRefreshContactList' is missing in the IM file");
      }
      im.AutoRefreshContactList = result[colIndex];

      colIndex = IM.arrIMLineHeaders.indexOf("FrbdUnauthoMsgs");
      if (colIndex == -1) {
        throw new Exception(
            "The colomn 'FrbdUnauthoMsgs' is missing in the IM file");
      }
      im.FrbdUnauthoMsgs = result[colIndex];

      //validate all data
      String lineErr = im.validateData();
      if (lineErr.length() > 0) {
        err += "IM, data not valid, line #" + (i + 1) +
            ": \n" + lineErr + "\n";
      }
      IM.arrIMFromLines.add(im);
    }
    return err;
  }

}
