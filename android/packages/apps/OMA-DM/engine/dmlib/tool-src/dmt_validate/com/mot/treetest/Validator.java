package com.mot.treetest;

public class Validator {
  Util u = new Util();

  public Content validateLeafAccess(Node n) throws Exception {
    //For the future development: command + path + value + lineCount + retCode
    String command = "";
    int count = 0;
    String suffix = u.getTypeSuffixForCommand(n);
    String testVal = u.getTestValue(n);
    Access a = u.getAccess(n);
    // Get, Replace, Delete, Add
    // start from the bottom "Add". check that we can delete it before
    if (a.add == 1 && a.delete == 1) {
      //command = "createl" + suffix + " " + n.path + " " + testVal + count + " 0\n";
      command = "createl" + suffix + " " + n.path + " " + testVal + "\n";
      //count++;
    }
    // Next from the bottom "Delete"
    if (a.delete == 1) {
      command = "delete " + n.path + "\n" + command;
      //count++;
    }
    // Next from the bottom "Replace"
    if (a.replace == 1) {
      //command = "set" + suffix + " " + n.path + " " + testVal + count + " 0\n";
      command = "set" + suffix + " " + n.path + " " + testVal + "\n" + command;
      //count++;
    }
    // Next from the bottom "Get"
    if (a.get == 1) {
      command = "get " + n.path + "\n" + command;
      //count++;
    }

    Content c = new Content();
    c.tests = command;
    c.lines = count;

    return c;
  }

  public Content validateInteriorAccess(Node n) {
    //For the future development: command + path + value + lineCount + retCode
    String command = "";
    int count = 0;

    command = "get " + n.path + "\n";
    count++;

    Content c = new Content();
    c.tests = command;
    c.lines = count;

    return c;
  }

}
