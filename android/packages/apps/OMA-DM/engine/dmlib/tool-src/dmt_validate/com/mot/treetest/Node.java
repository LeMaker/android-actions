package com.mot.treetest;

import java.util.*;

public class Node {
  public String path = "not_defined_xxx";
  public String name = "not_defined_xxx";
  HashMap parameters = new HashMap();
  public boolean isLeaf = false;
  public boolean isMultiNode = false;
  public boolean isPartOfMultinode = false;
  String value = "NULL";
  public Access access = new Access();
  ArrayList children = new ArrayList();
}
