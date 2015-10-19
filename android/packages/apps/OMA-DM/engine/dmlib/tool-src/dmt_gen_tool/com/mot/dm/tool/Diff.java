package com.mot.dm.tool;

import java.io.*;
import java.util.*;

public class Diff {
  public static boolean VERBOSE = false;
  private final String NULL = "NULL";
  private File tmpDir = null;
  private String path1, path2;

  private ArrayList presentInFirstOnly = new ArrayList();
  private ArrayList presentInSecondOnly = new ArrayList();
  private HashMap differentValues = new HashMap();

  public void getDiff(String path1, String path2) throws Exception {
    this.path1 = path1;
    this.path2 = path2;
    tmpDir = new File("tmp_" + Calendar.getInstance().getTime().getTime());
    try {
      tmpDir.mkdirs();

      File f;
      f = new File(path1);
      if (!f.exists()) {
        throw new Exception("File doesn't exists: " + path1);
      }
      f = new File(path2);
      if (!f.exists()) {
        throw new Exception("File doesn't exists: " + path2);
      }

      String dmtsPath1 = prepareFile(path1, 1);
      String dmtsPath2 = prepareFile(path2, 2);

      diff(dmtsPath1, dmtsPath2);
    }
    finally {
      if (tmpDir.exists()) {
        Util.deleteDir(tmpDir);
      }
    }

  }

  private String prepareFile(String path, int num) throws Exception {
    String tmpPath = tmpDir.getAbsolutePath() + "/" + num;
    File f = new File(tmpPath);
    if (!f.exists()) {
      f.mkdirs();
    }
    f = new File(path);
    if (f.isDirectory()) {
      String dmtsName = f.getName() + ".dmts";
      DMTS dmts = new DMTS();
      dmts.dir2dmtsDir(path, tmpPath);
      return tmpPath + "/" + dmtsName;
    }
    else if (path.toUpperCase().endsWith(".DMTS")) {
      return path;
    }
    else if (path.toUpperCase().endsWith(".ZIP")) {
      Zip z = new Zip();
      z.unzip(path, tmpPath);
      DMTS dmts = new DMTS();
      dmts.dir2dmtsDir(tmpPath + "/Dmt", tmpPath);
      return tmpPath + "/Dmt.dmts";
    }
    else {
      throw new Exception("Diff doesn't support file format: " + path);
    }
  }

  private void diff(String dmtsPath1, String dmtsPath2) throws Exception {
    HashMap map1 = dmtsToMap(dmtsPath1);
    HashMap map2 = dmtsToMap(dmtsPath2);

    HashMap parms1, parms2;

    Object[] keys = map1.keySet().toArray();

    for (int i = 0; i < keys.length; i++) {
      String nodePath = (String) keys[i];

      if (!map2.containsKey(nodePath)) {
        presentInFirstOnly.add(nodePath);
        continue;
      }
      parms1 = (HashMap) map1.get(nodePath);
      parms2 = (HashMap) map2.get(nodePath);

      ArrayList arr = compareParms(parms1, parms2);
      if (arr.size() > 0) {
        differentValues.put(nodePath, arr);
      }
      map2.remove(nodePath);
    }

    keys = map2.keySet().toArray();
    for (int i = 0; i < keys.length; i++) {
      presentInSecondOnly.add(keys[i]);
    }
    displayDiff();
  }

  private HashMap dmtsToMap(String dmtsPath) throws Exception {
    HashMap map = new HashMap();
    HashMap hashParms = new HashMap();
    FileReader fr = new FileReader(dmtsPath);
    BufferedReader br = new BufferedReader(fr);
    String line;
    String currPath = null;
    String err;
    try
    {
      while ( (line = br.readLine()) != null) {
        if (line.startsWith("#")) {
          continue;
        }
        if (line.startsWith("[")) {
          if (currPath != null && !hashParms.isEmpty()) {
            map.put(currPath, hashParms);
          }
          currPath = line;
          hashParms = new HashMap();
        }
        else {
          if (line.trim().length() > 0) {
            err = addParmToHash(hashParms, line);
            if (err != null) {
              throw new Exception(err + "\n Path: :" + currPath);
            }
          }
        }
      }
    }finally{
      br.close();
      fr.close();
    }
    return map;
  }

  private ArrayList compareParms(HashMap parms1, HashMap parms2) throws
      Exception {
    ArrayList arr = new ArrayList();
    String v1, v2;
    Object[] keys = parms1.keySet().toArray();
    for (int i = 0; i < keys.length; i++) {
      String name = (String) keys[i];
      if (!parms2.containsKey(name)) {
        Parm p = new Parm();
        p.name = name;
        p.value1 = (String) parms1.get(name);
        arr.add(p);
        continue;
      }
      v1 = (String) parms1.get(name);
      v2 = (String) parms2.get(name);
      if (v1.equals(v2)) {
        parms2.remove(name);
      }
      else {
        Parm p = new Parm();
        p.name = name;
        p.value1 = (String) parms1.get(name);
        p.value2 = (String) parms2.get(name);
        arr.add(p);
        parms2.remove(name);
      }
    }
    // check rest of second parameters
    if (!parms2.isEmpty()) {
      keys = parms2.keySet().toArray();
      for (int i = 0; i < keys.length; i++) {
        String name = (String) keys[i];
        Parm p = new Parm();
        p.name = name;
        p.value2 = (String) parms2.get(name);
        arr.add(p);
      }
    }

    return arr;
  }

  private String addParmToHash(HashMap hashParms, String line) throws Exception {
    String name = "";
    String value = "";
    int i = line.indexOf(":");
    if (i <= 0) {
      name = line;
      value = NULL;
      //return "Error: attribute doesn't contain \":\".";
    }else{
      name = line.substring(0, i);
      value = line.substring(i + 1, line.length());
    }
/*  if (name.length() == 0) {
      return "Error: attribute name in a parm.txt cannot have 0 length.";
    }
   else if (value.length() == 0) {
      return "Error: value cannot have 0 length.";
    }
*/
    hashParms.put(name, value);
    return null;
  }

  private void displayDiff() {
    boolean diffFound = false;
    System.out.println("\n*********************************** Diff Result ***********************************\n");
    if (presentInFirstOnly.size() > 0) {
      diffFound = true;
      System.out.println("=================== Nodes are presenting in the FIST file only ===================");
      System.out.println("== ( " + path1 + " ) ==");
      for (int i = 0; i < presentInFirstOnly.size(); i++) {
        System.out.println("   " + (String) presentInFirstOnly.get(i));
      }
      System.out.println("====================================== END ========================================\n");
    }

    if (presentInSecondOnly.size() > 0) {
      diffFound = true;
      System.out.println("=================== Nodes are presenting in the SECOND file only ===================");
      System.out.println("== ( " + path2 + " ) ==");
      for (int i = 0; i < presentInSecondOnly.size(); i++) {
        System.out.println("   " + (String) presentInSecondOnly.get(i));
      }
      System.out.println("====================================== END ========================================\n");
    }

    if (!differentValues.isEmpty()) {
      diffFound = true;
      String diffPath;
      System.out.println("============ The following nodes has different values for attribute(s) ============");
      System.out.println("== f1 : " + path1);
      System.out.println("== f2 : " + path2);
      System.out.println("===================================================================================");

      Object[] keys = differentValues.keySet().toArray();
      for (int i = 0; i < keys.length; i++) {
        StringBuffer sb1 = new StringBuffer();
        StringBuffer sb2 = new StringBuffer();
        Parm p;
        diffPath = (String) keys[i];
        System.out.println("\n--> " + diffPath);
        ArrayList arr = (ArrayList) differentValues.get(diffPath);
        for (int j = 0; j < arr.size(); j++) {
          p = (Parm) arr.get(j);
          sb1.append("    f1 -> " + p.name + ":" + p.value1 + "\n");
          sb2.append("    f2 -> " + p.name + ":" + p.value2 + "\n");
        }
        System.out.print(sb1.toString());
        System.out.println("--------------------");
        System.out.print(sb2.toString());

      }
      System.out.println("====================================== END ========================================\n");
    }
    if (!diffFound) {
      System.out.println("============================= The files are identical =============================\n");
      System.out.println("====================================== END ========================================\n");
    }
  }

  public static void main(String[] args) {
    try {
      if (args.length != 2) {
        System.out.println("Wrong arguments");
        DMTSTool.usage(true);
        System.exit( -1);
      }
      Util.VERBOSE = true;
      Diff d = new Diff();
      d.getDiff(args[0], args[1]);
      //d.getDiff("Dmt.dmts", "Dmt.dmts");
      //d.getDiff("I:/tmp/Dmt", "I:/tmp/Dmt.zip");
    }
    catch (Exception e) {
      e.printStackTrace();
    }
  }

  class Parm {
    public String name = "";
    public String value1 = "";
    public String value2 = "";

  }

}
