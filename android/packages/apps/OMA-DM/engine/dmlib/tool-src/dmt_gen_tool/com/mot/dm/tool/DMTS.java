package com.mot.dm.tool;

import java.io.*;
import java.util.*;

public class DMTS {
  private StringBuffer sb = null;
  private String topPath = null;

  public void convert(String pathFrom, String pathTo) throws Exception {
    if (! (pathFrom.toUpperCase().endsWith(".ZIP") ||
           pathFrom.toUpperCase().endsWith(".DMTS"))) {
      throw new Exception("\nFormat not supported for parameter <pathFrom>...");
    }
    else if (! (pathTo.toUpperCase().endsWith(".ZIP") ||
                pathTo.toUpperCase().endsWith(".DMTS"))) {
      throw new Exception("\nFormat not supported for parameter <fileFrom>...");
    }

    File f;
    f = new File(pathFrom);
    if (!f.exists()) {
      throw new Exception("File doesn't exists: " + pathFrom);
    }
    f = new File(pathTo);
    if (f.exists()) {
      Util.verbose("Removing file: " + pathTo + "...");
      f.delete();
    }
    if (pathFrom.toUpperCase().endsWith(".ZIP") &&
        pathTo.toUpperCase().endsWith(".DMTS")) {
      zip2dmts(pathFrom, pathTo);
    }
    else if (pathFrom.toUpperCase().endsWith(".DMTS") &&
             pathTo.toUpperCase().endsWith(".ZIP")) {
      dmts2zip(pathFrom, pathTo);
    }
    else {
      throw new Exception("Unsupported format for conversion. \nFormat should be \".zip -> .dmts\" or \".dmts -> .zip\"");
    }
    System.out.println("Done");
  }

  public void dmts2zip(String dmtsPath, String zipPath) throws Exception {
    String tmpDirPath = "tmp_s2z_" + Calendar.getInstance().getTime().getTime();
    try {
      Util.verbose("Converting dmts file to directory Dmt ...");
      dmts2dir(dmtsPath, tmpDirPath);

      Util.verbose("Zipping directory Dmt ...");
      Zip zip = new Zip();
      zip.zip(tmpDirPath + "/Dmt", zipPath);
    }
    finally {
      File f = new File(tmpDirPath);
      if (f.exists()) {
        Util.deleteDir(f);
      }
    }
  }

  public void zip2dmts(String zipPath, String dmtsPath) throws Exception {
    String tmpDirPath = "tmp_z2s_" + Calendar.getInstance().getTime().getTime();
    try {
      Util.verbose("Unzipping file to directory Dmt ...");
      Zip zip = new Zip();
      zip.unzip(zipPath, tmpDirPath);

      Util.verbose("Converting directory Dmt to dmts file...");
      dir2dmts(tmpDirPath + "/Dmt", dmtsPath);
    }
    finally {
      File f = new File(tmpDirPath);
      if (f.exists()) {
        Util.deleteDir(f);
      }
    }
  }

  public void dmts2dir(String dmtsPath, String outputDir) throws Exception {

    File dmts = new File(dmtsPath);
    if (!dmts.exists()) {
      throw new Exception("DMTS file doesn't exist: " + dmtsPath);
    }
    else if (dmts.isDirectory()) {
      throw new Exception("Dmts file cannot be directory : " + dmtsPath);
    }

    if (outputDir.length() == 0) {
      outputDir = ".";
    }

    File output = new File(outputDir);

    if (!output.exists() && !output.mkdirs()) {
      throw new Exception("Cannot create directory : " + topPath);
    }

    topPath = output.getPath() + "/Dmt";
    FileReader fr = new FileReader(dmts);
    BufferedReader br = new BufferedReader(fr);
    String line;
    String currPath = null;
    String tmp;
    StringBuffer sbParm = new StringBuffer();
    try {
      while ( (line = br.readLine()) != null) {
        if (line.startsWith("[")) {
          line = line.trim();
          tmp = line.substring(1, line.length() - 1);
          tmp = (tmp.equals(".")) ? "" : Util.replaceStr(tmp, "*", "[]");
          // if (currPath != null && sbParm.length() > 0) {
          if (currPath != null) {
            writeParm(currPath, sbParm.toString());
          }
          currPath = topPath + tmp;
          sbParm = new StringBuffer();
        }
        else {
          sbParm.append(line + "\n");
        }
      }
    }
    finally {
      br.close();
      fr.close();
    }
    //if (currPath != null && sbParm.length() > 0) {
    if (currPath != null) {
      writeParm(currPath, sbParm.toString());
    }
  }

  public void dir2dmtsDir(String dmtDir, String outputDir) throws Exception {

    if(outputDir.toUpperCase().endsWith(".DMTS")){
      dir2dmts(dmtDir, outputDir);
    }
    else{

      if (outputDir.length() > 0) {
        outputDir += (outputDir.endsWith("/")) ? "" : "/";
      }

      File f = new File(dmtDir);
      String dmtsName = f.getName() + ".dmts";

      File dmts = new File(outputDir + dmtsName);
      if (dmts.exists() && !dmts.canWrite()) {
        throw new Exception("DMTS file in read only mode: " + outputDir +
                            dmtsName);
      }
      dir2dmts(dmtDir, outputDir + dmtsName);
    }
  }

  public void dir2dmts(String dmtDir, String dmtsPath) throws Exception {
    sb = new StringBuffer();
    topPath = "";

    File top = new File(dmtDir);
    if (!top.exists()) {
      throw new Exception("Dmt dir doesn't exist: " + dmtDir);
    }
    else if (!top.isDirectory()) {
      throw new Exception("Dmt dir not directory: " + dmtDir);
    }

    File dmts = new File(dmtsPath);
    if (dmts.exists() && !dmts.canWrite()) {
      throw new Exception("DMTS file in read only mode: " + dmtsPath);
    }

    File dmtsParent = dmts.getParentFile();
    if (dmtsParent != null && !dmtsParent.exists()) {
      dmtsParent.mkdirs();
    }

    sb.append("[.]\n");
    topPath = top.getAbsolutePath();

    File[] files = top.listFiles();
    File parm = getParmFile(files);
    // if (parm == null) {
    //   throw new Exception("The parm.txt doesn't exists: " + top.getAbsolutePath());
    // }
    if (parm != null) {
      readParm(parm);
    }

    for (int i = 0; i < files.length; i++) {
      if (files[i].isDirectory()) {
        readDir(files[i]);
      }
    }
    Util.writeFile(dmts.getAbsolutePath(), sb.toString());
  }

  private void readDir(File f) throws Exception {
    String path = Util.replaceStr(f.getAbsolutePath(), this.topPath, "");
    path = Util.replaceStr(path, "\\", "/");
    path = Util.replaceStr(path, "[]", "*");
    sb.append("[" + path + "]\n");

    File[] files = f.listFiles();
    File parm = getParmFile(files);
    //  if (parm == null) {
    //    throw new Exception("The parm.txt doesn't exists: " + f.getAbsolutePath());
    //  }
    if (parm != null) {
      readParm(parm);
    }

    for (int i = 0; i < files.length; i++) {
      if (files[i].isDirectory()) {
        readDir(files[i]);
      }
    }
  }

  private void readParm(File f) throws Exception {
    FileInputStream in = new FileInputStream(f);
    byte[] b = new byte[in.available()];
    in.read(b);
    String s = new String(b);
    /*  while (s.endsWith("\n")) {
        s = s.substring(0, s.length() - 1);
      }
      sb.append(s + "\n\n");
     */
    // add additional
    sb.append(s + "\n");
    in.close();
  }

  private void writeParm(String dirPath, String text) throws Exception {
    File dir = new File(dirPath);
    if (!dir.exists()) {
      if (!dir.mkdirs()) {
        throw new Exception("Cannot create directorie(s) " + dirPath);
      }
    }
    if (text.length() > 0) {
      if (text.endsWith("\n")) {
        text = text.substring(0, text.length() - 1);
      }
      Util.writeFile(dirPath + "/parm.txt", text);
    }
  }

  private File getParmFile(File[] files) {
    for (int i = 0; i < files.length; i++) {
      if (files[i].isFile() && files[i].getName().equals("parm.txt")) {
        return files[i];
      }
    }
    return null;
  }

  public static void main(String[] args) {
    try {
      if (args.length != 3) {
        System.out.println("Wrong arguments");
        DMTSTool.usage(true);
        System.exit( -1);
      }

      Util.VERBOSE = true;
      DMTS dmts = new DMTS();
      if (args[0].equals("-sd")) {
        dmts.dmts2dir(args[1], args[2]);
      }
      else if (args[0].equals("-ds")) {
        dmts.dir2dmtsDir(args[1], args[2]);
      }
      else {
        System.out.println("Wrong arguments");
        DMTSTool.usage(true);
        System.exit( -1);
      }
    }
    catch (Exception e) {
      e.printStackTrace();
    }
  }

}
