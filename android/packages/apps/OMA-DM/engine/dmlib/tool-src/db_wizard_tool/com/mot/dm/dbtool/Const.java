package com.mot.dm.dbtool;

public class Const {
  public static int HEAD_DB_VERSION = 1;
  public static final String TRUE = "true";
  public static final String FALSE = "false";

  // max length for DB file
  public static final int ALIICATION_SETTINGS_MAX_SIZE = 2 * 1024 * 1024; // 2M

  // masks for Application availability bitmap (max 8)
  public static int BROWSER_MASK = 0x0001;
  public static int IM_MASK = 0x0002;
  public static int JAVA_MASK = 0x0004;
  public static int MMS_MASK = 0x0008;

  //size for the fields for carrier index table in bits
  public static int MCC_BITS = 10;
  public static int MNC_LENGTH_BITS = 1;
  public static int MNC_BITS = 10;
  public static int OPERATOR_NAME_OFFSET_BITS = 17;
  public static int ACC_TYPE_BITS = 2;
  public static int APP_AVAIL_BITMAP_BITS = 8;
  public static int APP_SETTINGS_OFFSET_BITS = 3 * 8; //3 bytes

  //size for the application settings block in bits
  public static int APP_BLOCK_LENGTH = 23;
  public static int APP_COMPRESSWD_FLAG = 1;

  //size for operator names and service name in bytes
  public static int OPERATOR_NAME_MAX_LENGTH = 128;
  public static int SERVICE_NAME_MAX_LENGTH = 128;

  // === IM related fields and constrains ===
  public static int BRAND_INSTANCE_NAME_MAX_LENGTH = 128;
  public static int ISP_PROFILE_NAME_MAX_LENGTH = 128;
  public static int SERVER_ADDR_MAX_LENGTH = 127;
  public static int CLIENT_ID_MAX_LENGTH = 200;
  public static int PRIVATE_KEY_MAX_LENGTH = 120;
  public static int SRV_POST_URL_MAX_LENGTH = 200;
  public static int SRV_PROXY_ADDR_MAX_LENGTH = 200;
  public static int SRV_PROXY_PWD_MAX_LENGTH = 64;
  public static int SRV_PROXY_USR_NAME_MAX_LENGTH = 64;
  public static int SRV_SMS_DEST_ADDR_MAX_LENGTH = 76;


  // supported Server Login Methods for IM
  public static String[] SERVER_LOGIN_METHODS = {
      "2WAY", "4WAY", "AOL"};
  // supported Server Transport Types for IM
  public static String[] SERVER_TRANSPORT_TYPES = {
      "HTTP", "SMS"};

  // === end IM related definitions ===

  // size for NAP and Proxy UserID and Password
  public static int USER_NAME_MAX_LENGTH = 32;
  public static int PASWORD_MAX_LENGTH = 32;

  //sizes for header table in bits
  public static int HEAD_HEAD_SISE = 1 * 8; //one byte
  public static int HEAD_OPERATOR_NAMES_TAB_SISE = 4 * 8; //4 byte
  public static int HEAD_CARRIER_INDEX_TAB_SISE = 4 * 8; //4 byte
  public static int HEAD_APP_SETTINGS_TAB_SIZE = 4 * 8; //4 byte
  public static int HEAD_DB_VERSION_TAB_SIZE = 1 * 8; //1 byte

  // files names
  public static String WORKING_DIR = "db_xml_wbxml_files";
  public static String DB_FILE_NAME = "lj_dmwizard.dat";
  public static String ERROR_FILE_NAME = "db_errors.txt";

  public static String PATH_SEP = java.io.File.separator;

  //account type
  public static String PREPAID = "prepaid";
  public static int PREPAID_INT = 0;
  public static String POSTPAID = "postpaid";
  public static int POSTPAID_INT = 1;
  public static String BOTH = "both";
  public static int BOTH_INT = 2;

  // supported authentication type for  NAPAUTHINFO.AUTHTYPE
  public static String[] NAPAUTHINFO_AUTHTYPE = {
      "PAP"};

  // supported NAPDEF address type  NAPDEF.NAP-ADDRTYPE
  public static String[] NAPDEF_NAPADDRTYPE = {
      "IPV4", "IPV6", "E164", "ALPHA", "APN"};

  // supported BEARER
  public static String[] BEARER = {
      "GSM-GPRS"};

  // supported PROXY address type  PXPHYSICAL.PXADDRTYPE
  public static String[] PXPHYSICAL_PXADDRTYPE = {
      "IPV4", "IPV6", "E164", "ALPHA"};

  // supported proxy_type mapped to  PORT.SERVICE
  public static String[] PORT_SERVICE = {
      "CL-WSP", "CO-WSP", "CL-SEC-WSP", "CO-SEC-WSP", "CO-SEC-WTA",
      "CL-SEC-WTA", "OTA-HTTP-TO", "OTA-HTTP-TLS-TO", "OTA-HTTP-PO",
      "OTA-HTTP-TLS-PO"};

  // supported proxy_auth_type authentication type for PXAUTHINFO.PXAUTH-TYPE
  public static String[] PXAUTHINFO_PXAUTHTYPE = {
      "HTTP-BASIC"};

//////////////////   Mapping //////////////////////////////
  /*
      1.	mcc:			n/a
   2.	mnc:				n/a
   3.	account_type:			n/a
   4.	operator_name:		        n/a
   5.	country_name: 		        n/a
   6.	ppp_auth_type:		        NAPAUTHINFO.AUTHTYPE
   7.	ppp_auth_name:		        NAPAUTHINFO.AUTHNAME
   8.	ppp_auth_secret:		NAPAUTHINFO.AUTHSECRET
   9.	ap_requires_auth:		n/a
   10.	service_name:			APPLICATION.NAME, PXLOGICAL.NAME, NAPDEF.NAME
   11.	gprs_access_point_name:	        NAPDEF.NAP-ADDRESS
   12.	dns_1:				NAPDEF.DNS-ADDR
   13.	dns_2:				NAPDEF.DNS-ADDR
   14.	nap_addr_type:		        NAPDEF.NAP-ADDRTYPE
   15.	bearer:				NAPDEF.BEARER
   16.	proxy:				PXPHYSICAL.PXADDR
   17.	proxy_addr_type:		PXPHYSICAL.PXADDRTYPE
   18.	port:				PORT.NUMBER
   19.	proxy_auth_name:		PXAUTHINFO.PXAUTH-ID
   20.	proxy_auth_secret:		PXAUTHINFO.PXAUTH-PW
   21.	proxy_auth_type:		PXAUTHINFO.PXAUTH-TYPE
   22.	proxy_type:			PORT.SERVICE
   23.	proxy_requires_auth:		n/a
   24.	mms_url:			APPLICATION.ADDR
   25.	homepage_url:			APPLICATION.RESOUCE.STARTPAGE

   */

}
