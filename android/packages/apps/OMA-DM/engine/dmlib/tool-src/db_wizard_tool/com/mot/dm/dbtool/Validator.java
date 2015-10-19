package com.mot.dm.dbtool;

import java.util.*;

public class Validator {

  // Validate that applications settings table length doesn't exceed max size
  // defined in the Const.java as ALIICATION_SETTINGS_MAX_SIZE in bytes
  public static void validateMaxAppsSettingsLength(int appsSettingsLength) throws
      Exception {
    if (appsSettingsLength >= Const.ALIICATION_SETTINGS_MAX_SIZE) {
      throw new Exception(
          "Error: Applications Settings Table length exceed " +
          (Const.ALIICATION_SETTINGS_MAX_SIZE / 8) + " bytes.");
    }
  }

  // Validate that last applications settings block offset (since it is the biggest)
  // doesn't exceed max size defined in the Const.java as APP_SETTINGS_OFFSET_BITS  in bits
  public static void validateMaxAppsSettingsOffset(int appsSettingsOffset) throws
      Exception {
    if (appsSettingsOffset >= Math.pow(2, Const.APP_SETTINGS_OFFSET_BITS)) {
      throw new Exception(
          "Error: Applications Settings block offset exceeds " +
          (Const.APP_SETTINGS_OFFSET_BITS / 8) + " bytes");
    }
  }

  // Validate that Data length for one Applications  doesn't exceed max size
  // defined in the Const.java as Const.APP_BLOCK_LENGTH in bits
  public static void validateAppBlockSettingsLength(int appBlockSettingsLength,
      String generalPath) throws
      Exception {
    if (appBlockSettingsLength >= Math.pow(2, Const.APP_BLOCK_LENGTH)) {
      throw new Exception(
          "Error: Applications Data length exceeds " + Const.APP_BLOCK_LENGTH +
          " bits for the " + generalPath);
    }
  }

  // Validate that operators names offset doesn't exceed max size
  // defined in the Const.java as Const.OPERATOR_NAME_OFFSET_BITS in bits
  public static void validateOperatorNameOffset(int operatorNameOffset) throws
      Exception {
    if (operatorNameOffset >= Math.pow(2, Const.OPERATOR_NAME_OFFSET_BITS)) {
      throw new Exception("Error: Operators Names offset exceeds " +
                          Const.OPERATOR_NAME_OFFSET_BITS + " bits");
    }
  }

  // Validate that operators names table size doesn't exceed max size
  // defined in the Const.java as Const.HEAD_OPERATOR_NAMES_TAB_SISE in bits
  public static void validateOperatorNamesTableSize(int operatorNamesTableSize) throws
      Exception {
    if (operatorNamesTableSize >=
        Math.pow(2, Const.HEAD_OPERATOR_NAMES_TAB_SISE)) {
      throw new Exception("Error: Operators Names Table size exceeds " +
                          (Const.HEAD_OPERATOR_NAMES_TAB_SISE / 8) + " bytes");
    }
  }

  // performs post validation (after all processes before generating files.
  public static void postValidate() throws Exception {
    String err = "";
    err += validateAccountTypesCompatibility_Post();

    // add more post validation here....

    if (err.length() > 0) {
      throw new Exception(err);
    }
  }

  // Validate that operators may have only (RPEPAID or/and POSTPAID) or (BOTH).
  // The BOTH cannot we with any other types.
  public static String validateAccountTypesCompatibility_Post() {

    String err = "";
    Record rec;
    int tmp;
    int acc_type;
    Map map = new HashMap();

    for (int i = 0; i < Generator.arraySortedRecords.length; i++) {
      rec = Generator.arraySortedRecords[i];

      String mnc = rec.mnc + "";
      while (mnc.length() < rec.mncLen) {
        mnc = "0" + mnc; // pedding with "0"
      }
      String key_tmp = rec.mcc + ":" + mnc + ":" + rec.operator_name;
      acc_type = rec.account_type;
      if (map.containsKey(key_tmp)) {
        tmp = ( (Integer) map.get(key_tmp)).intValue();
        acc_type += tmp;
        if (acc_type >= 2) {
          err += "mcc: " + rec.mcc + "  mnc: " + rec.mnc + "  operator name:  " +
              rec.operator_name + "\n";
        }
      }
      map.put(key_tmp, new Integer(acc_type));
    }
    if (err.length() > 0) {
      err = "The following records has incompatible account (payment) types;\n" +
          "'Prepaid' and/or 'Postpaid' cannot be with 'Both' for the following carriers:\n" +
          err;
    }
    return err;
  }

  //validate mcc: required, int, 0-999
  public static String validateMCC(Value parm) {
    if (isValEmpty(parm)) {
      return "  field 'mcc' is required and cannot be empty\n";
    }
    if (!isIntValid(parm, 0, 999)) {
      return "  field 'mcc' must be integer between 0 and 999\n";
    }
    if (parm.val.length() < 2 || parm.val.length() > 3) {
      return "  value in the field 'mcc' must have 2 or 3 digits\n";
    }

    return "";
  }

  //validate mnc: required, int, 0-999
  public static String validateMNC(Value parm) {
    if (isValEmpty(parm)) {
      return "  field 'mnc' is required and cannot be empty\n";
    }
    if (!isIntValid(parm, 0, 999)) {
      return "  field 'mnc' must be integer between 1 and 999\n";
    }
    if (parm.val.length() < 2 || parm.val.length() > 3) {
      return "  value in the field 'mnc' must have 2 or 3 digits\n";
    }

    return "";
  }

  //validate account_type: required, String, values: prepaid, postpaid or both
  public static String validateAccountType(Value parm) {
    if (isValEmpty(parm)) {
      return "  field 'account_type' is required and cannot be empty\n";
    }
    if (! (Const.PREPAID.equals(parm.val) || Const.POSTPAID.equals(parm.val) ||
           Const.BOTH.equals(parm.val))) {
      return "  field 'account_type' contains incorect value'" + parm.val +
          "'; the value can be '" + Const.PREPAID + "', '" + Const.POSTPAID +
          "' or '" + Const.BOTH + "'\n";
    }
    return "";
  }

  //validate string : required or not, max/min length.
  public static String validateStringValue(Value parm, boolean isRequired,
                                           int minLength, int maxLength,
                                           String fieldName) {
    if (!isRequired && isValEmpty(parm)) {
      return ""; //OK, value can be empty
    }

    if (isRequired && isValEmpty(parm)) {
      return "  field '" + fieldName + "' is required and cannot be empty\n";
    }
    if (maxLength > 0 && parm.val.getBytes().length > maxLength) {
      return "  field '" + fieldName + "' exceed max length: " + maxLength +
          " bytes\n";
    }
    if (minLength > 0 && parm.val.getBytes().length < minLength) {
      return "  field '" + fieldName + "' has value shorter then min length: " +
          minLength +
          " bytes\n";
    }
    return "";
  }

  //validate boolean : required or not, correct value.
  public static String validateBoolValue(Value parm, boolean isRequired,
                                         String fieldName) {
    if (isRequired && isValEmpty(parm)) {
      return "  field '" + fieldName + "' is required and cannot be empty\n";
    }
    if (!Const.TRUE.equals(parm.val) && !Const.FALSE.equals(parm.val)) {
      return "  field '" + fieldName +
          "' accept only values 'true' or 'false'\n";
    }
    return "";
  }

  public static String validateIntegerValue(Value parm, int min, int max,
                                            boolean isRequired,
                                            String fieldName) {
    boolean isParmValEmpty = isValEmpty(parm);

    if (isRequired && isParmValEmpty) {
      return
          "  field '" + fieldName + "' is required and cannot be empty\n";
    }
    if (!isParmValEmpty && !isIntValid(parm, min, max)) {
      return "  field '" + fieldName + "' contains invalid value'" + parm.val +
          "'\n";
    }
    return "";
  }

//validate value from predefined list (array)
  public static String validateValueFromDefinedList(Value parm,
      String[] listValues, boolean isRequired, String fieldName) {
    boolean isParmValEmpty = isValEmpty(parm);

    if (isParmValEmpty && isRequired) {
      return "  field '" + fieldName + "' is required and cannot be empty\n";
    }
    if (!isParmValEmpty && !isElementValid(listValues, parm.val)) {
      return "  field '" + fieldName + "' contains invalid value'" + parm.val +
          "'\n";
    }
    return "";
  }

  //validate operator_name: required, String, max length 128 (OPERATOR_NAME_MAX_LENGTH)
  /* public static String validateOperatorName(Value parm) {
    if (isValEmpty(parm)) {
      return "  field 'operator_name' is required and cannot be empty\n";
    }
    if (parm.val.getBytes().length > Const.OPERATOR_NAME_MAX_LENGTH) {
      return "  field 'operator_name' exceed max length " +
          Const.OPERATOR_NAME_MAX_LENGTH + " bytes\n";
    }
    return "";
     }*/

  //validate service_name: required, String, max length 128 (SERVICE_NAME_MAX_LENGTH)
  /* public static String validateServiceName(Value parm) {
     if (isValEmpty(parm)) {
       return "  field 'service_name' is required and cannot be empty\n";
     }
     if (!checkMaxLength(parm, Const.SERVICE_NAME_MAX_LENGTH)) {
       return "  field 'service_name' exceed max length " +
           Const.SERVICE_NAME_MAX_LENGTH + " bytes\n";
     }
     return "";
   }*/

  //validate ap_requires_auth: required, int, can be 0 or 1   wwwwwww
  public static String validateApRequiresAuth(Value parm) {
    if (isValEmpty(parm)) {
      return "  field 'ap_requires_auth' is required and cannot be empty\n";
    }
    if (!isIntValid(parm, 0, 1)) {
      return "  field 'ap_requires_auth' must be integer 0 or 1\n";
    }
    return "";
  }

  //validate ppp_auth_type: required if ap_requires_auth=1, String, can have only
  //values from array Const.NAPAUTHINFO_AUTHTYPE
  public static String validatePppAuthType(Value parm, String authReq) {
    boolean isParmValEmpty = isValEmpty(parm);

    if (authReq.equals("0") && !isParmValEmpty) {
      return "  field 'ppp_auth_type' can't have value if ap_requires_auth=0\n";
    }
    if (authReq.equals("1") && isParmValEmpty) {
      return "  field 'ppp_auth_type' can't be empty if ap_requires_auth=1\n";
    }
    if (!isParmValEmpty && !isElementValid(Const.NAPAUTHINFO_AUTHTYPE, parm.val)) {
      return "  field 'ppp_auth_type' contains invalid value'" + parm.val +
          "'\n";
    }
    return "";
  }

  //validate ppp_auth_name: can be set only if ap_requires_auth=1, String, max length 30 bytes
  public static String validatePppAuthName(Value parm, String authReq) {
    if (authReq.equals("0") && !isValEmpty(parm)) {
      return "  field 'ppp_auth_name' can't have value if ap_requires_auth=0\n";
    }
    if (!checkMaxLength(parm, Const.USER_NAME_MAX_LENGTH)) {
      return "  field 'ppp_auth_name' exceed max length " +
          Const.USER_NAME_MAX_LENGTH + "\n";
    }
    return "";
  }

  //validate ppp_auth_secret: can be set only if ap_requires_auth=1, String, max length 30 bytes
  public static String validatePppAuthSecret(Value val, String authReq) {
    if (authReq.equals("0") && !isValEmpty(val)) {
      return
          "  field 'ppp_auth_secret' can't have value if ap_requires_auth=0\n";
    }
    if (!checkMaxLength(val, Const.PASWORD_MAX_LENGTH)) {
      return "  field 'ppp_auth_secret' exceed max length " +
          Const.PASWORD_MAX_LENGTH + "\n";
    }
    return "";
  }

  //validate nap_addr_type: required, String, can have only values from array
  //Const.NAPDEF_NAPADDRTYPE
  public static String validateNAPAddrType(Value parm) {
    if (isValEmpty(parm)) {
      return "  field 'nap_addr_type' is required and cannot be empty\n";
    }
    if (!isElementValid(Const.NAPDEF_NAPADDRTYPE, parm.val)) {
      return "  field 'nap_addr_type' contains invalid value'" + parm.val +
          "'\n";
    }
    return "";
  }

  //validate bearer: required , String, can have only values from array
  //Const.BEARER
  public static String validateBearer(Value parm) {
    if (isValEmpty(parm)) {
      return "  field 'bearer' is required and cannot be empty\n";
    }
    if (!isElementValid(Const.BEARER, parm.val)) {
      return "  field 'bearer' contains invalid value'" + parm.val + "'\n";
    }
    return "";
  }

  //validate gprs_access_point_name: required , String, should match nap_addr_type
  //(NAPDEF.NAP-ADDRESS)
  public static String validateGprsAccessPoint(Value parm, String napAddrType) {
    if (isValEmpty(parm)) {
      return
          "  field 'gprs_access_point_name' is required and cannot be empty\n";
    }
    if (napAddrType.equals("IPV4") && !isValidIPV4Format(parm)) {
      return
          "  field 'gprs_access_point_name' contains invalid IPV4 format value: '" +
          parm.val + "'\n";
    }
    return "";
  }

  //validate dns: optional, String, should be in IPV4 format
  //(NAPDEF.NAP-ADDRESS)
  public static String validateDNS(Value parm) {
    if (!isValEmpty(parm)) {
      if (!isValidIPV4Format(parm)) {
        return
            "  field 'dns' contains invalid IPV4 format value: '" + parm.val +
            "'\n";
      }
    }
    return "";
  }

  //validate proxy: optional , String, should match proxy_addr_type
  //(PXPHYSICAL.PXADDR)
  public static String validateProxy(Value parm, String addrType) {
    if (!isValEmpty(parm)) {
      if (addrType.equals("IPV4") && !isValidIPV4Format(parm)) {
        return
            "  field 'proxy' contains invalid IPV4 format value: '" + parm.val +
            "'\n";
      }
    }
    return "";
  }

  //validate proxy_addr_type: can be set only if field "proxy" is not empty ,
  // String, can have only values from array Const.PXPHYSICAL_PXADDRTYPE
  public static String validateProxyAddrType(Value parm, boolean isProxyEmpty) {
    boolean isParmValEmpty = isValEmpty(parm);

    if (isProxyEmpty && !isParmValEmpty) {
      return
          "  field 'proxy_addr_type' can't have value if field proxy is empty\n";
    }
    if (!isParmValEmpty &&
        !isElementValid(Const.PXPHYSICAL_PXADDRTYPE, parm.val)) {
      return "  field 'proxy_addr_type' contains invalid value'" + parm.val +
          "'\n";
    }
    return "";
  }

//validate ServerPort for IM: is not empty, int, 0 - 65535   ===================================================
  public static String validateIMServerPort(Value parm) {
    if (isValEmpty(parm)) {
      return "  field 'port' required and can't be empty\n";
    }
    if (!isIntValid(parm, 0, 65535)) {
      return "  field 'port' contains invalid value'" + parm.val + "'\n";
    }
    return "";
  }

//================================================================================================
  //validate port: can be set only if field "proxy" is not empty, int, 0 - 65535
  public static String validatePort(Value parm, boolean isProxyEmpty) {
    boolean isParmValEmpty = isValEmpty(parm);

    if (isProxyEmpty && !isParmValEmpty) {
      return
          "  field 'port' can't have value if field proxy is empty\n";
    }
    if (!isParmValEmpty && !isIntValid(parm, 0, 65535)) {
      return "  field 'port' contains invalid value'" + parm.val +
          "'\n";
    }
    return "";
  }

  //validate proxy_type: can be set only if field "proxy" is not empty ,
  // String, can have only values from array Const.PORT_SERVICE
  public static String validateProxyType(Value parm, boolean isProxyEmpty) {
    boolean isParmValEmpty = isValEmpty(parm);

    if (isProxyEmpty && !isParmValEmpty) {
      return
          "  field 'proxy_type' can't have value if field proxy is empty\n";
    }
    if (!isParmValEmpty && !isElementValid(Const.PORT_SERVICE, parm.val)) {
      return "  field 'proxy_type' contains invalid value'" + parm.val +
          "'\n";
    }
    return "";
  }

  //validate proxy_requires_auth: must set only if field "proxy" is not empty,
  // int, can be 0 or 1
  public static String validateProxyRequiresAuth(Value parm,
                                                 boolean isProxyEmpty) {
    boolean isParmValEmpty = isValEmpty(parm);

    if (isProxyEmpty && !isParmValEmpty) {
      return
          "  field 'proxy_requires_auth' can't have value if field proxy is empty\n";
    }
    if (!isProxyEmpty && isParmValEmpty) {
      return "  field 'proxy_requires_auth' is required and cannot be empty if field proxy has been set\n";
    }
    if (!isProxyEmpty && !isIntValid(parm, 0, 1)) {
      return "  field 'proxy_requires_auth' must be integer 0 or 1\n";
    }
    return "";
  }

  //validate proxy_auth_type: required if field "proxy" is not empty and
  //proxy_requires_auth=1, String, can have only values from
  // array Const.PXAUTHINFO_PXAUTHTYPE
  public static String validateProxyAuthType(Value parm, boolean isProxyEmpty,
                                             String authReq) {
    boolean isParmValEmpty = isValEmpty(parm);

    if (isProxyEmpty && !isParmValEmpty) {
      return
          "  field 'proxy_requires_auth' can't have value if field proxy is empty\n";
    }
    if (authReq.equals("0") && !isParmValEmpty) {
      return
          "  field 'proxy_auth_type' can't have value if proxy_requires_auth=0\n";
    }
    if (authReq.equals("1") && isParmValEmpty) {
      return
          "  field 'proxy_auth_type' can't be empty if proxy_requires_auth=1\n";
    }

    if (!isParmValEmpty &&
        !isElementValid(Const.PXAUTHINFO_PXAUTHTYPE, parm.val)) {
      return "  field 'proxy_auth_type' contains invalid value'" + parm.val +
          "'\n";
    }
    return "";
  }

  //validate proxy_auth_name: can be set only if proxy_requires_auth=1, String, max length 32 bytes
  public static String validateProxyAuthName(Value parm, boolean isProxyEmpty,
                                             String authReq) {
    boolean isParmValEmpty = isValEmpty(parm);

    if (isProxyEmpty && !isParmValEmpty) {
      return
          "  field 'proxy_auth_name' can't have value if field proxy is empty\n";
    }
    if (authReq.equals("0") && !isParmValEmpty) {
      return
          "  field 'proxy_auth_name' can't have value if proxy_requires_auth=0\n";
    }
    if (!isParmValEmpty && !checkMaxLength(parm, Const.USER_NAME_MAX_LENGTH)) {
      return "  field 'proxy_auth_name' exceed max length " +
          Const.USER_NAME_MAX_LENGTH + "\n";
    }
    return "";
  }

  //validate proxy_auth_secret: can be set only if proxy_requires_auth=1, String, max length 32 bytes
  public static String validateProxyAuthSecret(Value parm, boolean isProxyEmpty,
                                               String authReq) {
    boolean isParmValEmpty = isValEmpty(parm);
    if (isProxyEmpty && !isParmValEmpty) {
      return
          "  field 'proxy_auth_secret' can't have value if field proxy is empty\n";
    }

    if (authReq.equals("0") && !isParmValEmpty) {
      return
          "  field 'proxy_auth_secret' can't have value if proxy_requires_auth=0\n";
    }
    if (!isParmValEmpty && !checkMaxLength(parm, Const.PASWORD_MAX_LENGTH)) {
      return "  field 'proxy_auth_secret' exceed max length " +
          Const.PASWORD_MAX_LENGTH + "\n";
    }
    return "";
  }

  //validate mms_url: required for MMS settings only, String
  public static String validateMMSUrl(Value parm) {
    if (isValEmpty(parm)) {
      return "  field 'mms_url' is required and cannot be empty\n";
    }
    return "";
  }

  // set flag to avoid duplication NAP names before calling createXmlMessages()
  // for IM. Also validate NAP names against other application settings.
  public static void setNAPFlagForIMAndValidate() throws Exception {
    String napId;
    String err = "";
    IM im;
    Browser br;
    MMS mms;
    JavaApp java;

    // Validate IspProfileName IM  with  id_name (service_name) for other settings
    // Browser
    for (int i = 0; i < Browser.arrBrowserFromLines.size(); i++) {
      br = (Browser) Browser.arrBrowserFromLines.get(i);
      napId = "NAP_" + br.id_name;
      for (int j = 0; j < IM.arrIMFromLines.size(); j++) {
        im = (IM) IM.arrIMFromLines.get(j);
        if (im.IspProfileName.val.equals(napId) && br.mcc.val.equals(im.mcc.val) &&
            br.mnc.val.equals(im.mnc.val) &&
            br.operator_name.val.equals(im.operator_name.val) &&
            br.account_type.val.equals(im.account_type.val)) {

          err += "Browser with mcc=" + br.mcc + " mnc=" + br.mnc +
              " operator_name=" + br.operator_name + " and IM with mcc=" +
              im.mcc + " mnc=" + im.mnc + " operator_name=" + im.operator_name +
              "\n";
        }
      }
    }

    //MMS
    for (int i = 0; i < MMS.arrMMSFromLines.size(); i++) {
      mms = (MMS) MMS.arrMMSFromLines.get(i);
      napId = "NAP_" + mms.id_name;
      for (int j = 0; j < IM.arrIMFromLines.size(); j++) {
        im = (IM) IM.arrIMFromLines.get(j);
        if (im.IspProfileName.val.equals(napId) &&
            mms.mcc.val.equals(im.mcc.val) && mms.mnc.val.equals(im.mnc.val) &&
            mms.operator_name.val.equals(im.operator_name.val) &&
            mms.account_type.val.equals(im.account_type.val)) {

          err += "MMS with mcc=" + mms.mcc + " mnc=" + mms.mnc +
              " operator_name=" + mms.operator_name + " and IM with mcc=" +
              im.mcc + " mnc=" + im.mnc + " operator_name=" + im.operator_name +
              "\n";
        }
      }
    }

    //Java
    for (int i = 0; i < JavaApp.arrJavaAppFromLines.size(); i++) {
      java = (JavaApp) JavaApp.arrJavaAppFromLines.get(i);
      napId = "NAP_" + java.id_name;
      for (int j = 0; j < IM.arrIMFromLines.size(); j++) {
        im = (IM) IM.arrIMFromLines.get(j);
        if (im.IspProfileName.val.equals(napId) &&
            java.mcc.val.equals(im.mcc.val) && java.mnc.val.equals(im.mnc.val) &&
            java.operator_name.val.equals(im.operator_name.val) &&
            java.account_type.val.equals(im.account_type.val)) {

          err += "Java with mcc=" + java.mcc + " mnc=" + java.mnc +
              " operator_name=" + java.operator_name + " and IM with mcc=" +
              im.mcc + " mnc=" + im.mnc + " operator_name=" + im.operator_name +
              "\n";
        }
      }
    }

    if (err.length() > 0) {
      err =
          "Field 'IspProfileName' from IM settings conflicts with 'service_name' for:\n" +
          err;
      throw new Exception(err);
    }

    //set flag to avoid duplication NAP names for IM. checking all gprs (NAP) setting for
    //the same combin. key and IspProfileName.
    ArrayList arrTmpIM = new ArrayList();
    TreeMap map = new TreeMap();

    for (int i = 0; i < IM.arrIMFromLines.size(); i++) {
      IM imTmp = (IM) IM.arrIMFromLines.get(i);
      for (int j = 0; j < arrTmpIM.size(); j++) {
        im = (IM) arrTmpIM.get(j);
        //check if match found
        if (imTmp.mcc.val.equals(im.mcc.val) && imTmp.mnc.val.equals(im.mnc.val) &&
            imTmp.operator_name.val.equals(im.operator_name.val) &&
            imTmp.account_type.val.equals(im.account_type.val) &&
            imTmp.IspProfileName.val.equals(im.IspProfileName.val)) {
          //check NAP settings for match
          if (imTmp.gprs_access_point_name.val.equals(im.gprs_access_point_name.
              val) &&
              imTmp.ap_requires_auth.val.equals(im.ap_requires_auth.val) &&
              imTmp.ppp_auth_type.val.equals(im.ppp_auth_type.val) &&
              imTmp.ppp_auth_name.val.equals(im.ppp_auth_name.val) &&
              imTmp.ppp_auth_secret.val.equals(im.ppp_auth_secret.val) &&
              imTmp.nap_addr_type.val.equals(im.nap_addr_type.val) &&
              imTmp.bearer.val.equals(im.bearer.val)) {
            // match found with the same NAP settings.
            ( (IM) IM.arrIMFromLines.get(i)).generateNAPXmlReq = false;
          }
          else {
            throw new Exception("IM dataset(s) are incorrect in the lines " +
                                im.mcc.pos + " and " + imTmp.mcc.pos +
                                ": \nGPRS (NAP) use the same name (field 'IspProfileName') with different settings.\n");

          } //end check NAP settings for match
        } //end match found

      }
      arrTmpIM.add(imTmp);
    }
  }

////////////////////////  General  /////////////////////
  public static boolean isValEmpty(Value parm) {
    return (parm == null || parm.val.trim().equals("")) ? true : false;
  }

  public static boolean isIntValid(Value parm, int minPossible, int maxPossible) {
    try {
      int i = Integer.parseInt(parm.val);
      if (minPossible > -1 && i < minPossible) {
        return false;
      }
      if (maxPossible > -1 && i > maxPossible) {
        return false;
      }
    }
    catch (Exception e) {
      return false;
    }
    return true;
  }

  public static boolean checkMaxLength(Value parm, int maxLen) {
    if (parm.val.getBytes().length > maxLen) {
      return false;
    }
    return true;
  }

  public static boolean isElementValid(String[] arr, String element) {
    for (int i = 0; i < arr.length; i++) {
      if (arr[i].equals(element)) {
        return true;
      }
    }
    return false;
  }

  public static boolean isValidIPV4Format(Value parm) {
    StringTokenizer st = new StringTokenizer(parm.val, ".");
    Value tmp;
    if (st.countTokens() > 4) {
      return false;
    }
    while (st.hasMoreTokens()) {
      tmp = new Value(Util.str(st.nextToken()));
      if (!isIntValid(tmp, 0, 255)) {
        return false;
      }
    }
    return true;
  }

}
