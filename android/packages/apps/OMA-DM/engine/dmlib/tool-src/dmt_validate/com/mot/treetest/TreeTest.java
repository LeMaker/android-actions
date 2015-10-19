package com.mot.treetest;

import java.util.*;
import java.io.*;

public class TreeTest {
  private Node rootNode;
  private String dmtDir;
  private int counter = 1;
  private Util u = new Util();
  private Validator v = new Validator();

  public void createTestFile(String dmtDir, String testFile) throws Exception {
    this.dmtDir = (new File(dmtDir)).getAbsolutePath();
    createNodes();
    Content c = validateNode(rootNode);
    writeFile(testFile, "SetAutoRelease off \n" + c.tests);
    System.out.println("Test file has been created...");
  }

  public Content validateNode(Node n) throws Exception {

    if (n.isLeaf) {
      return validateLeafNode(n);
    }
    else {
      Content cin = new Content(); //content for interior node
      Content tmp = null;
      for (int i = 0; i < n.children.size(); i++) {
        tmp = validateNode( (Node) n.children.get(i));
        cin.lines += tmp.lines;
        cin.tests += tmp.tests;
      }
      return validateInteriorNode(n, cin);
    }
  }

  public Content validateLeafNode(Node n) throws Exception {
    // for the future development use line count ...
    // Content c = new Content();
    // c.lines = 2;
    // c.tests = "leaf test:  " + n.path + "   0" + "\n" + "leaf test: " + n.path + "   0" + "\n";

    Content c;
    c = v.validateLeafAccess(n);

    //This MUST  execute last before return statment !!!
    if (n.isPartOfMultinode) {
      if(n.access.add == 1){
        String suffix = u.getTypeSuffixForCommand(n);
        String testVal = u.getTestValue(n);
        // !!! For the future developmentchange to use  use line count,  ...
        String command = "createl" + suffix + " " + n.path + " " + testVal +
            "\n";
        c.tests = command + c.tests;
        c.lines++;
        return c;
      }else{
        return new Content();
      }
    }
    return c;
  }

  public Content validateInteriorNode(Node n, Content cin) throws Exception {
    //c.lines = cin.lines + 1;
    //c.tests = "node test:  " + n.path + "   " + cin.lines + "\n";
    //c.tests += cin.tests;

    Content c = v.validateInteriorAccess(n);
    c.lines = cin.lines;
    c.tests += cin.tests;
    if (n.isPartOfMultinode) {
      // !!! For the future developmentchange to use  use line count,  ...
      String command = "createi" + " " + n.path + "\n";
      c.tests = command + c.tests;
      c.lines++;
    }
    return c;
  }

  public void createNodes() throws Exception {
    File rootFile = new File(dmtDir);
    rootNode = new Node();
    rootNode.name = ".";
    rootNode.path = ".";
    rootNode.parameters = getParms(rootFile.getAbsolutePath());
    rootNode.access = Util.getAccess(rootNode);
    File[] fileChildren = rootFile.listFiles();
    for (int i = 0; i < fileChildren.length; i++) {
      createChildrenNodes(fileChildren[i], rootNode);
    }
  }

  private void createChildrenNodes(File f, Node parentNode) throws Exception {
    String tmp;
    Object nodatagen = parentNode.parameters.get("nodatagen");
    Object storesPD = parentNode.parameters.get("storesPD");

    // ignore all children handled by plugins
    if (nodatagen != null || storesPD != null) {
      return;
    }

    //return from the level parm.txt and ignore settings for Opera
    if (f.isFile() || f.getName().equals("Opera") || f.getName().equals("Con#")) {
      return;
    }

    Node node = new Node();

    tmp = f.getName();

    //tmp = tmp.replaceAll("#", "");
    tmp = u.replaceAll(tmp, "#", "");
    //tmp = tmp.replaceAll("@", "");
    tmp = u.replaceAll(tmp, "@", "");
    node.name = tmp.trim();

    node.parameters = getParms(f.getAbsolutePath());
    node.access = Util.getAccess(node);
    tmp = (String) node.parameters.get("type");
    if (!tmp.equals("node")) {
      node.isLeaf = true;
    }

    nodatagen = node.parameters.get("nodatagen");
    storesPD = node.parameters.get("storesPD");

    // ignore this child handled by plugins
    if (nodatagen != null || storesPD != null) {
      return;
    }

   // Access a = u.getAccess(parentNode);

    if (node.name.equals("[]")) {
      //ignore multinodes without access "Add"
      if (node.access.add == 0) {
        return;
      }
      node.isMultiNode = true;
      node.isPartOfMultinode = true;
      String nValues = (String) node.parameters.get("nValues");
      if (nValues == null) {
        node.name = "VH_multi" + (counter++);
      }
      else {
        StringTokenizer tok = new StringTokenizer(nValues, ",");
        while (tok.hasMoreTokens()) {
          //take the latest name
          node.name = tok.nextToken().trim();
        }
      }
    }
    else {
      node.isPartOfMultinode = parentNode.isPartOfMultinode;
      //ignore multinodes without access "Add"
      if (node.isPartOfMultinode && node.access.add == 0) {
        return;
      }
    }
    node.path = parentNode.path + "/" + node.name;

    // ignore ./SyncML/DMAcc since it should be only for DM 1.2
    if ("./SyncML/DMAcc".equals(node.path)) {
      return;
    }

    File[] fileChildren = f.listFiles();
    for (int i = 0; i < fileChildren.length; i++) {
      createChildrenNodes(fileChildren[i], node);
    }

    parentNode.children.add(node);
  }

// Utility function to parse the contents of parm.txt files in each DMT directory
  private HashMap getParms(String fileName) throws
      Exception {
    //correct path for multi nodes ( /[]/ )
    String modifiedFileName = getFileNameForOwnAttributes(fileName);

    HashMap parameters = new HashMap();

    //Get all attributes for the node
    BufferedReader in = new BufferedReader(new FileReader(modifiedFileName +
        "/parm.txt"));
    String line;

    while ( (line = in.readLine()) != null) {
      //skip empty lines and comments
      if (line.trim().length() == 0 || line.trim().startsWith("#")) {
        continue;
      }

      int nameBoundary = line.indexOf(":");
      if (nameBoundary <= 0) {
        continue;
      }

      String parmName = line.substring(0, nameBoundary);
      String parmValue = line.substring(nameBoundary + 1);

      if (parmName.equalsIgnoreCase("type")) {
        if (parmValue.equalsIgnoreCase("boolean")) {
          parmValue = "bool";
        }
        parameters.put("type", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("storesPD")) {
        parameters.put("storesPD", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("handledByPlugin")) {
        parameters.put("handledByPlugin", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("id")) {
        parameters.put("id", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("minLen")) {
        parameters.put("minLen", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("maxLen")) {
        parameters.put("maxLen", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("maxChild")) {
        parameters.put("maxChild", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("min")) {
        parameters.put("min", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("max")) {
        parameters.put("max", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("values")) {
        //parameters.put("values", parmValue.trim().replaceAll(" ", ""));
        parmValue = u.replaceAll(parmValue, " ", "");
        parameters.put("values", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("regexp")) {
        parameters.put("regexp", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("nMaxLen")) {
        parameters.put("nMaxLen", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("nValues")) {
        //parameters.put("nValues", parmValue.trim().replaceAll(" ", ""));
        parmValue = u.replaceAll(parmValue, " ", "");
        parameters.put("nValues", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("nRegexp")) {
        parameters.put("nRegexp", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("auto")) {
        parameters.put("auto", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("fk")) {
        parameters.put("fk", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("child")) {
        //parameters.put("child", parmValue.trim().replaceAll(" ", ""));
        parmValue = u.replaceAll(parmValue, " ", "");
        parameters.put("child", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("depend")) {
        parameters.put("depend", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("recur-after-segment")) {
        parameters.put("recur-after-segment", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("max-recurrence")) {
        parameters.put("max-recurrence", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("acl")) {
        parameters.put("acl", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("default")) {
        parameters.put("default", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("value")) {
        parameters.put("value", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("mime")) {
        parameters.put("mime", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("access")) {
        //parameters.put("access", parmValue.trim().replaceAll(" ", ""));
        parmValue = u.replaceAll(parmValue, " ", "");
        parameters.put("access", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("nodatagen")) {
        parameters.put("nodatagen", new Boolean(true));
      }
      else if (parmName.equalsIgnoreCase("nometagen")) {
        parameters.put("nometagen", new Boolean(true));
      }
      else if (parmName.equalsIgnoreCase("store")) {
        parameters.put("store", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("LOBProgressBAR")) {
        parameters.put("LOBProgressBAR", parmValue.trim());
      }
      else if (parmName.equalsIgnoreCase("description")) {
        //return parameters;
        break;
      }
      else {
        //ignore unsupported attributes
        continue;
      }
    }

    in.close();
//System.out.println(modifiedFileName + " modifiedFileName.length()=" +modifiedFileName.length() + "dmtDir.length()="+dmtDir.length());
    if (modifiedFileName.length() > dmtDir.length()) {
      // get attribute "Add" from the parent node for the modifiedFileName
      in = new BufferedReader(new FileReader( (new File(
          modifiedFileName)).getParentFile().getAbsolutePath() + "/parm.txt"));

      while ( (line = in.readLine()) != null) {
        //skip empty lines and comments
        if (line.trim().length() == 0 || line.trim().startsWith("#")) {
          continue;
        }

        int nameBoundary = line.indexOf(":");
        if (nameBoundary <= 0) {
          continue;
        }

        String parmName = line.substring(0, nameBoundary);
        String parmValue = line.substring(nameBoundary + 1);

        if (parmName.equalsIgnoreCase("access")) {
          //parameters.put("access", parmValue.trim().replaceAll(" ", ""));
          parmValue = u.replaceAll(parmValue, " ", "");
          parameters.put("parentAccess", parmValue.trim());
          break;
        }
      }

      in.close();
    }

    return parameters;
  }

  private void writeFile(String path, String data) throws
      Exception {
    BufferedWriter out = new BufferedWriter(new FileWriter(path));
    out.write(data);
    out.close();
  }

  public void parseResult(String resultFile, String errorFile) throws Exception {
    BufferedReader in = new BufferedReader(new FileReader(resultFile));
    String line;
    String str;
    StringBuffer tmp = new StringBuffer();
    StringBuffer err = new StringBuffer();

    while ( (line = in.readLine()) != null) {
      if (line.trim().length() == 0) {
        str = tmp.toString();
        if (str.indexOf("[31mError!") > 0) {
          err.append(str + "\n");
        }
        tmp = new StringBuffer();
      }
      else {
        tmp.append(line + "\n");
      }
    }

    str = tmp.toString();
    if (str.indexOf("[31mError!") > 0) {
      err.append(str + "\n");
    }

    if (err.length() > 0) {
      System.out.println(
          "\n!!!An error occurred!!! Please check file with all errors:\n " +
          errorFile + "\n");
      System.out.println(err.toString());
      writeFile(errorFile, err.toString());
      System.exit(1);
    }
    else {
      System.out.println("\n-Test completed successfully...");
    }
  }

  public static void usage() {
    System.out.println(
        "Usage: java com.mot.treetest.TreeTest testgen <path/Dmt> <path/test.txt>\n" +
        " or    java com.mot.treetest.TreeTest testrun <path/result.txt>");
  }

  //replace all file names to [] if required (for multi nodes)
  private String getFileNameForOwnAttributes(String fileName) throws Exception {
    if (fileName.length() == dmtDir.length()) {
      return fileName; // root node
    }
    String path = dmtDir; // new path including multi nodes ./a/[]/f/[]/#s
    String tmpPath = dmtDir; //real path for requested file ./a/b/f/c/#s
    String uniquePath = fileName.substring(dmtDir.length(), fileName.length());
    String delim = File.separator;
    String tmpFolderName;
    boolean multiNodeFound;
    StringTokenizer st = new StringTokenizer(uniquePath, delim);
    while (st.hasMoreTokens()) {
      tmpFolderName = st.nextToken();
      multiNodeFound = false;

      //System.out.println("tmpFolderName = " + tmpFolderName);
      //if(true){
       // continue;
     // }

      if (tmpFolderName.equals("[]")) {
        path = path.concat(File.separator).concat(tmpFolderName);
      }
      else {
        File currentParentDir = new File(path);
        File[] childrens = currentParentDir.listFiles();
        for (int i = 0; i < childrens.length; i++) {
          if (childrens[i].getName().equals("[]")) {
            multiNodeFound = true;
            break;
          }
        }

        // this is the real path for requested file
        tmpPath = tmpPath.concat(File.separator).concat(tmpFolderName);
        if (multiNodeFound) {
          path = path.concat(File.separator).concat("[]");
        }
        else {
          path = path.concat(File.separator).concat(tmpFolderName);
        }
      }
    }
    //System.out.println("fileName = " + fileName);
    //System.out.println("path = " + path +"\n");
    return path;
  }

  public static void main(String[] args) {
    try {
      TreeTest tt = new TreeTest();

 /*
    tt.createTestFile("path/Dmt", "path/tests.txt"); //test
     // tt.createTestFile("I:\\remove_later\\access_Local\\Dmt", "path/tests.txt"); //test
      if (true) { //test
        return; //test
      } //test
*/
      if (args.length == 0) {
        usage();
        System.exit(1);
      }

      String action = args[0];
      if ("testgen".equals(action)) {
        if (args.length != 3) {
          usage();
          System.exit(1);
        }
        tt.createTestFile(args[1], args[2]);
      }
      else if ("testrun".equals(action)) {
        if (args.length != 3) {
          usage();
          System.exit(1);
        }
        tt.parseResult(args[1], args[2]);
      }
      else {
        usage();
        System.exit(1);
      }
      System.exit(0);
    }
    catch (Exception e) {
      e.printStackTrace();
      System.exit(1);
    }
  }

}
