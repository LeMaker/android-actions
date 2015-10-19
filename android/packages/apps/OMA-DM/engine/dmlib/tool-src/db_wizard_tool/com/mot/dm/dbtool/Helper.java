package com.mot.dm.dbtool;

import java.util.*;

public class Helper {
  // not used all class!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  public static String getPXAddrType(String addr) throws Exception {
    if (isAddrTypeIPV4(addr)) {
      return "IPV4";
    }
    else if (isAddrTypeIPV6(addr)) {
      return "IPV6";
    }
    else if (isAddrTypeE164(addr)) {
      return "E164";
    }
    return null;

  }

  private static boolean isAddrTypeIPV4(String addr) {
    //123.211.1.3
    String tmp;
    StringTokenizer st = new StringTokenizer(addr, ".");
    if (st.countTokens() != 4) {
      return false;
    }
    try {
      while (st.hasMoreTokens()) {
        tmp = st.nextToken();
        Integer.parseInt(tmp);
      }
      return true;
    }
    catch (Exception ex) {
      return false;
    }
  }

  private static boolean isAddrTypeIPV6(String addr) {
    //??????

    return false;

  }

  private static boolean isAddrTypeE164(String addr) {
    // 9870981 or +30945984732
    return false;
  }

}
