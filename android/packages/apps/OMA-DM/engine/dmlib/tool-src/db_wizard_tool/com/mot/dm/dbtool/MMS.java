package com.mot.dm.dbtool;

import java.util.*;

public class MMS {
  public static int colsNum = 0; //number of columns in the comma sep file.
  public static ArrayList arrMMSFromLines = new ArrayList();
  public static ArrayList arrMMSLineHeaders = new ArrayList();

  public Value mcc = new Value();
  public Value mnc = new Value();
  public Value operator_name = new Value();
  public Value country_name = new Value();
  public Value ppp_auth_type = new Value();
  public Value ppp_auth_name = new Value();
  public Value ppp_auth_secret = new Value();
  public Value ap_requires_auth = new Value();
  public Value service_name = new Value();
  public Value nap_addr_type = new Value();
  public Value gprs_access_point_name = new Value();
  public Value dns_1 = new Value();
  public Value dns_2 = new Value();
  public Value proxy = new Value();
  public Value proxy_addr_type = new Value();
  public Value port = new Value();
  public Value proxy_type = new Value();
  public Value proxy_requires_auth = new Value();
  public Value proxy_auth_name = new Value();
  public Value proxy_auth_secret = new Value();
  public Value proxy_auth_type = new Value();
  public Value account_type = new Value();
  public Value bearer = new Value();
  public Value mms_url = new Value();

  public String id_name = "";

  // Data validation
  public String validateData() {
    boolean isProxyEmpty = Validator.isValEmpty(proxy);
    StringBuffer err = new StringBuffer();
    err.append(Validator.validateMCC(mcc));
    err.append(Validator.validateMNC(mnc));
    err.append(Validator.validateStringValue(operator_name, true, -1,
                                             Const.OPERATOR_NAME_MAX_LENGTH,
                                             "operator_name"));
    err.append(Validator.validateApRequiresAuth(ap_requires_auth));
    err.append(Validator.validatePppAuthType(ppp_auth_type,
                                             ap_requires_auth.val));
    err.append(Validator.validatePppAuthName(ppp_auth_name,
                                             ap_requires_auth.val));
    err.append(Validator.validatePppAuthSecret(ppp_auth_secret,
                                               ap_requires_auth.val));
    err.append(Validator.validateStringValue(service_name, true, -1,
                                             Const.SERVICE_NAME_MAX_LENGTH,
                                             "service_name"));
    err.append(Validator.validateNAPAddrType(nap_addr_type));
    err.append(Validator.validateGprsAccessPoint(gprs_access_point_name,
                                                 nap_addr_type.val));
    err.append(Validator.validateDNS(dns_1));
    err.append(Validator.validateDNS(dns_2));
    err.append(Validator.validateBearer(bearer));
    err.append(Validator.validateProxy(proxy, proxy_addr_type.val));
    err.append(Validator.validateProxyAddrType(proxy_addr_type, isProxyEmpty));
    err.append(Validator.validatePort(port, isProxyEmpty));
    err.append(Validator.validateProxyType(proxy_type, isProxyEmpty));
    err.append(Validator.validateProxyRequiresAuth(proxy_requires_auth,
        isProxyEmpty));
    err.append(Validator.validateProxyAuthName(proxy_auth_name, isProxyEmpty,
                                               proxy_requires_auth.val));
    err.append(Validator.validateProxyAuthSecret(proxy_auth_secret,
                                                 isProxyEmpty,
                                                 proxy_requires_auth.val));
    err.append(Validator.validateProxyAuthType(proxy_auth_type, isProxyEmpty,
                                               proxy_requires_auth.val));
    err.append(Validator.validateAccountType(account_type));
    err.append(Validator.validateMMSUrl(mms_url));

    return err.toString();
  }

  public static void createXmlMessages() throws Exception {
    for (int i = 0; i < arrMMSFromLines.size(); i++) {
      createXml( (MMS) arrMMSFromLines.get(i));
    }
  }

  public static void createXml(MMS mms) throws Exception {
    String xml;
    //String key = mms.mcc.val + ":" + mms.mnc.val + ":" + mms.operator_name.val +
    //    ":" + mms.account_type.val.toUpperCase();
    String key = Record.createKey(mms.mcc.val, mms.mnc.val,
                                  mms.operator_name.val,
                                  mms.account_type.val.toUpperCase());

    mms.id_name = Util.getIdNameFromRecords(mms.service_name.val, key);

    // APPLICATION
    xml = " <characteristic type=\"APPLICATION\">\n";
    xml += "  <parm name=\"APPID\"  value=\"w4\"/>\n";
    xml += "  <parm name=\"NAME\"  value=\"" + mms.id_name + "\"/>\n";
    if (mms.proxy.val.length() > 0) {
      xml += "  <parm name=\"TO-PROXY\"  value=\"PX_" + mms.id_name + "\"/>\n";
    }
    else {
      xml += "  <parm name=\"TO-NAPID\"  value=\"NAP_" + mms.id_name + "\"/>\n";
    }
    xml += "  <parm name=\"ADDR\"  value=\"" + mms.mms_url.val + "\"/>\n";
    xml += " </characteristic>\n\n"; ///// APPLICATION
    if (mms.proxy.val.length() > 0) {
      // PXLOGICAL
      xml += " <characteristic type=\"PXLOGICAL\">\n";
      xml += "  <parm name=\"PROXY-ID\"  value=\"PX_" + mms.id_name + "\"/>\n";
      xml += "  <parm name=\"NAME\"  value=\"" + mms.id_name + "\"/>\n";
      // PXPHYSICAL
      xml += "   <characteristic type=\"PXPHYSICAL\">\n";
      xml += "    <parm name=\"PHYSICAL-PROXY-ID\"  value=\"PXPH_" + mms.id_name +
          "\"/>\n";
      xml += "    <parm name=\"PXADDR\"  value=\"" + mms.proxy.val + "\"/>\n";
      xml += "    <parm name=\"PXADDRTYPE\"  value=\"" +
          mms.proxy_addr_type.val + "\"/>\n";
      xml += "    <parm name=\"TO-NAPID\"  value=\"NAP_" + mms.id_name + "\"/>\n";
      if (mms.port.val.length() > 0) {
        //PORT
        xml += "    <characteristic type=\"PORT\">\n";
        xml += "     <parm name=\"PORTNBR\"  value=\"" + mms.port.val +
            "\"/>\n";
        if (mms.proxy_type.val.length() > 0) {
          xml += "     <parm name=\"SERVICE\"  value=\"" + mms.proxy_type.val +
              "\"/>\n";
        }
        xml += "    </characteristic>\n"; /////PORT
      }
      xml += "   </characteristic>\n"; ///// PXPHYSICAL
      if (Util.strToBool(mms.proxy_requires_auth.val)) {
        // PXAUTHINFO
        xml += "   <characteristic type=\"PXAUTHINFO\">\n";
        xml += "    <parm name=\"PXAUTH-TYPE\"  value=\"" +
            mms.proxy_auth_type.val + "\"/>\n";
        xml += "    <parm name=\"PXAUTH-ID\"  value=\"" +
            mms.proxy_auth_name.val + "\"/>\n";
        xml += "    <parm name=\"PXAUTH-PW\"  value=\"" +
            mms.proxy_auth_secret.val + "\"/>\n";
        xml += "   </characteristic>\n"; ///// PXAUTHINFO
      }
      xml += " </characteristic>\n\n"; ///// PXLOGICAL
    }
    // NAPDEF
    xml += " <characteristic type=\"NAPDEF\">\n";
    xml += "  <parm name=\"NAPID\"  value=\"NAP_" + mms.id_name + "\"/>\n";
    xml += "  <parm name=\"NAME\"  value=\"" + mms.id_name + "\"/>\n";
    xml += "  <parm name=\"BEARER\"  value=\"" + mms.bearer.val + "\"/>\n";
    xml += "  <parm name=\"NAP-ADDRESS\"  value=\"" +
        mms.gprs_access_point_name.val + "\"/>\n";
    xml += "  <parm name=\"NAP-ADDRTYPE\"  value=\"" + mms.nap_addr_type.val +
        "\"/>\n";
    if (mms.dns_1.val.length() > 0) {
      xml += "  <parm name=\"DNS-ADDR\"  value=\"" + mms.dns_1.val + "\"/>\n";
    }
    if (mms.dns_2.val.length() > 0) {
      xml += "  <parm name=\"DNS-ADDR\"  value=\"" + mms.dns_2.val + "\"/>\n";
    }
    if (Util.strToBool(mms.ap_requires_auth.val)) {
      // NAPAUTHINFO
      xml += "   <characteristic type=\"NAPAUTHINFO\">\n";
      xml += "    <parm name=\"AUTHTYPE\"  value=\"" + mms.ppp_auth_type.val +
          "\"/>\n";
      xml += "    <parm name=\"AUTHNAME\"  value=\"" + mms.ppp_auth_name.val +
          "\"/>\n";
      xml += "    <parm name=\"AUTHSECRET\"  value=\"" +
          mms.ppp_auth_secret.val + "\"/>\n";
      xml += "   </characteristic>\n"; ///// NAPAUTHINFO
    }
    xml += " </characteristic>\n\n"; ///// NAPDEF
    Util.addXmlToRecord(xml, key, mms.id_name, "MMS");
  }
}
