package com.mot.dm.dbtool;

import java.util.*;

public class IM {
  public static int colsNum = 0; //number of columns in the comma sep file.
  public static ArrayList arrIMFromLines = new ArrayList();
  public static ArrayList arrIMLineHeaders = new ArrayList();

  public Value mcc = new Value();
  public Value mnc = new Value();
  public Value operator_name = new Value();
  public Value account_type = new Value();
  public Value country_name = new Value();
  public Value Brand_instance_name = new Value();
  public Value IspProfileName = new Value();
  public Value gprs_access_point_name = new Value();
  public Value ap_requires_auth = new Value();
  public Value ppp_auth_type = new Value();
  public Value ppp_auth_name = new Value();
  public Value ppp_auth_secret = new Value();
  public Value nap_addr_type = new Value();
  public Value bearer = new Value();

  public Value ServerAddr = new Value();
  public Value ServerPort = new Value();
  public Value ClientID = new Value();
  public Value PrivateKey1 = new Value();
  public Value PrivateKey2 = new Value();
  public Value ServerPostURL = new Value();
  public Value LoginAutoResetConversation = new Value();
  public Value ServerCirSupported = new Value();
  public Value ServerLoginMethod = new Value();
  public Value ServerProxyAddress = new Value();
  public Value ServerProxyPassword = new Value();
  public Value ServerProxyUserName = new Value();
  public Value ServerTransportType = new Value();
  public Value ServerSMSDestinationAddress = new Value();
  public Value ServSMSCirSup = new Value();
  public Value SvTCPCIRKeepTim = new Value();
  public Value SvTCPCIRSup = new Value();
  public Value ServerCloseSocketAfterResponse = new Value();
  public Value AutoRefreshContactList = new Value();
  public Value FrbdUnauthoMsgs = new Value();
  public boolean generateNAPXmlReq = true;

  // Data validation
  public String validateData() {
    StringBuffer err = new StringBuffer();
    err.append(Validator.validateMCC(mcc));
    err.append(Validator.validateMNC(mnc));
    err.append(Validator.validateStringValue(operator_name, true, -1,
                                             Const.OPERATOR_NAME_MAX_LENGTH,
                                             "operator_name"));
    err.append(Validator.validateAccountType(account_type));
    err.append(Validator.validateStringValue(Brand_instance_name, true, -1,
                                             Const.
                                             BRAND_INSTANCE_NAME_MAX_LENGTH,
                                             "Brand_instance_name"));
    err.append(Validator.validateStringValue(IspProfileName, true, -1,
                                             Const.ISP_PROFILE_NAME_MAX_LENGTH,
                                             "IspProfileName"));
    err.append(Validator.validateGprsAccessPoint(gprs_access_point_name,
                                                 nap_addr_type.val));
    err.append(Validator.validateApRequiresAuth(ap_requires_auth));
    err.append(Validator.validatePppAuthType(ppp_auth_type,
                                             ap_requires_auth.val));
    err.append(Validator.validatePppAuthName(ppp_auth_name,
                                             ap_requires_auth.val));
    err.append(Validator.validatePppAuthSecret(ppp_auth_secret,
                                               ap_requires_auth.val));
    err.append(Validator.validateNAPAddrType(nap_addr_type));
    err.append(Validator.validateBearer(bearer));

    // IM specific
    err.append(Validator.validateStringValue(ServerAddr, false, 0,
                                             Const.SERVER_ADDR_MAX_LENGTH,
                                             "ServerAddr")); ////ServerAddr
    err.append(Validator.validateIMServerPort(ServerPort)); ////ServerPort
    err.append(Validator.validateStringValue(ClientID, false, 0,
                                             Const.CLIENT_ID_MAX_LENGTH,
                                             "ClientID")); ////ClientID
    err.append(Validator.validateStringValue(PrivateKey1, false, 0,
                                             Const.PRIVATE_KEY_MAX_LENGTH,
                                             "PrivateKey1")); ////PrivateKey1
    err.append(Validator.validateStringValue(PrivateKey2, false, 0,
                                             Const.PRIVATE_KEY_MAX_LENGTH,
                                             "PrivateKey2")); ////PrivateKey2
    err.append(Validator.validateStringValue(ServerPostURL, false, 0,
                                             Const.SRV_POST_URL_MAX_LENGTH,
                                             "ServerPostURL")); ////ServerPostURL
    err.append(Validator.validateBoolValue(LoginAutoResetConversation, true,
                                           "LoginAutoResetConversation")); ////LoginAutoResetConversation
    err.append(Validator.validateBoolValue(ServerCirSupported, true,
                                           "ServerCirSupported")); ////ServerCirSupported
    err.append(Validator.validateValueFromDefinedList(ServerLoginMethod,
        Const.SERVER_LOGIN_METHODS, true, "ServerLoginMethod")); ////ServerLoginMethod
    err.append(Validator.validateStringValue(ServerProxyAddress, false, 0,
                                             Const.SRV_PROXY_ADDR_MAX_LENGTH,
                                             "ServerProxyAddress")); ////ServerProxyAddress
    err.append(Validator.validateStringValue(ServerProxyPassword, false, 0,
                                             Const.SRV_PROXY_PWD_MAX_LENGTH,
                                             "ServerProxyPassword")); ////ServerProxyPassword
    err.append(Validator.validateStringValue(ServerProxyUserName, false, 0,
                                             Const.
                                             SRV_PROXY_USR_NAME_MAX_LENGTH,
                                             "ServerProxyUserName")); ////ServerProxyUserName
    err.append(Validator.validateValueFromDefinedList(ServerTransportType,
        Const.SERVER_TRANSPORT_TYPES, true, "ServerTransportType")); ////ServerTransportType
    err.append(Validator.validateStringValue(ServerSMSDestinationAddress, false,
                                             0,
                                             Const.SRV_SMS_DEST_ADDR_MAX_LENGTH,
                                             "ServerSMSDestinationAddress")); ////ServerSMSDestinationAddress
    err.append(Validator.validateBoolValue(ServSMSCirSup, true, "ServSMSCirSup")); ////ServSMSCirSup
    err.append(Validator.validateIntegerValue(SvTCPCIRKeepTim, 0, 2147483647, true,
                                              "SvTCPCIRKeepTim")); ////SvTCPCIRKeepTim
    err.append(Validator.validateBoolValue(SvTCPCIRSup, true, "SvTCPCIRSup")); ////SvTCPCIRSup
    err.append(Validator.validateBoolValue(ServerCloseSocketAfterResponse, true,
                                           "ServerCloseSocketAfterResponse")); ////ServerCloseSocketAfterResponse
    err.append(Validator.validateBoolValue(AutoRefreshContactList, true,
                                           "AutoRefreshContactList")); ////AutoRefreshContactList
    err.append(Validator.validateBoolValue(FrbdUnauthoMsgs, true,
                                           "FrbdUnauthoMsgs")); ////FrbdUnauthoMsgs
    return err.toString();
  }

  public static void createXmlMessages() throws Exception {
    for (int i = 0; i < arrIMFromLines.size(); i++) {
      createXml((IM) arrIMFromLines.get(i));
    }
  }

  public static void createXml(IM im) throws Exception {
    String xml = "";
    //String key = im.mcc.val + ":" + im.mnc.val + ":" + im.operator_name.val +
    //    ":" + im.account_type.val.toUpperCase();
    String key = Record.createKey(im.mcc.val, im.mnc.val,
                                  im.operator_name.val,
                                  im.account_type.val.toUpperCase());

    // NAPDEF
    if (im.generateNAPXmlReq) {
      xml += " <characteristic type=\"NAPDEF\">\n";
      xml += "  <parm name=\"NAPID\"  value=\"" + im.IspProfileName.val +
          "\"/>\n";
      xml += "  <parm name=\"NAME\"  value=\"" + im.IspProfileName.val +
          "\"/>\n";
      xml += "  <parm name=\"BEARER\"  value=\"" + im.bearer.val + "\"/>\n";
      xml += "  <parm name=\"NAP-ADDRESS\"  value=\"" +
          im.gprs_access_point_name.val + "\"/>\n";
      xml += "  <parm name=\"NAP-ADDRTYPE\"  value=\"" + im.nap_addr_type.val +
          "\"/>\n";
      // NAPAUTHINFO
      if (Util.strToBool(im.ap_requires_auth.val)) {
        xml += "   <characteristic type=\"NAPAUTHINFO\">\n";
        xml += "    <parm name=\"AUTHTYPE\"  value=\"" + im.ppp_auth_type.val +
            "\"/>\n";
        xml += "    <parm name=\"AUTHNAME\"  value=\"" + im.ppp_auth_name.val +
            "\"/>\n";
        xml += "    <parm name=\"AUTHSECRET\"  value=\"" +
            im.ppp_auth_secret.val + "\"/>\n";
        xml += "   </characteristic>\n"; ///// NAPAUTHINFO
      }
      xml += " </characteristic>\n\n"; ///// NAPDEF
    }

    // APPLICATION
    xml += " <characteristic type=\"APPLICATION\">\n";
    xml += "  <parm name=\"APPID\"  value=\"wA\"/>\n";
    xml += "  <parm name=\"NAME\"  value=\"" + im.Brand_instance_name.val +
        "\"/>\n";
    xml += "  <parm name=\"TO-NAPID\"  value=\"" + im.IspProfileName.val +
        "\"/>\n";

    xml += "  <characteristic type=\"APPADDR\">\n"; //APPADDR
    xml += "   <parm name=\"ADDR\"  value=\"" + im.ServerAddr.val + "\"/>\n";
    xml += "   <characteristic type=\"PORT\">\n"; //PORT
    xml += "    <parm name=\"PORTNBR\"  value=\"" + im.ServerPort.val +
        "\"/>\n";
    xml += "   </characteristic>\n"; ///// PORT
    xml += "  </characteristic>\n"; ///// APPADDR

    xml += "  <characteristic type=\"APPAUTH\">\n"; //APPAUTH
    xml += "   <parm name=\"AAUTHLEVEL\"  value=\"APPSRV\"/>\n";
    xml += "   <parm name=\"AAUTHNAME\"  value=\"" + im.ClientID.val + "\"/>\n";
    xml += "   <parm name=\"AAUTHSECRET\"  value=\"" + im.PrivateKey1.val +
        "\"/>\n";
    xml += "  </characteristic>\n"; ///// APPAUTH

    xml += "  <characteristic type=\"APPAUTH\">\n"; //APPAUTH
    xml += "   <parm name=\"AAUTHLEVEL\"  value=\"CLIENT\"/>\n";
    xml += "   <parm name=\"AAUTHNAME\"  value=\"no in use\"/>\n";
    xml += "   <parm name=\"AAUTHSECRET\"  value=\"" + im.PrivateKey2.val +
        "\"/>\n";
    xml += "  </characteristic>\n"; ///// APPAUTH

    xml += "  <characteristic type=\"PROPRIETARY\">\n"; //PROPRIETARY
    xml += "   <parm name=\"ServerPostURL\"  value=\"" + im.ServerPostURL.val +
        "\"/>\n";
    xml += "   <parm name=\"LoginAutoResetConversation\"  value=\"" +
        im.LoginAutoResetConversation.val + "\"/>\n";
    xml += "   <parm name=\"ServerCirSupported\"  value=\"" +
        im.ServerCirSupported.val + "\"/>\n";
    xml += "   <parm name=\"ServerLoginMethod\"  value=\"" +
        im.ServerLoginMethod.val + "\"/>\n";
    xml += "   <parm name=\"ServerProxyAddress\"  value=\"" +
        im.ServerProxyAddress.val + "\"/>\n";
    xml += "   <parm name=\"ServerProxyPassword\"  value=\"" +
        im.ServerProxyPassword.val + "\"/>\n";
    xml += "   <parm name=\"ServerProxyUserName\"  value=\"" +
        im.ServerProxyUserName.val + "\"/>\n";
    xml += "   <parm name=\"ServerTransportType\"  value=\"" +
        im.ServerTransportType.val + "\"/>\n";
    xml += "   <parm name=\"ServerSMSDestinationAddress\"  value=\"" +
        im.ServerSMSDestinationAddress.val + "\"/>\n";
    xml += "   <parm name=\"ServSMSCirSup\"  value=\"" + im.ServSMSCirSup.val +
        "\"/>\n";
    xml += "   <parm name=\"SvTCPCIRKeepTim\"  value=\"" +
        im.SvTCPCIRKeepTim.val + "\"/>\n";
    xml += "   <parm name=\"SvTCPCIRSup\"  value=\"" + im.SvTCPCIRSup.val +
        "\"/>\n";
    xml += "   <parm name=\"ServerCloseSocketAfterResponse\"  value=\"" +
        im.ServerCloseSocketAfterResponse.val + "\"/>\n";
    xml += "   <parm name=\"AutoRefreshContactList\"  value=\"" +
        im.AutoRefreshContactList.val + "\"/>\n";
    xml += "   <parm name=\"FrbdUnauthoMsgs\"  value=\"" +
        im.FrbdUnauthoMsgs.val + "\"/>\n";
    xml += "  </characteristic>\n"; ///// PROPRIETARY
    xml += " </characteristic>\n\n"; ///// APPLICATION

    Util.addXmlToRecord(xml, key, im.Brand_instance_name.val, "IM");
  }

}
