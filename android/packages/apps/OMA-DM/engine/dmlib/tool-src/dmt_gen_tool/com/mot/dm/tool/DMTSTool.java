package com.mot.dm.tool;

import java.io.File;

public class DMTSTool {
  private static String fileFrom = null;
  private static String fileTo = null;
  private static boolean conversion = false;
  private static boolean difference = false;

  public static void usage(boolean isForDev) {
    System.out.println("");
    System.out.println(
        "Usage:  java com.mot.dm.tool.DMTSTool  [-options]  <fileFrom> <fileTo>");
    System.out.println("where options include:");
    System.out.println(
        "    -conversion     perform conversion process for given files ");
    System.out.println("                    file formats: zip or dmts");
    System.out.println(
        "    -difference     find difference between given files ");
    System.out.println("                    file formats: zip or dmts");
    System.out.println("    -?|-h|--help    print this help message");
    System.out.println("    -verbose        enable verbose output");
    System.out.println("    <fileFrom>      path to source file (from)");
    System.out.println("    <fileTo>        path to source file (to)");
    System.out.println( "");

    if(isForDev){
      System.out.println("Usage for developers only !!!");
      System.out.println("\njava com.mot.dm.tool.Diff <fileFrom> <fileTo>");
      System.out.println("                   file formats: dir, zip, dmts");
      System.out.println("\njava com.mot.dm.tool.DMTS -ds|-sd <fileFrom> <fileTo>");
      System.out.println("                   file formats: dir, dmts");
      System.out.println("                   options:  -ds    dir to dmts");
      System.out.println("                             -sd    dmts to dir");
      System.out.println("\njava com.mot.dm.tool.Zip   -zip|-unzip <fileFrom> <fileTo>");
      System.out.println("                   -zip   : dir  to .zip");
      System.out.println("                   -unzip : .zip to  dir");
      System.out.println( "");
    }
  }

  public static void main(String[] args) {
    String arg;
    for (int i = 0; i < args.length; i++) {
      arg = args[i];
      if (arg.equals("-?") || arg.equals("-h") || arg.equals("--help")) {
        usage(false);
        System.exit( -1);
      }
      else if (arg.equals("--helpdev")) {
        usage(true);
        System.exit( -1);
      }
      else if (arg.equals("-verbose")) {
        Util.VERBOSE = true;
      }
      else if (arg.equals("-conversion")) {
        conversion = true;
      }
      else if (arg.equals("-difference")) {
        difference = true;
      }
      else {
        if (fileFrom == null) {
          fileFrom = arg;
        }
        else if (fileTo == null) {
          fileTo = arg;
        }
        else {
          System.out.println("\nWrong parameters...");
          usage(false);
          System.exit( -1);
        }
      }
    }
    if (!validateParms()) {
      usage(false);
      System.exit( -1);
    }

    try {
      if (difference) {
        Diff diff = new Diff();
        diff.getDiff(fileFrom, fileTo);
      }
      else if (conversion) {
        boolean fileFromIsFile = fileFrom.toUpperCase().endsWith(".ZIP") || fileFrom.toUpperCase().endsWith(".DMTS");
        boolean fileToIsFile = fileTo.toUpperCase().endsWith(".ZIP") || fileTo.toUpperCase().endsWith(".DMTS");

        DMTS dmts = new DMTS();
        if(fileFromIsFile && fileToIsFile){
          dmts.convert(fileFrom, fileTo);
        }
        else if(fileFromIsFile){
          dmts.dmts2dir(fileFrom, fileTo);
        }
        else  if(fileToIsFile){
          dmts.dir2dmts(fileFrom, fileTo);
        }
        else{
          System.out.println("\nConversion Error!!! Unsupported file format(s) or file combinations");
          System.exit(-1);
        }
      }
    }
    catch (Exception e) {
      e.printStackTrace();
      System.exit(-1);
    }
    System.exit(1);
  }

  private static boolean validateParms() {

    if (!conversion && !difference ) {
      System.out.println(
          "\nRequired parameter (-conversion or -difference) is missing...");
      return false;
    }
    else if (conversion && difference) {
      System.out.println(
          "\nParameters -conversion and -difference cannot be used together...");
      return false;
    }
    if (fileFrom == null) {
      System.out.println("\nRequired parameter <fileFrom> not set...");
      return false;
    }
    if (fileTo == null) {
      System.out.println("\nRequired parameter <fileTo> not set...");
      return false;
    }
    return true;
  }
}
