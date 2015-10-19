package com.mot.treetest;

import java.util.*;

public class Util {

  public String getCreateCommand(String nodeType) {
    if (nodeType.equals("node")) {
      return "createi";
    }
    else if (nodeType.equals("chr")) {
      return "createl";
    }
    else if (nodeType.equals("bin")) {
      return "createlb";
    }
    else if (nodeType.equals("bool")) {
      return "createlz";
    }
    else if (nodeType.equals("int")) {
      return "createli";
    }
    else if (nodeType.equals("date")) {
      return "createld";
    }
    else if (nodeType.equals("time")) {
      return "createlt";
    }
    else if (nodeType.equals("float")) {
      return "createlf";
    }
    else {
      return null;
    }
  }

  public String getSetCommand(String nodeType) {
    if (nodeType.equals("chr")) {
      return "set";
    }
    else if (nodeType.equals("bin")) {
      return "setb";
    }
    else if (nodeType.equals("bool")) {
      return "setz";
    }
    else if (nodeType.equals("int")) {
      return "seti";
    }
    else if (nodeType.equals("date")) {
      return "setd";
    }
    else if (nodeType.equals("time")) {
      return "sett";
    }
    else if (nodeType.equals("float")) {
      return "setf";
    }
    else {
      return null;
    }
  }

  public static Access getAccess(Node n) {
    Access a = new Access();
    String parmValue = (String) n.parameters.get("access");
    if (parmValue != null) {
      StringTokenizer tok = new StringTokenizer(parmValue, ",");
      String accessType;
      while (tok.hasMoreTokens()) {
        accessType = tok.nextToken().trim();
        if ("Get".equals(accessType)) {
          a.get = 1;
        }
        else if ("Add".equals(accessType)) {
          a.add = 1;
        }
        else if ("Replace".equals(accessType)) {
          a.replace = 1;
        }
        else if ("Delete".equals(accessType)) {
          a.delete = 1;
        }
      }
    }

    // get access "Add" from parent
    parmValue = (String) n.parameters.get("parentAccess");
    if (parmValue != null) {
      a.add = 0;
      StringTokenizer tok = new StringTokenizer(parmValue, ",");
      String accessType;
      while (tok.hasMoreTokens()) {
        accessType = tok.nextToken().trim();
        if ("Add".equals(accessType)) {
          a.add = 1;
        }
      }
    }
    return a;
  }

  public String getTypeSuffixForCommand(Node n) {
    String type = (String) n.parameters.get("type");
    if ("int".equals(type)) {
      return "i";
    }
    else if ("bool".equals(type)) {
      return "z";
    }
    else if ("bin".equals(type)) {
      return "b";
    }
    else if ("date".equals(type)) {
      return "d";
    }
    else if ("time".equals(type)) {
      return "t";
    }
    else if ("float".equals(type)) {
      return "f";
    }
    else {
      return "";
    }
  }

  public String getTestValue(Node n) throws Exception {
    String values = (String) n.parameters.get("values");
    //take first from the predefined values
    if (values != null) {
      StringTokenizer tok = new StringTokenizer(values, ",");
      if (tok.hasMoreTokens()) {
        return tok.nextToken().trim();
      }
    }

    String type = (String) n.parameters.get("type");
    if ("int".equals(type)) {
      String min = (String) n.parameters.get("min");
      if (min == null) {
        return "1";
      }
      else {
        return (Integer.parseInt(min) + 1) + "";
      }

    }
    else if ("bool".equals(type)) {
      return "true";
    }
    else if ("bin".equals(type)) {
      return "1A";
    }
    else if ("date".equals(type)) {
      return "1962-02-06";
    }
    else if ("time".equals(type)) {
      return "07:40";
    }
    else if ("float".equals(type)) {
      return "1.1";
    }
    else {
      return "VH";
    }

  }

// use this function instead .replaceAll() to support java 1.3.x.x
  public String replaceAll(String line, String oldStr, String newStr) {
    int i = 0;

    if ( (i = line.indexOf(oldStr, i)) >= 0) {
      char[] line2 = line.toCharArray();
      StringBuffer buf = new StringBuffer(line2.length);
      buf.append(line2, 0, i).append(newStr);

      i++;
      int j = i;

      while ( (i = line.indexOf(oldStr, i)) > 0) {
        buf.append(line2, j, i - j).append(newStr);
        i++;
        j = i;
      }
      buf.append(line2, j, line2.length - j);
      return buf.toString();
    }
    return line;
  }

}
