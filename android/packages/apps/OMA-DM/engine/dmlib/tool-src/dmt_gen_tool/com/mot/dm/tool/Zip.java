package com.mot.dm.tool;

import java.io.*;
import java.util.zip.*;
import java.util.Enumeration;

public class Zip {
  private final int BUFFER = 1024;
  private String topPath;

  public void zip(String inputDir, String outputFile) throws Exception {
    File input = new File(inputDir);

    if (!input.exists()) {
      throw new Exception("Input directory doesn't exist: " + inputDir);
    }
    else if (!input.isDirectory()) {
      throw new Exception("Input file doesn't directory: " + inputDir);
    }

    File output = new File(outputFile);
    if (!output.getName().toUpperCase().endsWith(".ZIP")) {
      throw new Exception("Output file is not zip: " + outputFile);
    }
    if (!output.exists()) {
      File parentDir = output.getParentFile();
      if (parentDir != null && !parentDir.exists()) {
        parentDir.mkdirs();
      }
    }
    FileOutputStream dest = new FileOutputStream(output);
    ZipOutputStream out = new ZipOutputStream(new BufferedOutputStream(dest));

    topPath = input.getParentFile().getAbsolutePath() + File.separator;
    addFileToZip(input, out);
    parseDirectory(input, out);

    /*
        File files[] = input.listFiles();
        for (int i = 0; i < files.length; i++) {
          if (files[i].isDirectory()) {
            addFileToZip(files[i], out);
            parseDirectory(files[i], out);
          }
          else {
            addFileToZip(files[i], out);
          }
        }
          }
     */
    out.close();
  }

  public void unzip(String zipFilePath, String outputDir) throws Exception {
    BufferedOutputStream dest = null;
    BufferedInputStream is = null;
    ZipEntry entry;

    ZipFile zipfile = new ZipFile(zipFilePath);
    Enumeration e = zipfile.entries();

    while (e.hasMoreElements()) {
      try {
        entry = (ZipEntry) e.nextElement();

        Util.verbose("Extracting: " + entry);

        is = new BufferedInputStream(zipfile.getInputStream(entry));
        int count;
        byte data[] = new byte[BUFFER];
        outputDir +=
            (outputDir.length() > 0 && !outputDir.endsWith(File.separator)) ?
            "/" : "";

        File newFile = new File(outputDir + entry);
        if (entry.isDirectory()) {
          if (!newFile.exists()) {
            newFile.mkdirs();
          }
        }
        else {
          File parent = newFile.getParentFile();
          if (parent != null && !parent.exists()) {
            parent.mkdirs();
          }
        }

        FileOutputStream fos = new FileOutputStream(newFile.getAbsolutePath());
        dest = new BufferedOutputStream(fos, BUFFER);
        while ( (count = is.read(data, 0, BUFFER)) != -1) {
          dest.write(data, 0, count);
        }
        dest.flush();
        dest.close();
        is.close();
      }
      catch (Exception ee) {
//ee.printStackTrace();
      }
    }
  }

  private void parseDirectory(File dir, ZipOutputStream out) throws Exception {
    File files[] = dir.listFiles();
    for (int i = 0; i < files.length; i++) {
      if (files[i].isDirectory()) {
        addFileToZip(files[i], out);
        parseDirectory(files[i], out);
      }
      else {
        addFileToZip(files[i], out);
      }
    }
  }

  private void addFileToZip(File file, ZipOutputStream out) throws Exception {

    BufferedInputStream origin = null;
    byte data[] = new byte[BUFFER];

    Util.verbose("Adding: " + file);

    String zipEntryName = file.getAbsolutePath();
    zipEntryName = Util.replaceStr(zipEntryName, topPath, "");
    zipEntryName = Util.replaceStr(zipEntryName, "\\", "/");
    if (file.isDirectory() && !zipEntryName.endsWith("/")) {
      zipEntryName += "/";
    }
    ZipEntry entry = new ZipEntry(zipEntryName);
    out.putNextEntry(entry);
    if (file.isFile()) {
      FileInputStream fi = new FileInputStream(file);
      origin = new BufferedInputStream(fi, BUFFER);
      int count;
      while ( (count = origin.read(data, 0, BUFFER)) != -1) {
        out.write(data, 0, count);
      }
      origin.close();
    }
  }

  public static void main(String[] args) {
    try {
     if (args.length != 3) {
       System.out.println("Wrong arguments");
       DMTSTool.usage(true);
       System.exit( -1);
     }

     Util.VERBOSE = true;
     Zip zip  = new Zip();
     if(args[0].equals("-zip")){
       zip.zip(args[1], args[2]);
     }
     else if(args[0].equals("-unzip")){
        zip.unzip(args[1], args[2]);
     }
     else{
       System.out.println("Wrong arguments");
       DMTSTool.usage(true);
       System.exit(-1);
     }
   }
   catch (Exception e) {
     e.printStackTrace();
   }

    //z.zip("I:\\tmp\\Dmt", "R_NewZIP.zip");
    // z.unzip("NewZIP.zip", "");
    // z.unzip("D:/Profiles/E32569/jbproject/Dmt.zip", "");
    // z.unzip("I:/tmp/Dmt.zip", "");
    // z.unzip("D:/Profiles/E32569/jbproject/Dmt.zip", "D:\\Profiles\\E32569\\jbproject\\XML2WBXMLPrj\\tmp_1129935212004\\2");
  }
}
