package com.mot.dm.dbtool;

/** This class contains the OMACP coding tables for elements
 *  and attributes needed by the OMACPParser.
 */

public class OMACP {

  public static int TAG = 1;
  public static int VAL = 2;
  public static int ATTR = 3;

  public static ValTab encode(String value, int tabName, int shift) throws Exception {

    int index;
    if (tabName == TAG) {
      return encodeTag(value, shift);
    }
    else if (tabName == VAL) {
      return encodeVal(value);
    }
    else if (tabName == ATTR) {
      return encodeAttr(value);
    }
    else {
      throw new Exception("Error: Unsupported table name...");
    }
  }

  public static ValTab encodeTag(String value, int shift) {
    ValTab result = new ValTab();
    int offset;
    //First table for tags
    offset = 0x05;
    for (int i = 0; i < TAG_TABLE_0.length; i++) {
      if (value.equals(TAG_TABLE_0[i])) {
        result.encVal = new Byte( (byte) ((offset + i) | shift));
        return result;
      }
    }
    //Second table for tags
    offset = 0x06;
    for (int i = 0; i < TAG_TABLE_1.length; i++) {
      if (value.equals(TAG_TABLE_1[i])) {
        result.encVal = new Byte( (byte) ((offset + i) | shift));
        result.table = result.table + 1;
        return result;
      }
    }
    return result;
  }

  public static ValTab encodeVal(String value) {
    ValTab result = new ValTab();
    int offset;
    //First table for values
    offset = 0x85;
    for (int i = 0; i < ATTR_VALUE_TABLE_0.length; i++) {
      if (value.equals(ATTR_VALUE_TABLE_0[i])) {
        result.encVal = new Byte( (byte) (offset + i));
        return result;
      }
    }
    //Second table for values
    offset = 0x85;
    for (int i = 0; i < ATTR_VALUE_TABLE_1.length; i++) {
      if (value.equals(ATTR_VALUE_TABLE_1[i])) {
        result.encVal = new Byte( (byte) (offset + i));
        result.table = result.table + 1;
        return result;
      }
    }
    return result;
  }

  public static ValTab encodeAttr(String value) {
    ValTab result = new ValTab();
    int offset;
    //First table for attributes
    offset = 0x05;
    for (int i = 0; i < ATTR_START_TABLE_0.length; i++) {
      if (value.equals(ATTR_START_TABLE_0[i])) {
        result.encVal = new Byte( (byte) (offset + i));
        return result;
      }
    }
    //Second table for attributes
    offset = 0x05;
    for (int i = 0; i < ATTR_START_TABLE_1.length; i++) {
      if (value.equals(ATTR_START_TABLE_1[i])) {
        result.encVal = new Byte( (byte) (offset + i));
        result.table = result.table + 1;
        return result;
      }
    }
    return result;
  }


  public static final String[] TAG_TABLE_0 = {
      //  page 0
      "wap-provisioningdoc", // 05
      "characteristic", // 06
      "parm" // 07
  };

  public static final String[] TAG_TABLE_1 = {
      //  page 1
      "characteristic", // 06
      "parm" // 07
  };

  public static final String[] ATTR_VALUE_TABLE_0 = {
      // ADDRTYPE //
      "IPV4", //  85
      "IPV6", //  86
      "E164", //  87
      "ALPHA", //  88
      "APN", //  89
      "SCODE", //  8a
      "TETRA-ITSI", //  8b
      "MAN", //  8c
      null, //  8d
      null, //  8e
      null, //  8f
      // CALLTYPE //
      "ANALOG-MODEM", //  90
      "V.120", //  91
      "V.110", //  92
      "X.31", //  93
      "BIT-TRANSPARENT", //  94
      "DIRECT-ASYNCHRONOUS-DATA-SERVICE", //  95
      null, //  96
      null, //  97
      null, //  98
      null, //  99
      // AUTHTYPE/PXAUTH-TYPE //
      "PAP", //  9a
      "CHAP", //  9b
      "HTTP-BASIC", //  9c
      "HTTP-DIGEST", //  9d
      "WTLS-SS", //  9e
      "MD5", //  9f  // OMA //
      // BEARER //
      null, //  a0
      null, //  a1
      "GSM-USSD", //  a2
      "GSM-SMS", //  a3
      "ANSI-136-GUTS", //  a4
      "IS-95-CDMA-SMS", //  a5
      "IS-95-CDMA-CSD", //  a6
      "IS-95-CDMA-PACKET", //  a7
      "ANSI-136-CSD", //  a8
      "ANSI-136-GPRS", //  a9
      "GSM-CSD", //  aa
      "GSM-GPRS", //  ab
      "AMPS-CDPD", //  ac
      "PDC-CSD", //  ad
      "PDC-PACKET", //  ae
      "IDEN-SMS", //  af
      "IDEN-CSD", //  b0
      "IDEN-PACKET", //  b1
      "FLEX/REFLEX", //  b2
      "PHS-SMS", //  b3
      "PHS-CSD", //  b4
      "TRETRA-SDS", //  b5
      "TRETRA-PACKET", //  b6
      "ANSI-136-GHOST", //  b7
      "MOBITEX-MPAK", //  b8
      "CDMA2000-1X-SIMPLE-IP", //  b9  // OMA //
      "CDMA2000-1X-MOBILE-IP", //  ba  // OMA //
      null, // bb
      null, // bc
      null, // bd
      null, // be
      null, // bf
      null, // c0
      null, // c1
      null, // c2
      null, // c3
      null, // c4
      // LINKSPEED //
      "AUTOBAUDING", //  c5
      null, // c6
      null, // c7
      null, // c8
      null, // c9
      // SERVICE //
      "CL-WSP", //  ca
      "CO-WSP", //  cb
      "CL-SEC-WSP", //  cc
      "CO-SEC-WSP", //  cd
      "CL-SEC-WTA", //  ce
      "CO-SEC-WTA", //  cf
      "OTA-HTTP-TO", //  d0  // OMA //
      "OTA-HTTP-TLS-TO", //  d1  // OMA //
      "OTA-HTTP-PO", //  d2  // OMA //
      "OTA-HTTP-TLS-PO", //  d3  // OMA //
      null, // d4
      null, // d5
      null, // d6
      null, // d7
      null, // d8
      null, // d9
      null, // da
      null, // db
      null, // dc
      null, // dd
      null, // de
      null, // df
      // AUTH-ENTITY //
      "AAA", //  e0  // OMA //
      "HA" //  e1  // OMA //
  };

  public static final String[] ATTR_VALUE_TABLE_1 = {

      /*  // AAUTHTYPE //
          ",", //  80  // OMA //
          "HTTP-", //  81  // OMA //
          "BASIC", //  82  // OMA //
          "DIGEST", //  83  // OMA //
          null, //  84
          null, // 85
       */
      null, // 85
      // ADDRTYPE //
      "IPV6", //  86  // OMA //
      "E164", //  87  // OMA //
      "ALPHA", //  88  // OMA //
      null, //  89
      null, //  8a
      null, //  8b
      null, //  8c
      "APPSRV", //  8d  // OMA //
      "OBEX", //  8e  // OMA //
      null, // 8f
      // AAUTHTYPE  //
      ",", //  90  // OMA //
      "HTTP-", //  81  // OMA //
      "BASIC", //  82  // OMA //
      "DIGEST" //  83  // OMA //
  };

  public static final String[] ATTR_START_TABLE_0 = {

      // Parm //
      "name", //  05
      "value", //  06
      "name=NAME", //  07
      "name=NAP-ADDRESS", //  08
      "name=NAP-ADDRTYPE", //  09
      "name=CALLTYPE", //  0a
      "name=VALIDUNTIL", //  0b
      "name=AUTHTYPE", //  0c
      "name=AUTHNAME", //  0d
      "name=AUTHSECRET", //  0e
      "name=LINGER", //  0f
      "name=BEARER", //  10
      "name=NAPID", //  11
      "name=COUNTRY", //  12
      "name=NETWORK", //  13
      "name=INTERNET", //  14
      "name=PROXY-ID", //  15
      "name=PROXY-PROVIDER-ID", //16
      "name=DOMAIN", //  17
      "name=PROVURL", //  18
      "name=PXAUTH-TYPE", //  19
      "name=PXAUTH-ID", //  1a
      "name=PXAUTH-PW", //  1b
      "name=STARTPAGE", //  1c
      "name=BASAUTH-ID", //  1d
      "name=BASAUTH-PW", //  1e
      "name=PUSHENABLED", //  1f
      "name=PXADDR", //  20
      "name=PXADDRTYPE", //  21
      "name=TO-NAPID", //  22
      "name=PORTNBR", //  23
      "name=SERVICE", //  24
      "name=LINKSPEED", //  25
      "name=DNLINKSPEED", //  26
      "name=LOCAL-ADDR", //  27
      "name=LOCAL-ADDRTYPE", //  28
      "name=CONTEXT-ALLOW", //  29
      "name=TRUST", //  2a
      "name=MASTER", //  2b
      "name=SID", //  2c
      "name=SOC", //  2d
      "name=WSP-VERSION", //  2e
      "name=PHYSICAL-PROXY-ID", //2f
      "name=CLIENT-ID", //  30
      "name=DELIVERY-ERR-SDU", // 31
      "name=DELIVERY-ORDER", //  32
      "name=TRAFFIC-CLASS", //  33
      "name=MAX-SDU-SIZE", //  34
      "name=MAX-BITRATE-UPLINK", //35
      "name=MAX-BITRATE-DNLINK", //36
      "name=RESIDUAL-BER", //  37
      "name=SDU-ERROR-RATIO", //  38
      "name=TRAFFIC-HANDL-PRIO", //39
      "name=TRANSFER-DELAY", //  3a
      "name=GUARANTEED-BITRATE-UPLINK", //3b
      "name=GUARANTEED-BITRATE-DNLINK", //3c
      "name=PXADDR-FQDN", //3d  // OMA //
      "name=PROXY-PW", //3e  // OMA //
      "name=PPGAUTH-TYPE", //3f  // OMA //
      null, // 40
      null, // 41
      null, // 42
      null, // 43
      null, // 44
      "version", // 45
      "version=1.0", // 46
      "name=PULLENABLED", //  47  // OMA //
      "name=DNS-ADDR", //  48  // OMA //
      "name=MAX-NUM-RETRY", //  49  // OMA //
      "name=FIRST-RETRY-TIMEOUT", //4a  // OMA //
      "name=REREG-THRESHOLD", //  4b  // OMA //
      "name=T-BIT", //  4c  // OMA //
      null, //  4d
      "name=AUTH-ENTITY", //  4e  // OMA //
      "name=SPI", //  4f  // OMA //
      // Characteristic //
      "type", //50
      "type=PXLOGICAL", //  51
      "type=PXPHYSICAL", //  52
      "type=PORT", //  53
      "type=VALIDITY", //  54
      "type=NAPDEF", //  55
      "type=BOOTSTRAP", //  56
      "type=VENDORCONFIG", //  57
      "type=CLIENTIDENTITY", //  58
      "type=PXAUTHINFO", //  59
      "type=NAPAUTHINFO", //  5a
      "type=ACCESS", //  5b  // OMA //
  };

  public static final String[] ATTR_START_TABLE_1 = {

      // Parm //
      "name", //  05  // OMA //
      "value", //  06  // OMA //
      "name=NAME", //  07  // OMA //
      null, //  08
      null, //  09
      null, //  0a
      null, //  0b
      null, //  0c
      null, //  0d
      null, //  0e
      null, //  0f
      null, //  10
      null, //  11
      null, //  12
      null, //  13
      "name=INTERNET", //  14  // OMA //
      null, //  15
      null, //  16
      null, //  17
      null, //  18
      null, //  19
      null, //  1a
      null, //  1b

      "name=STARTPAGE", //  1c  // OMA //
      null, //  1d
      null, //  1e
      null, //  1f
      null, //  20
      null, //  21

      "name=TO-NAPID", //  22  // OMA //
      "name=PORTNBR", //  23  // OMA //
      "name=SERVICE", //  24  // OMA //
      null, //  25
      null, //  26
      null, //  27
      null, //  28
      null, //  29
      null, //  2a
      null, //  2b

      null, //  2c  // OMA //
      null, //  2d

      "name=AACCEPT", //  2e  // OMA //
      "name=AAUTHDATA", //  2f  // OMA //
      "name=AAUTHLEVEL", //  30  // OMA //
      "name=AAUTHNAME", //  31  // OMA //
      "name=AAUTHSECRET", //  32  // OMA //
      "name=AAUTHTYPE", //  33  // OMA //
      "name=ADDR", //  34  // OMA //
      "name=ADDRTYPE", //  35  // OMA //
      "name=APPID", //  36  // OMA //
      "name=APROTOCOL", //  37  // OMA //
      "name=PROVIDER-ID", //  38  // OMA //
      "name=TO-PROXY", //  39  // OMA //
      "name=URI", //  3a  // OMA //
      "name=RULE", //  3b  // OMA //
      null, //  3c
      null, //  3d
      null, //  3e
      null, //  3f
      null, //  40
      null, //  41
      null, //  42
      null, //  43
      null, //  44
      null, //  45
      null, //  46
      null, //  47
      null, //  48
      null, //  49
      null, //  4a
      null, //  4b
      null, //  4c
      null, //  4d
      null, //  4e
      null, //  4f
      "type", //  50  // OMA //
      null, //  51
      null, //  52
      "type=PORT", //  53  // OMA //
      null, //54

      "type=APPLICATION", //  55  // OMA //
      "type=APPADDR", //  56  // OMA //
      "type=APPAUTH", //  57  // OMA //
      "type=CLIENTIDENTITY", //  58  // OMA //
      "type=RESOURCE", //  59  // OMA //

  };

}
