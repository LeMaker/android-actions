/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//==================================================================================================
//
// Module Name: Gen
//
// General Description: The classes contained here provide a depth first recursive traversal of the
//                       Dmt.zip directory heirarchy, and generate the necessary data and meta data
//                       files that make up the data store for the DMT on the device.
//
//==================================================================================================

package com.mot.dm.core;

//Necessary Java language imports
import java.io.File;
import java.io.Reader;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.PrintWriter;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.StringWriter;
import java.io.DataOutputStream;

import java.util.HashMap;
import java.util.ArrayList;
import java.util.StringTokenizer;
import java.util.Iterator;
import java.util.Map;
import java.util.List;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;

import com.mot.dm.io.Node;
import com.mot.dm.io.NodeLoader;
import com.mot.dm.tool.Util;

//Encapsulate multiple data files
class Archives {
  HashMap archivesMap; //path, printWriter
  public Archives(String fstabname) throws Exception {
    archivesMap = new HashMap();
    BufferedReader reader = new BufferedReader(Util.openUtf8FileReader(fstabname));

    String line = null;
    while ( (line = reader.readLine()) != null) {
      if (line.length() == 0 || line.startsWith("#")) {
        continue;
      }
      StringTokenizer tokener = new StringTokenizer(line);
      String path = tokener.nextToken();
      String filename = tokener.nextToken();
      int index = filename.lastIndexOf(".wbxml");
      filename = filename.substring(0, index) + ".xml";
      PrintWriter fwriter = new PrintWriter(Util.openUtf8FileWriter(filename));
      archivesMap.put(path, fwriter);
    }
    reader.close();
    reader = null;
  }

  //print to all nodes
  public void printAll(String s) throws IOException {
    Iterator it = archivesMap.values().iterator();
    while (it.hasNext()) {
      PrintWriter writer = (PrintWriter) it.next();
      writer.print(s);
    }
  }

  public void print(String path, String s) throws IOException {
    Iterator it = archivesMap.entrySet().iterator();
    String myfilerootpath = null;
    PrintWriter myfilewriter = null;
    int longestmatch = 0;
    while (it.hasNext()) {
      Map.Entry entry = (Map.Entry) it.next();
      String rootpath = (String) entry.getKey();
      PrintWriter writer = (PrintWriter) entry.getValue();

      //See which directory contains the path
      int len = rootpath.length();
      if (path.indexOf(rootpath) == 0 && longestmatch < len) {
        longestmatch = len;
        myfilerootpath = rootpath;
        myfilewriter = writer;
      }
    }

    if (myfilerootpath != null) {
      myfilewriter.print(s);
    }
  }

  public void close() throws IOException {
    Iterator it = archivesMap.values().iterator();
    while (it.hasNext()) {
      PrintWriter writer = (PrintWriter) it.next();
      writer.close();
    }
    archivesMap.clear();
  }
}

class DocumentationIndexObject {
  public String nodeName;
  public String nodeType;
  public String nodeDesc;
  public String linkToNode;

  public DocumentationIndexObject() {
  }
}

class Documentation {
  final static private String dmtdoc = "index.html";
  final static private String dmttoc = "toc.html";
  final static private String dmtdesc = "desc.html";
  final static private String dmtindex = "index-all.html";
  private PrintWriter docWriter = null;
  private PrintWriter tocWriter = null;
  private PrintWriter descWriter = null;
  private PrintWriter indexWriter = null;
  private SortedMap indexMap = null;
  private int indexDupCtr = 0;
  private StringBuffer indexHdrFtr = null;
  private StringBuffer descHdrFtr = null;

  public Documentation() throws Exception {
    // Setup our index and desc HdrFtr buffers
    indexHdrFtr = new StringBuffer();
    descHdrFtr = new StringBuffer();

    // Setup sortedMap (TreeMap) to store our index objects
    indexMap = new TreeMap();

    // main page
    generateMainPage();

    tocWriter = new PrintWriter(Util.openUtf8FileWriter(dmttoc));
    descWriter = new PrintWriter(Util.openUtf8FileWriter(dmtdesc));
    indexWriter = new PrintWriter(Util.openUtf8FileWriter(dmtindex));

    // initial toc page
    generateTocHeader();

    // initial description page
    generateDescHeader();
  }

  private void generateHeader(PrintWriter writer, String title) throws
      IOException {
    writer.println("<HTML>");
    writer.println("<HEAD>");
    writer.println("<TITLE>");
    writer.println(title);
    writer.println("</TITLE>");
    writer.println("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">");
    writer.println("</HEAD>");
  }

  private void generateTrailer(PrintWriter writer) throws IOException {
    writer.println("</HTML>");
  }

  private void generateMainPage() throws IOException {
      docWriter = new PrintWriter(Util.openUtf8FileWriter(dmtdoc));
    generateHeader(docWriter, "Device Management Tree Documentation");

    docWriter.println("<FRAMESET cols=\"40%,60%\">");
    docWriter.println("<FRAME src=\"toc.html\" name=\"tocFrame\">");
    docWriter.println("<FRAME src=\"desc.html\" name=\"descFrame\">");
    docWriter.println("</FRAMESET>");

    generateTrailer(docWriter);
    docWriter.close();
  }

  private void generateTocHeader() throws IOException {
    generateHeader(tocWriter, "TOC");
    tocWriter.println("<BODY BGCOLOR=\"white\">");
    tocWriter.println("<TABLE BORDER=\"0\" WIDTH=\"100%\">");
    tocWriter.println("<TR>");
    tocWriter.println("<TD NOWRAP>");
  }

  private void generateTocTrailer() throws IOException {
    tocWriter.println("</TD>");
    tocWriter.println("</TR>");
    tocWriter.println("</TABLE>");
    tocWriter.println("</BODY>");
    generateTrailer(tocWriter);
  }

  private void generateDescHeader() throws IOException {
    descWriter.println("<HTML>");
    descWriter.println("<HEAD>");
    descWriter.println("<TITLE>");
    descWriter.println("Description");
    descWriter.println("</TITLE>");
    descWriter.println("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">");
    descWriter.println("<LINK REL =\"stylesheet\" TYPE=\"text/css\" HREF=\"stylesheet.css\" TITLE=\"Style\">");
    descWriter.println("</HEAD>");

    descHdrFtr.append(
        "<TABLE BORDER=\"0\" WIDTH=\"100%\" CELLPADDING=\"1\" CELLSPACING=\"0\">");
    descHdrFtr.append("<TR>");
    descHdrFtr.append(
        "<TD COLSPAN=2 BGCOLOR=\"#EEEEFF\" CLASS=\"NavBarCell1\">");
    descHdrFtr.append("<A NAME=\"navbar_top_firstrow\"><!-- --></A>");
    descHdrFtr.append(
        "<TABLE BORDER=\"0\" CELLPADDING=\"0\" CELLSPACING=\"3\">");
    descHdrFtr.append("<TR ALIGN=\"center\" VALIGN=\"top\">");
    descHdrFtr.append("<TD BGCOLOR=\"#FFFFFF\" CLASS=\"NavBarCell1Rev\"> &nbsp;<FONT CLASS=\"NavBarFont1Rev\"><B>Node List</B></FONT>&nbsp;</TD>");
    descHdrFtr.append("<TD BGCOLOR=\"#EEEEFF\" CLASS=\"NavBarCell1\"><A HREF=\"index-all.html\"><FONT CLASS=\"NavBarFont1\"><B>Index</B></FONT></A>&nbsp;</TD>");
    //descHdrFtr.append("<TD BGCOLOR=\"#EEEEFF\" CLASS=\"NavBarCell1\"><A HREF=\"help-doc.html\"><FONT CLASS=\"NavBarFont1\"><B>Help</B></FONT></A>&nbsp;</TD>");
    descHdrFtr.append("</TR>");
    descHdrFtr.append("</TABLE>");
    descHdrFtr.append("</TD>");
    descHdrFtr.append("<TD ALIGN=\"right\" VALIGN=\"top\" ROWSPAN=3><EM>");
    descHdrFtr.append("</EM>");
    descHdrFtr.append("</TD>");
    descHdrFtr.append("</TR>");
    descHdrFtr.append("</TABLE>");

    descWriter.println(descHdrFtr.toString());
    descWriter.println("<HR>");
  }

  private void generateDescTrailer() throws IOException {
    descWriter.println(descHdrFtr.toString());
    generateTrailer(descWriter);
  }

  private void generateIndex() {
    // Write out our header info

    indexWriter.println("<HTML>");
    indexWriter.println("<HEAD>");
    indexWriter.println("<TITLE>");
    indexWriter.println("Index");
    indexWriter.println("</TITLE>");
    indexWriter.println("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">");
    indexWriter.println("<LINK REL =\"stylesheet\" TYPE=\"text/css\" HREF=\"stylesheet.css\" TITLE=\"Style\">");
    indexWriter.println("</HEAD>");

    indexHdrFtr.append(
        "<TABLE BORDER=\"0\" WIDTH=\"100%\" CELLPADDING=\"1\" CELLSPACING=\"0\">");
    indexHdrFtr.append("<TR>");
    indexHdrFtr.append(
        "<TD COLSPAN=2 BGCOLOR=\"#EEEEFF\" CLASS=\"NavBarCell1\">");
    indexHdrFtr.append("<A NAME=\"navbar_top_firstrow\"><!-- --></A>");
    indexHdrFtr.append(
        "<TABLE BORDER=\"0\" CELLPADDING=\"0\" CELLSPACING=\"3\">");
    indexHdrFtr.append("<TR ALIGN=\"center\" VALIGN=\"top\">");
    indexHdrFtr.append("<TD BGCOLOR=\"#FFFFFF\" CLASS=\"NavBarCell1\"> &nbsp;<A HREF=\"desc.html\"><FONT CLASS=\"NavBarFont1\"><B>Node List</B></FONT></A>&nbsp;</TD>");
    indexHdrFtr.append("<TD BGCOLOR=\"#EEEEFF\" CLASS=\"NavBarCell1Rev\"><FONT CLASS=\"NavBarFont1Rev\"><B>Index</B></FONT>&nbsp;</TD>");
    //indexHdrFtr.append("<TD BGCOLOR=\"#EEEEFF\" CLASS=\"NavBarCell1\"><A HREF=\"help-doc.html\"><FONT CLASS=\"NavBarFont1\"><B>Help</B></FONT></A>&nbsp;</TD>");
    indexHdrFtr.append("</TR>");
    indexHdrFtr.append("</TABLE>");
    indexHdrFtr.append("</TD>");
    indexHdrFtr.append("<TD ALIGN=\"right\" VALIGN=\"top\" ROWSPAN=3><EM>");
    indexHdrFtr.append("</EM>");
    indexHdrFtr.append("</TD>");
    indexHdrFtr.append("</TR>");
    indexHdrFtr.append("</TABLE>");

    Set keys = indexMap.keySet();
    Iterator indexShortcutKeys = keys.iterator();
    String firstLetter = "";

    // Build set of first letter shortcuts to index pages

    while (indexShortcutKeys.hasNext()) {
      String tmpKey = (String) indexShortcutKeys.next();

      if (!tmpKey.substring(0, 1).equalsIgnoreCase(firstLetter)) {
        firstLetter = tmpKey.substring(0, 1);
        indexHdrFtr.append("<A HREF=\"#_" + firstLetter.toUpperCase() + "_\">" +
                           firstLetter.toUpperCase() + "</A>&nbsp;&nbsp;");
      }
    }

    // Build our list of indexed items

    indexWriter.println(indexHdrFtr.toString());
    indexWriter.println("<DL>");

    Iterator indexListKeys = keys.iterator();
    firstLetter = "";

    while (indexListKeys.hasNext()) {
      String tmpKey = (String) indexListKeys.next();

      if (!tmpKey.substring(0, 1).equalsIgnoreCase(firstLetter)) {
        firstLetter = tmpKey.substring(0, 1);
        indexWriter.println("<HR><BR><A NAME=\"_" + firstLetter.toUpperCase() +
                            "_\"><!-- --></A>");
      }

      DocumentationIndexObject tmpIndex = (DocumentationIndexObject) indexMap.
          get(tmpKey);
      indexWriter.println("<DT><A HREF=\"desc.html#" + tmpIndex.linkToNode +
                          "\"><B>" + Gen.xmlEscapeNode(tmpIndex.nodeName) + "</B></A>" +
                          ( (tmpIndex.nodeType.equals("node")) ?
                           " - Interior Node" : " - Leaf Node") +
                          ( (tmpIndex.nodeDesc != null) ?
                           ": " + tmpIndex.nodeDesc : "") + "<BR><BR>");
    }

    indexWriter.println("</DL>");
    indexWriter.println(indexHdrFtr.toString());
  }

  private void generateIndexTrailer() throws IOException {
    generateTrailer(indexWriter);
  }

  public void addIndexEntry(String tocEntry, HashMap parms) {
    DocumentationIndexObject tmpIndex = new DocumentationIndexObject();
    String tmpKey;

    if (!tocEntry.equals(".")) {
      tmpKey = tocEntry.substring(tocEntry.lastIndexOf("/") + 1);

      if (!tmpKey.equals("*") && !Character.isDigit(tmpKey.charAt(0))) {
        tmpIndex.nodeName = tmpKey;
        tmpIndex.nodeType = (String) parms.get("Type");

        if (tocEntry.length() > 1) {
          tmpIndex.linkToNode = tocEntry.substring(1);
        }
        else {
          tmpIndex.linkToNode = tocEntry;

        }
        if (parms.get("Description") != null) {
          tmpIndex.nodeDesc = (String) parms.get("Description");
        }

        if (!indexMap.containsKey(tmpKey.toLowerCase())) {
          indexMap.put(tmpKey.toLowerCase(), tmpIndex);
        }
        else {
          indexMap.put(tmpKey.toLowerCase() + indexDupCtr++, tmpIndex);
        }
      }
    }
  }

  public void addTocEntry(String tocEntry) throws IOException {
    String nodeName;

    if (tocEntry.equals(".")) {
      nodeName = tocEntry;
    }
    else if (tocEntry.length() > 1) {
      nodeName = tocEntry.substring(1);
    }
    else {
      nodeName = tocEntry;

    }
    tocWriter.println("<A HREF=\"desc.html#" + Gen.xmlEscapeNode(nodeName) +
                      "\" TARGET=\"descFrame\">" + Gen.xmlEscapeNode(nodeName) + "</A>");
    tocWriter.println("<BR>");
  }

  public void addDescEntry(HashMap parms) throws IOException {
    String nodeName;
    String uri = null;

    if (parms.get("URI") != null) {
      uri = (String) parms.get("URI");
    }
    else {
      return;
    }

    if (uri.equals(".")) {
      nodeName = uri;
    }
    else if (uri.length() > 1) {
      nodeName = uri.substring(1);
    }
    else {
      nodeName = uri;

    }
    descWriter.println("<A NAME=\"" + Gen.xmlEscapeNode(nodeName) + "\" </A>");
    descWriter.println("<p><b>" + Gen.xmlEscapeNode(nodeName) + "</b></p>");
    descWriter.println("<p></p>");

    if (parms.get("Description") != null) {
      descWriter.println("<p>" + (String) parms.get("Description") + "</p>");
      descWriter.println("<p></p>");
    }

    descWriter.println("<ul type=disc>");

    if (parms.get("Type") != null) {
      descWriter.println("<li>Format : " + (String) parms.get("Type") + "</li>");

    }
    if (parms.get("Mime") != null) {
      descWriter.println("<li>Mime Type : " + (String) parms.get("Mime") +
                         "</li>");

    }
    if (parms.get("Acl") != null) {
      descWriter.println("<li>ACL : " + (String) parms.get("Acl") + "</li>");

    }
    if (parms.get("Event") != null) {
        descWriter.println("<li>Event : " + (String) parms.get("Event") + "</li>");

    }
    if (parms.get("AccessType") != null) {
      ArrayList access = (ArrayList) parms.get("AccessType");
      descWriter.print("<li>Access Types : ");
      for (int i = 0; i < access.size(); i++) {
        if (i > 0) {
          descWriter.print(", ");
        }
        descWriter.print( (String) access.get(i));
      }

      descWriter.println("</li>");
    }

    if (parms.get("Default") != null) {
      descWriter.println("<li>Default : " + (String) parms.get("Default") +
                         "</li>");

    }
    if (parms.get("Value") != null) {
      descWriter.println("<li>Initial Value : " + (String) parms.get("Value") +
                         "</li>");

    }
    if (parms.get("values") != null) {
      descWriter.println("<li>Possible Values : " + (String) parms.get("values") +
                         "</li>");

    }
    if (parms.get("minLen") != null) {
      descWriter.println("<li>Min Length : " + (String) parms.get("minLen") +
                         "</li>");

    }
    if (parms.get("maxLen") != null) {
      descWriter.println("<li>Max Length : " + (String) parms.get("maxLen") +
                         "</li>");

    }
    if (parms.get("HandledByPlugin") != null) {
      descWriter.println("<li>Handled By Plugin : " +
                         (String) parms.get("HandledByPlugin") + "</li>");

    }
    if (parms.get("store") != null) {
      descWriter.println("<li>LOB Store : " + (String) parms.get("store") +
                         "</li>");

    }
    if (parms.get("LOBProgressBAR") != null) {
      descWriter.println("<li>LOB Progress Bar : " +
                         (String) parms.get("LOBProgressBAR") + "</li>");

    }
    if (parms.get("min") != null) {
      descWriter.println("<li>Minimum Value : " + (String) parms.get("min") +
                         "</li>");

    }
    if (parms.get("max") != null) {
      descWriter.println("<li>Maximum Value : " + (String) parms.get("max") +
                         "</li>");

    }
    if (parms.get("regexp") != null) {
      descWriter.println("<li>RegExp : " + (String) parms.get("regexp") +
                         "</li>");

    }
    if (parms.get("nMaxLen") != null) {
      descWriter.println("<li>Node Name Max Len : " +
                         (String) parms.get("nMaxLen") + "</li>");

    }
    if (parms.get("nValues") != null) {
      descWriter.println("<li>Node Name Possible Values : " +
                         (String) parms.get("nValues") + "</li>");

    }
    if (parms.get("nRegexp") != null) {
      descWriter.println("<li>Node Name Regexp : " +
                         (String) parms.get("nRegexp") + "</li>");

    }
    if (parms.get("auto") != null) {
      descWriter.println("<li>Auto Node Creation : " +
                         (String) parms.get("auto") + "</li>");

    }
    if (parms.get("fk") != null) {
      descWriter.println("<li>Foreign Key : " + (String) parms.get("fk") +
                         "</li>");

    }
    if (parms.get("child") != null) {
      descWriter.println("<li>Child Node Deletion : " +
                         (String) parms.get("child") + "</li>");

    }
    if (parms.get("depend") != null) {
      descWriter.println("<li>Deletion Dependency : " +
                         (String) parms.get("depend") + "</li>");

    }
    if (parms.get("recur-after-segment") != null) {
      descWriter.println("<li>Recurse After Segment : " +
                         (String) parms.get("recur-after-segment") + "</li>");

    }
    if (parms.get("max-recurrence") != null) {
      descWriter.println("<li>Maximum Recurrence : " +
                         (String) parms.get("max-recurrence") + "</li>");

    }
    descWriter.println("</ul>");
  }

  public void close() throws IOException {
    generateTocTrailer();
    tocWriter.close();

    generateDescTrailer();
    descWriter.close();

    generateIndex();
    generateIndexTrailer();
    indexWriter.close();
  }
}

class ByteSwapper {
  public static long swapLong(long value) {
    long b1 = (value >> 0) & 0xff;
    long b2 = (value >> 8) & 0xff;
    long b3 = (value >> 16) & 0xff;
    long b4 = (value >> 24) & 0xff;
    long b5 = (value >> 32) & 0xff;
    long b6 = (value >> 40) & 0xff;
    long b7 = (value >> 48) & 0xff;
    long b8 = (value >> 56) & 0xff;

    return b1 << 56 | b2 << 48 | b3 << 40 | b4 << 32 |
        b5 << 24 | b6 << 16 | b7 << 8 | b8 << 0;
  }

  public static int swapInt(int value) {
    int b1 = (value >> 0) & 0xff;
    int b2 = (value >> 8) & 0xff;
    int b3 = (value >> 16) & 0xff;
    int b4 = (value >> 24) & 0xff;

    return b1 << 24 | b2 << 16 | b3 << 8 | b4 << 0;
  }

  public static float swapFloat(float value) {
    int intValue = Float.floatToIntBits(value);
    intValue = swapInt(intValue);
    return Float.intBitsToFloat(intValue);
  }

  public static short swapShort(short value) {
    int b1 = value & 0xff;
    int b2 = (value >> 8) & 0xff;

    return (short) (b1 << 8 | b2 << 0);
  }
}

class MDFNode {
  public String nodeName;
  public int nodeNameOffset;
  public int nodeOffset;

  public short nodeType;
  public static final short NULL_TYPE = 0;
  public static final short NULL_MN_TYPE = 128;
  public static final short CHR_TYPE = 1;
  public static final short CHR_MN_TYPE = 129;
  public static final short INT_TYPE = 2;
  public static final short INT_MN_TYPE = 130;
  public static final short BOOL_TYPE = 3;
  public static final short BOOL_MN_TYPE = 131;
  public static final short BIN_TYPE = 4;
  public static final short BIN_MN_TYPE = 132;
  public static final short NODE_TYPE = 5;
  public static final short NODE_MN_TYPE = 133;
  public static final short DATE_TYPE = 8;
  public static final short DATE_MN_TYPE = 136;
  public static final short TEST_TYPE = 9;
  public static final short TEST_MN_TYPE = 137;
  public static final short TIME_TYPE = 10;
  public static final short TIME_MN_TYPE = 138;
  public static final short FLOAT_TYPE = 11;
  public static final short FLOAT_MN_TYPE = 139;
  public static final short STORES_PD = 256;
  public static final short HANDLE_BY_PLUGIN = 512;
  public static final short USE_NODE_ID = 1024;
  public static final short LOB_STORE = 2048;
  public static final short LOB_PROGRESS_BAR = 4096;

  public String metaNodeID;
  public int metaNodeIDOffset;

  public byte accessType;
  public static final byte ADD_ACCESS = 0x1;
  public static final byte DELETE_ACCESS = 0x2;
  public static final byte GET_ACCESS = 0x4;
  public static final byte REPLACE_ACCESS = 0x8;
  public static final byte EXEC_ACCESS = 0x10;
  public static final byte LOCAL_ACCESS = 0x20;

  public byte mimeType;
  public static final byte TEXT_PLAIN = 0;

  public byte numConstraints;
  public List constraints;
  public SortedMap children;

  public MDFNode() {
    constraints = new ArrayList();
    children = new TreeMap();
  }

  public void addConstraint(MDFConstraint constraint) {
    constraints.add(constraint);
  }

  public void addChild(String key, MDFNode node) {
    children.put(key, node);
  }

  public void toBinary(DataOutputStream out) throws IOException {
    out.writeInt(ByteSwapper.swapInt(nodeNameOffset));
    out.writeShort(ByteSwapper.swapShort(nodeType));

    if ( (nodeType & MDFNode.USE_NODE_ID) == MDFNode.USE_NODE_ID) {
      out.writeInt(ByteSwapper.swapInt(metaNodeIDOffset));
    }

    out.writeByte(accessType);
    out.writeByte(mimeType);
    out.writeShort(ByteSwapper.swapShort( (short) children.size()));

    if (children.size() > 0) {
      Set childKeys = children.keySet();
      Iterator childIterator = childKeys.iterator();

      while (childIterator.hasNext()) {
        out.writeInt(ByteSwapper.swapInt( ( (MDFNode) children.get(
            childIterator.next())).nodeOffset));
      }
    }

    out.writeByte(numConstraints);

    if (numConstraints > 0) {
      Iterator tmpConstraints = constraints.iterator();

      while (tmpConstraints.hasNext()) {
        ( (MDFConstraint) tmpConstraints.next()).toBinary(out);
      }
    }

    if (children.size() > 0) {
      Set childKeys = children.keySet();
      Iterator childIterator = childKeys.iterator();

      while (childIterator.hasNext()) {
        ( (MDFNode) children.get(childIterator.next())).toBinary(out);
      }
    }
  }

  public String toString() {
    StringBuffer tmpBuf = new StringBuffer();

    tmpBuf.append("Node Name: " + nodeName);
    tmpBuf.append("\nNode Name offset: " + nodeNameOffset);
    tmpBuf.append("\nNode Offset: " + nodeOffset);
    tmpBuf.append("\nNode Type: " + nodeType);

    if ( (nodeType & MDFNode.USE_NODE_ID) == MDFNode.USE_NODE_ID) {
      tmpBuf.append("\nMeta Node ID:" + metaNodeID);
      tmpBuf.append("\nMeta Node ID offset:" + metaNodeIDOffset);
    }

    tmpBuf.append("\nNode Access Type: " + accessType);
    tmpBuf.append("\nNode Mime Type: " + mimeType);
    tmpBuf.append("\nNumber of Constraints: " + numConstraints);

    if (children.size() > 0) {
      tmpBuf.append("\nNumber of Children: " + children.size());
      tmpBuf.append("\n============================================\n");

      Set childKeys = children.keySet();
      Iterator childIterator = childKeys.iterator();

      while (childIterator.hasNext()) {
        tmpBuf.append(children.get(childIterator.next()));
      }

      tmpBuf.append("\n---------------------------------------------");
      tmpBuf.append("\n\n");
    }

    tmpBuf.append("\n---------------------------------------------\n");

    Iterator tmpConstraints = constraints.iterator();

    while (tmpConstraints.hasNext()) {
      tmpBuf.append(tmpConstraints.next().toString());
    }

    tmpBuf.append("\n=============================================\n\n");

    return tmpBuf.toString();
  }
}

class MDFConstraint {
  public byte constraintType;
  public static final byte MIN_VALUE_CONSTRAINT = 1;
  public static final byte MAX_VALUE_CONSTRAINT = 2;
  public static final byte VALUES_CONSTRAINT = 3;
  public static final byte DEFAULT_VALUE_CONSTRAINT = 4;
  public static final byte MIN_LENGTH_CONSTRAINT = 5;
  public static final byte MAX_LENGTH_CONSTRAINT = 6;
  public static final byte REGEXP_CONSTRAINT = 7;
  public static final byte NAME_MAX_LENGTH_CONSTRAINT = 8;
  public static final byte NAME_VALUES_CONSTRAINT = 9;
  public static final byte NAME_REGEXP_CONSTRAINT = 10;
  public static final byte AUTONODE_CONSTRAINT = 11;
  public static final byte RECUR_AFTER_CONSTRAINT = 12;
  public static final byte MAX_RECUR_CONSTRAINT = 13;
  public static final byte FOREIGN_KEY_CONSTRAINT = 14;
  public static final byte DELETE_CHILD_CONSTRAINT = 15;
  public static final byte DEPEND_CHILD_CONSTRAINT = 16;
  public static final byte MAX_CHILD_CONSTRAINT = 17;

  public byte defaultValueType;
  public static final byte DEFAULT_INT_TYPE = 18;
  public static final byte DEFAULT_CHR_TYPE = 19;
  public static final byte DEFAULT_BYTE_TYPE = 20;
  public static final byte DEFAULT_FLOAT_TYPE = 21;

  public byte byteData;
  public short shortData;
  public int intData;
  public float floatData;
  public String chrData;
  public int strDataOffset;

  public MDFConstraint() {
  }

  public void toBinary(DataOutputStream out) throws IOException {
    out.writeByte(constraintType);

    switch (constraintType) {
      case MDFConstraint.MIN_VALUE_CONSTRAINT:
      case MDFConstraint.MAX_VALUE_CONSTRAINT:
        out.writeInt(ByteSwapper.swapInt(intData));
        break;

      case MDFConstraint.VALUES_CONSTRAINT:
      case MDFConstraint.REGEXP_CONSTRAINT:
      case MDFConstraint.NAME_VALUES_CONSTRAINT:
      case MDFConstraint.NAME_REGEXP_CONSTRAINT:
      case MDFConstraint.AUTONODE_CONSTRAINT:
      case MDFConstraint.RECUR_AFTER_CONSTRAINT:
      case MDFConstraint.FOREIGN_KEY_CONSTRAINT:
      case MDFConstraint.DELETE_CHILD_CONSTRAINT:
      case MDFConstraint.DEPEND_CHILD_CONSTRAINT:
        out.writeInt(ByteSwapper.swapInt(strDataOffset));
        break;

      case MDFConstraint.MIN_LENGTH_CONSTRAINT:
      case MDFConstraint.MAX_LENGTH_CONSTRAINT:
      case MDFConstraint.NAME_MAX_LENGTH_CONSTRAINT:
      case MDFConstraint.MAX_CHILD_CONSTRAINT:
      case MDFConstraint.MAX_RECUR_CONSTRAINT:
        out.writeShort(ByteSwapper.swapShort(shortData));
        break;

      case MDFConstraint.DEFAULT_VALUE_CONSTRAINT:
        switch (defaultValueType) {
          case DEFAULT_INT_TYPE:
            out.writeInt(ByteSwapper.swapInt(intData));
            break;

          case DEFAULT_FLOAT_TYPE:
            out.writeFloat(ByteSwapper.swapFloat(floatData));
            break;

          case DEFAULT_BYTE_TYPE:
            out.writeByte(byteData);
            break;

          case DEFAULT_CHR_TYPE:
            out.writeInt(ByteSwapper.swapInt(strDataOffset));
            break;
        }
        break;
    }
  }

  public String toString() {
    StringBuffer tmpBuf = new StringBuffer();

    tmpBuf.append("Constraint Type: " + constraintType);
    tmpBuf.append("\nConstraint Value: ");

    switch (constraintType) {
      case MDFConstraint.MIN_VALUE_CONSTRAINT:
      case MDFConstraint.MAX_VALUE_CONSTRAINT:
        tmpBuf.append(intData);
        break;

      case MDFConstraint.VALUES_CONSTRAINT:
      case MDFConstraint.REGEXP_CONSTRAINT:
      case MDFConstraint.NAME_VALUES_CONSTRAINT:
      case MDFConstraint.NAME_REGEXP_CONSTRAINT:
      case MDFConstraint.AUTONODE_CONSTRAINT:
      case MDFConstraint.RECUR_AFTER_CONSTRAINT:
      case MDFConstraint.FOREIGN_KEY_CONSTRAINT:
      case MDFConstraint.DELETE_CHILD_CONSTRAINT:
      case MDFConstraint.DEPEND_CHILD_CONSTRAINT:
        tmpBuf.append(chrData + " - Offset: " + strDataOffset);
        break;

      case MDFConstraint.MIN_LENGTH_CONSTRAINT:
      case MDFConstraint.MAX_LENGTH_CONSTRAINT:
      case MDFConstraint.NAME_MAX_LENGTH_CONSTRAINT:
      case MDFConstraint.MAX_CHILD_CONSTRAINT:
      case MDFConstraint.MAX_RECUR_CONSTRAINT:
        tmpBuf.append(shortData);
        break;

      case MDFConstraint.DEFAULT_VALUE_CONSTRAINT:
        switch (defaultValueType) {
          case DEFAULT_INT_TYPE:
            tmpBuf.append(intData);
            break;

          case DEFAULT_FLOAT_TYPE:
            tmpBuf.append(floatData);
            break;

          case DEFAULT_BYTE_TYPE:
            tmpBuf.append(byteData);
            break;

          case DEFAULT_CHR_TYPE:
            tmpBuf.append(chrData + " - Offset: " + strDataOffset);
            break;
        }
        break;
    }

    tmpBuf.append("\n");
    return tmpBuf.toString();
  }
}

class DDFGen {
  private static final String ddffile = "syncmldm.ddf";
  private static final String shift = " ";
  private PrintWriter ddfWriter = null;
  private StringBuffer valuesForDDF = new StringBuffer();
  
  public DDFGen() throws Exception {
      ddfWriter = new PrintWriter(Util.openUtf8FileWriter(ddffile));

    ddfWriter.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    ddfWriter.println("<!DOCTYPE MgmtTree SYSTEM \"http://www.openmobilealliance.com/tech/DTD/dm_ddf-v1_2.dtd\">");        
    ddfWriter.println("<MgmtTree>");
    ddfWriter.println(shift + "<VerDTD>1.2</VerDTD>");
    ddfWriter.println(shift + "<Man>Motorola</Man>");
    ddfWriter.println(shift + "<Mod>" + Gen.phoneModel + "</Mod>");
  }

  public void addDDFForNode(HashMap parms, String indent) throws IOException {
    ddfWriter.println(indent + "<Node>");
    String nodeName = (String) parms.get("NodeName");
    boolean multinode = false;

    if (nodeName.equals("*")) {
      multinode = true;

    }
    if (multinode) {
      ddfWriter.println(indent + shift + "<NodeName/>");
    }
    else {
      ddfWriter.println(indent + shift + "<NodeName>" +
              Gen.xmlEscapeNode((String) parms.get("NodeName")) + "</NodeName>");
    }
    String path = (String) parms.get("URI");
    if (path !=null){
          String fileSep = System.getProperty("file.separator");
        if(path.indexOf(fileSep) > 0){
            path=path.substring(0, path.lastIndexOf(fileSep));
        }else{
            path="";
        }
        ddfWriter.println(indent + shift + "<Path>" +
                  Gen.xmlEscapeNode(path) + "</Path>");       
    }
    ddfWriter.println(indent + shift + "<DFProperties>");
    
    ddfWriter.println(indent + shift + shift + "<AccessType>");
    ArrayList access = (ArrayList) parms.get("AccessType");
    if(access.contains("Add")){
        ddfWriter.println(indent + shift + shift + shift + "<Add/>");
    }
    if(access.contains("Delete")){
        ddfWriter.println(indent + shift + shift + shift + "<Delete/>");
    }
    if(access.contains("Exec")){
        ddfWriter.println(indent + shift + shift + shift + "<Exec/>");
    }
    if(access.contains("Get")){
        ddfWriter.println(indent + shift + shift + shift + "<Get/>");
    }
    if(access.contains("Replace")){
        ddfWriter.println(indent + shift + shift + shift + "<Replace/>");
    }
    ddfWriter.println(indent + shift + shift + "</AccessType>");
    
    if(Gen.generateExtDDF){
        //get value or default value if presents  
        String tmp = (String)parms.get("Value");
        if(tmp != null){
            tmp = (String)parms.get("Default");           
        }
        if(tmp != null){
            ddfWriter.println(indent + shift + shift + "<DefaultValue>" + Gen.xmlEscape(tmp) + "</DefaultValue>");           
        }
        //add main constarins under <Description>
        StringBuffer sbDescription = new StringBuffer();
        tmp = (String)parms.get("values");
        if(tmp != null){
            valuesForDDF.append("[" + (String) parms.get("URI") + "]\n");
            valuesForDDF.append(Gen.xmlEscape(tmp) +"\n\n"); 
            sbDescription.append(indent + shift + shift + shift + "List of allowed values: " + tmp + "\n");
        }
        tmp = (String)parms.get("min");
        if(tmp != null){
            sbDescription.append(indent + shift + shift + shift + "Minimum value: " + tmp + "\n");
        }
        tmp = (String)parms.get("max");
        if(tmp != null){
            sbDescription.append(indent + shift + shift + shift + "Maximum value: " + tmp + "\n");
        }
        tmp = (String)parms.get("minLen");
        if(tmp != null){
            sbDescription.append(indent + shift + shift + shift + "Minimum length: " + tmp + "\n");
        }
        tmp = (String)parms.get("maxLen");
        if(tmp != null){
            sbDescription.append(indent + shift + shift + shift + "Maximum length: " + tmp + "\n");
        }
        if(sbDescription.length() > 0){
            ddfWriter.println(indent + shift + shift + "<Description>");
            ddfWriter.print(sbDescription.toString());
            ddfWriter.println(indent + shift + shift + "</Description>");
        }    
    }
    
    ddfWriter.println(indent + shift + shift + "<DFFormat>");
    ddfWriter.println(indent + shift + shift + shift +
                      "<" + (String) parms.get("Type") + "/>");
    ddfWriter.println(indent + shift + shift + "</DFFormat>");

    if (multinode) {       
        ddfWriter.println(indent + shift + shift + "<Occurrence>");
        ddfWriter.println(indent + shift + shift + shift + "<ZeroOrMore/>");
        ddfWriter.println(indent + shift + shift + "</Occurrence>");
    }

    if (parms.get("Mime") != null) {
      ddfWriter.println(indent + shift + shift + "<DFType>");
      ddfWriter.println(indent + shift + shift + shift + "<MIME>" +
                        parms.get("Mime") + "</MIME>");
      ddfWriter.println(indent + shift + shift + "</DFType>");
    }
    ddfWriter.println(indent + shift + shift + "<CaseSense>");
    ddfWriter.println(indent + shift + shift + shift + "<CS/>");
    ddfWriter.println(indent + shift + shift + "</CaseSense>");

    ddfWriter.println(indent + shift + "</DFProperties>");
   
  }

  public void nodeDelim(String indent) throws IOException {
    ddfWriter.println(indent + "</Node>");
  }

  public void close() throws IOException {
    ddfWriter.println("</MgmtTree>");
    ddfWriter.close();
    if(Gen.generateExtDDF){
        writeValuesTxt();
    }
  }
  
 
  //write  values.txt for DDF file
  private final void writeValuesTxt() throws IOException{
      if(valuesForDDF.length() > 0){
          PrintWriter ddfValsWriter  = new PrintWriter(Util.openUtf8FileWriter("ddfvalues.txt"));
          ddfValsWriter.println(valuesForDDF.toString());
          ddfValsWriter.close();    
          ddfValsWriter = null;
      } 
  }
}

// class convert .bmdf, fstab, .ini  to .c and .h files (for P2K)

class File2C {
  String outputDir = "";
  final String BODY = "<BODY>";
  final String SIZE = "<SIZE>";

  public File2C() {}

  public File2C(String outputDir) {
    outputDir += (outputDir.endsWith("/")) ? "" : "/";
    this.outputDir = outputDir;
  }

  public void convert(String filePath) throws Exception {
    boolean isTextFile = !filePath.toLowerCase().endsWith(".bmdf");
    convert(filePath, isTextFile);
  }

  public void convert(String filePath, boolean isTextFile) throws Exception {
    File file = new File(filePath);
    if (!file.exists()) {
      throw new Exception("File does not exist");
    }

    String name = replaceStr(file.getName(), ".", "_");
    String strNewFile;
    if (isTextFile) {
      strNewFile = convertText2C(file, name);
    }
    else {
      strNewFile = convertBytes2C(file, name);
    }
    writeFile(outputDir + name + ".c", strNewFile);

    strNewFile = convert2H(name);
    writeFile(outputDir + name + ".h", strNewFile);
  }

  public String convertText2C(File file, String name) throws Exception {
    StringBuffer body = new StringBuffer();
    Reader fr = Util.openUtf8FileReader(file);
    BufferedReader br = new BufferedReader(fr);
    int fileSize = 0;
    String line;
    String str;
    while ( (line = br.readLine()) != null) {
      line = line.trim();
      if (line.startsWith("#") || line.length() == 0) {
        continue;
      }
      if (name.equalsIgnoreCase("fstab")) {
        line = replaceStr(line, "\t", " ");
        String strTmp = null;
        while (!line.equals(strTmp)) {
          strTmp = line;
          line = replaceStr(line, "  ", " ");
        }
      }

      fileSize += line.length() + 1;
      line = "\n    \"" + line + "\"  \"\\0\"";
      body.append(line);
    }
    body.append("\n;\n");
    String result = getCTemplate(name, false);
    String strBody = body.toString();
    result = replaceStr(result, BODY, strBody);
    result = replaceStr(result, SIZE, fileSize + "");
    try {
      br.close();
    }
    catch (Exception e) {}
    try {
      fr.close();
    }
    catch (Exception e) {}

    br = null;
    fr = null;

    return result;
  }

  public String convertBytes2C(File file, String name) throws Exception {
    StringBuffer body = new StringBuffer();

    FileInputStream fstream = new FileInputStream(file);
    byte[] bytes = new byte[fstream.available()];
    if (bytes.length == 0) {
      throw new Exception("File is empty. Path: " + file.getPath());
    }
    fstream.read(bytes);
    fstream.close();
    String str;

    if (file.getName().toLowerCase().endsWith(".bmdf")) {
      validateBMDFfile(bytes);
    }

    for (int i = 0; i < bytes.length; i++) {
      str = "0x" + Integer.toHexString( ( (int) bytes[i]) & 0xff);
      str += ( (i + 1) == bytes.length) ? "" :
          ( (str.length() < 4) ? " , " : ", ");
      str += ( ( (i + 1) % 25) == 0) ? "\n" : "";
      body.append(str);
    }
    String result = getCTemplate(name, true);
    result = replaceStr(result, BODY, body.toString());
    result = replaceStr(result, SIZE, bytes.length + "");

    return result;
  }

  public void validateBMDFfile(byte[] bytes) throws Exception {
    if (bytes.length <= 4) {
      throw new Exception("File size less then 4 chars.\n");
    }
    int values[] = {
        ( (int) bytes[0]) & 0xff,
        ( (int) bytes[1]) & 0xff,
        ( (int) bytes[2]) & 0xff,
        ( (int) bytes[3]) & 0xff
    };
    int value = 0;

    if (values[0] == '[' && (values[1] == '/' || values[2] == '.')) {
      throw new Exception("BMDF: file not recognized as a bmdf file.\n");
    }

    for (int i = 0; i < 4; i++) {
      value |= values[i] << (i * 8);
    }
    if (value != bytes.length) {
      throw new Exception("Corrupted mdf file: size not match");
    }
  }

  public String convert2H(String name) throws Exception {

    StringBuffer sb = new StringBuffer();
    sb.append("#ifndef " + name + "_H\n");
    sb.append("#define " + name + "_H\n\n");
    sb.append("#include \"xpl_Types.h\"\n\n");
    sb.append("#ifdef __cplusplus \n extern \"C\" {\n#endif\n\n");
    sb.append("const UINT8 * dmGet_" + name + "(UINT32 * size);\n\n");
    sb.append("#ifdef __cplusplus \n}\n#endif\n\n");
    sb.append("#endif\n");

    return sb.toString();
  }

  private String getCTemplate(String name, boolean isBinary) {
    StringBuffer sb = new StringBuffer();
    sb.append("#include \"" + name + ".h\"\n\n");
    sb.append("#ifdef __cplusplus \n extern \"C\" {\n#endif\n\n");
    sb.append("static UINT8 " + name + "[] = ");
    if (isBinary) {
      sb.append("{\n");
    }
    sb.append(BODY);
    sb.append("\n");
    if (isBinary) {
      sb.append("};\n\n");
    }
    sb.append("const UINT8 * dmGet_" + name + "(UINT32 * size)\n");
    sb.append("{\n");
    sb.append("    if ( size )\n");
    sb.append("    {\n");
    sb.append("        *size = " + SIZE + ";\n");
    sb.append("        return " + name + ";\n");
    sb.append("    }\n");
    sb.append("    else\n");
    sb.append("    {\n");
    sb.append("        return NULL;\n");
    sb.append("    }\n");
    sb.append("}\n\n");
    sb.append("#ifdef __cplusplus \n}\n#endif\n\n");
    return sb.toString();
  }

  private String replaceStr(String str, String oldPat, String newPat) {
    String result = str;
    int from = 0;
    while ( (from = str.indexOf(oldPat, from)) >= 0) {
      result = str.substring(0, from);
      result += newPat;
      result += str.substring(from + oldPat.length(), str.length());
      str = result;
    }
    return result;
  }

  public void writeFile(String path, String body) throws Exception {
    PrintWriter fileWriter = new PrintWriter(Util.openUtf8FileWriter(path));
    
    fileWriter.print(body);
    fileWriter.flush();
    fileWriter.close();
    fileWriter = null;
  }
}

// Main public class
public class Gen {
  public final static String GEN_TOOL_VERSION = "1.11";
  private static short BMDF_VERSION = 1;
  private int depth = 1;
  final static private String shift = " ";
  final static private String multi = "[ ]";
  final static private String parm = "parm.txt";
  static private String initDir = "D:\\Dmt";
  final static private String dataFile = "d_dmtree.xml";
  final static private String metaDataFile = "root.mdf";
  final static private String binaryMetaDataFile = "root.bmdf";
  final static private String aclFile = "acl.txt";
  final static private String aclFileDat = "acl.dat";
  final static private String eventFile = "event.txt";
  final static private String eventFileDat = "event.dat";  
  static private String fstabFile = "fstab";
  static private Archives dataOut = null;
  static private PrintWriter metaDataOut = null;
  static private FileOutputStream binaryMetaDataOut = null;
  static private PrintWriter aclOut = null;
  static private PrintWriter eventOut = null;
  static private StringWriter metaDataSW = null;
  static private PrintWriter metaDataBuffer = null;
  private String[] path = new String[20];
  static private String softwareVersion = "";
  static private Documentation docs = null;
  static private DDFGen ddfgen = null;
  static private boolean generateDocs = false;
  static private boolean generateDDF = false;
  static private boolean MDFconversion = false;
  static private MDFNode rootMDF = new MDFNode();
  private static int totalMDFBytes = 0;
  private static int nodeConstraintBytes = 0;
  private static Map stringTable = new HashMap();
  private static SortedMap revStringTable = new TreeMap();
  private static StringBuffer sbAclData = new StringBuffer();
  private static StringBuffer sbEventData = new StringBuffer();
  private static StringBuffer sbEventDataFromParms = new StringBuffer();
  private static ArrayList arrAclData = new ArrayList();
  private static ArrayList arrEventData = new ArrayList();
  private static String iniPath = null;
  public static String phoneModel = "unknown";
  public static boolean generateExtDDF = false;
  private String factoryBootstrapEncodeValue = null;
  
  private static void usage() {
    System.err.println(
        "java Gen -d dirName -f fstab [-gendoc] [-genddf | -genextddf] [-sv version] \n" +
        "\t -d         : dirName points to the directory where Dmt is located\n" +
        "\t -f         : location of fstab file\n" +
        "\t -i         : location of ini file(s)\n" +
        "\t -gendoc    : optional argument to generate documentation\n" +
        "\t -genddf    : optional argument to generate standard DDF file\n" +
        "\t -genextddf : optional argument to generate DDF file with values and DDFValues.txt\n" +
        "\t -sv        : optional argument specifies software version to be\n" +
        "\t\t            included in the DM tree\n");
  }

  // take string and make sure that each < > & etc are escaped properly
  public static String xmlEscape(String s) {
    StringTokenizer st = new StringTokenizer(s, "<>&\"$#*", true);
    StringBuffer sb = new StringBuffer(s.length());
    if (!st.hasMoreTokens()) {
      return s;
    }

    while (st.hasMoreTokens()) {
      s = st.nextToken();
      if (s.equals("<")) {
        sb.append("&lt;");
      }
      else if (s.equals(">")) {
        sb.append("&gt;");
      }
      else if (s.equals("&")) {
        sb.append("&amp;");
      }
      else if (s.equals("$")) {
        sb.append("&#36;");
      }
      else if (s.equals("\"")) {
        sb.append("&#34;");
      }
      else if (s.equals("#")) {
        sb.append("&#35;");
      }
      else if (s.equals("*")) {
        sb.append("&#42;");
      }
      else {
        sb.append(s);
      }
    }
    return sb.toString();
  }
  
  //escape illigal chars in the node name or uri
  public static final String xmlEscapeNode(String nodeNameOrUri) {
      return (nodeNameOrUri == null) ? "" : nodeNameOrUri.replaceAll("&", "&#38;");
  }
  
  public static void main(String[] args) throws Exception {
      System.out.println("Gen Tool version " + Gen.GEN_TOOL_VERSION);
      try {
          Gen.genTree(args);
      }catch (Exception e){
         System.err.println(e.getMessage());
        System.exit(1);
      }
      System.exit(0);  
  }
  
  // This function replaced main() for compatibility with LV flexlib
  public static void genTree(String[] args) throws Exception {
      //parse arguments        
    for (int ac = 0; ac < args.length; ac++) {
      if (args[ac].equals("-mdfconvert")) {
        MDFconversion = true;
        ac++;
      }
      else if (args[ac].equals("-d")) {
        if (args.length < ac + 2) {
          usage();
          throw new Exception("Error: Dmt location has not been specified");
        }
        initDir = args[++ac];
      }
      else if (args[ac].equals("-f")) {
        if (args.length < ac + 2) {
          usage();
          throw new Exception("Error: fstab file location has not been specified");
        }
        fstabFile = args[++ac];
      }
      else if (args[ac].equals("-i")) {
        if (args.length < ac + 2) {
          usage();
          throw new Exception("Error: 'ini' file location has not been specified");
        }
        iniPath = args[++ac];
      }
      else if (args[ac].equals("-gendoc")) {
        generateDocs = true;
      }
      else if (args[ac].equals("-genddf")) {
        generateDDF = true;
      }
      else if (args[ac].equals("-genextddf")) {
          generateDDF = true;
          generateExtDDF = true;
      }     
      else if (args[ac].equals("-sv")) {
        if (args.length < ac + 2) {
          usage();
          throw new Exception("Error: software version is not specified");
        }
        softwareVersion = args[++ac];
      } 
    }
    
    //set phone model... read from parm.txt
    try {
      setPhoneModel(initDir);
    }catch (Exception e){
        throw new Exception("Error: can not read phone model from /DevInfo/#Mod/parm.txt. " + e.getMessage());
    } 
    
    // writing acl, event, metadata, ddf, ...
    try
    {
      if (!MDFconversion) {
        dataOut = new Archives(fstabFile);
        metaDataOut = new PrintWriter(new BufferedWriter(Util.openUtf8FileWriter(metaDataFile)));
        aclOut = new PrintWriter(new BufferedWriter(Util.openUtf8FileWriter(aclFile)));
      }

      if (generateDocs) {
        docs = new Documentation();
      }
    
      if (generateDDF) {
        ddfgen = new DDFGen();
      }
    
      if (!MDFconversion) {
        dataOut.printAll("<MgmtTree>\n");
        dataOut.printAll(shift + "<VerDTD>1.1.2</VerDTD>\n");
        dataOut.printAll(shift + "<Man>Motorola</Man>\n");
        dataOut.printAll(shift + "<Mod>" + Gen.phoneModel + "</Mod>\n");
      }

      Gen gen = new Gen();
      gen.analyzeDir(initDir, rootMDF, true, false, false, false);

      if (!MDFconversion) {
        dataOut.printAll("</MgmtTree>\n");
        aclOut.close();       
        metaDataOut.close();
        dataOut.close();
        writeAclDatFile();
        // write event.dat and event.txt files
        if(sbEventDataFromParms.length() > 0){
            eventOut = new PrintWriter(new BufferedWriter(Util.openUtf8FileWriter(eventFile)));
            eventOut.print(sbEventDataFromParms.toString());
            eventOut.close();
            writeEventDatFile();       
        }
      }

      if (generateDocs) {
        docs.close();
      }

      if (generateDDF) {
        ddfgen.close();
      }

      gen.processMDFStrings(rootMDF);
      gen.processNodeAndConstraintOffsets(rootMDF);
      totalMDFBytes += 6;

      System.out.println(rootMDF.toString());
      System.out.println("String table (raw) = " + stringTable);
      System.out.println("String table (output order) = " + revStringTable);
      System.out.println();
      System.out.println("Total Number of Bytes = " + totalMDFBytes);

    }catch (Exception e){
        throw new Exception("Error: " + e.getMessage());
    } 
      
    // Now actually write out the bytes to bmdf file
    DataOutputStream bmdfOut = null;
    try {
      binaryMetaDataOut = new FileOutputStream(binaryMetaDataFile);
      bmdfOut = new DataOutputStream(binaryMetaDataOut);

      // Write out the total # of bytes to the file
      bmdfOut.writeInt(ByteSwapper.swapInt(totalMDFBytes));

      // Write out the BMDF file version
      bmdfOut.writeShort(ByteSwapper.swapShort(BMDF_VERSION));

      // Put the MDF data into the buffer
      rootMDF.toBinary(bmdfOut);

      // Put the string table into the buffer
      Set stKeys = revStringTable.keySet();
      Iterator stIterator = stKeys.iterator();

      while (stIterator.hasNext()) {
        bmdfOut.writeBytes( ( (String) revStringTable.get(stIterator.next()) + "\0"));
      }

      bmdfOut.flush();   
    }
    catch (Exception e) {
        throw new Exception("Error: unable to create binary metadata file. " + e.getMessage());
    }
    finally{
      try {
        binaryMetaDataOut.close();
      }catch (Exception ex){}     
      
      try {
        bmdfOut.close();
      }catch (Exception ex){}
     
      binaryMetaDataOut=null;
      bmdfOut=null;
    }
        
    // For P2K only... Generate .c and .h files from sysplagins.ini (if presents)
    if (iniPath != null) {
      try {
          convertToCAndH();
      }catch (Exception e){
          throw new Exception("Error: fail to convert files to .c and .h. " + e.getMessage());
      }      
    }
    
    //Convert all XML files to WBXML
    try {
      convert2WBXML();
    }catch (Exception e){
        throw new Exception("Error: during conversion xml to wbxml. " + e.getMessage());
    }
  }

  //Convert all XML files from current directory to WBXML (replace Python script).
  private static void convert2WBXML() throws Exception {
      boolean isXmlFileFound = false;
    Xml2WBXml x2b = new Xml2WBXml();
    File top = new File(".");
    File f;
    for (int i = 0; i < top.listFiles().length; i++) {
      f = top.listFiles()[i];
      if (f.getName().toUpperCase().endsWith(".XML")) {
        isXmlFileFound = true;
        x2b.convert(f);
      }
    }
    if (!isXmlFileFound){
      throw new Exception("There are no single xml files have been created.");
    }
  }

  //set phone model global variable which will be used for ddf and MDF
  public static void setPhoneModel(String initDir) throws Exception{
    BufferedReader br = null;
    Reader reader = null;
    try {
      Node f = NodeLoader.getInstance(initDir + "/DevInfo/#Mod/parm.txt"); // changed to support FlexML, replace new File()
      reader = NodeLoader.getReader(f);
      br = new BufferedReader(reader); // changed to support FlexML, replaced File()
      String line;
      while ( (line = br.readLine()) != null) {
        if (line.startsWith("value:")) {
          phoneModel = line.substring(6, line.length());
          break;
        }
      }
    }
    catch (Exception e) {
      throw e;
    }
    finally {
      try {
        br.close();
      }
      catch (Exception e) {}
      
      try {
        reader.close();
      }
      catch (Exception e) {}

      br = null;
      reader = null;
    }
  }

  public boolean analyzeDir(String dirName, MDFNode mdf, boolean metaData,
                            boolean noMetaDataGen, boolean parmNoMetaData, 
                            boolean foundMultiNode) throws Exception {
      
    // Create the indentation string

    //YXU Add
    boolean isLeaf = false;
    boolean notMultiNode = true;
    // this variable used as a flag for ddfgen.nodeDelim() method to close node with "</Node>"
    // has been introduced to avoid adding multi node's instances into ddf file
    boolean bAddDDFCalled = false;

    String indent = "";
    depth += 1;

    for (int ii = 0; ii < depth; ii++) {
      indent += shift;
    }

    // Remove leading (leaf) and trailing (reserved name) "#"
    String nodeName;
    if (depth <= 2) {
      nodeName = ".";
    }
    else {
      nodeName = dirName.substring(dirName.lastIndexOf(System.getProperty(
          "file.separator")) + 1);
      //YXU Add
      if (nodeName.startsWith("#")) {
        isLeaf = true;
      }
      nodeName = nodeName.replace('#', ' ');
      if (nodeName.startsWith("@")) {
        nodeName = nodeName.substring(1);
      }
      nodeName = nodeName.trim();
      if (nodeName.charAt(0) == '[') {
        nodeName = "*";
        metaData = false; // Indicate that data part of the tree is
        // not to be generated underneath this node
        notMultiNode = false;
      }
      path[depth - 2] = nodeName;
    }

    //Get the current node path
    String nodepath = ".";
    for (int i = 1; i < depth - 1; i++) {
      nodepath = nodepath + "/" + path[i];
    }
    System.out.println("nodepath = " + nodepath);

    metaDataSW = new StringWriter();
    metaDataBuffer = new PrintWriter(metaDataSW);

    if (!noMetaDataGen) {
      if (depth > 2) {
        metaDataBuffer.print("[");
        for (int ii = 1; ii < depth - 1; ii++) {
          metaDataBuffer.print("/" + path[ii].trim());
        }
        metaDataBuffer.print("]\n");
      }
      else {
        metaDataBuffer.println("[.]");
      }
    }
    Node directory = NodeLoader.getInstance(dirName); //changed to support FlexML, replaced File()
    Node[] children = directory.listNodes(); //changed to support FlexML, replaced listFiles().
    
    //sorting nodes
    Util.quickSort(children);

    //YXU else
    //type:chr
    //maxLen:31
    //access:Get
    //mime:text/plain
    //default:1.0
    { // *** bracket uses for scope separation only
      // Setting the default parameter values
      HashMap parameters = new HashMap();
      //YXU parameters.put("Format", "node");
      if (isLeaf) {
        parameters.put("Type", "chr");
      }
      else {
        parameters.put("Type", "node");
      }

      parameters.put("Mime", "text/plain");
      ArrayList access = new ArrayList();
      access.add("Get");
      access.add("Add");
      access.add("Replace");
      access.add("Delete");
      parameters.put("AccessType", access); //YXU "text/plain");

      parameters.put("URI", nodepath);
      parameters.put("NodeName", nodeName);

      String filename = dirName + "/" + parm;

      // Get parameters from the current branch
      
      if (mdf != null) {
        mdf.nodeName = nodeName;
      }

      getParms(filename, parameters, access, mdf, notMultiNode, noMetaDataGen);

      if (generateDocs) {
        docs.addTocEntry(nodepath);
        docs.addDescEntry(parameters);
        docs.addIndexEntry(nodepath, parameters);
      }

      // Get parameters from the parallel meta-data branch, if any
      String metaBranch = "";
      int index;
      int curr = 0;
      boolean meta = false;
      while ( (index = filename.substring(curr).indexOf("/")) > 0) {
        if (filename.charAt(curr) == '@') {
          metaBranch += "/" + multi;
          meta = true;
        }
        else {
          metaBranch += "/" + metaBranch + filename.substring(curr, index);
        }
        curr = index + 1;
      }
      if (meta) {
        getParms(metaBranch, parameters, access, null, notMultiNode,
                 noMetaDataGen);
      }

      if (parameters.get("DataGen") != null) {
        System.out.println("-No Data tree generation flag is set");
        metaData = false;
      }

      if (parameters.get("MetaGen") != null) {
        System.out.println("-No meta data generation flag is set");
        noMetaDataGen = true;
        parmNoMetaData = true;
      }

      if (!noMetaDataGen) {
        if (!MDFconversion) {
          metaDataOut.print(metaDataSW.toString());
          metaDataSW.flush();
        }

        totalMDFBytes += 11;
      }

      if (generateDDF && !foundMultiNode) {
        ddfgen.addDDFForNode(parameters, indent);
        bAddDDFCalled = true;
      }

      if (metaData && !MDFconversion) {
        dataOut.print(nodepath, "\n" + indent + "<Node>\n");
        dataOut.print(nodepath,
                      indent + shift + "<NodeName>" + Gen.xmlEscapeNode(nodeName) +
                      "</NodeName>\n");
      }

      // Generate the entries into the xml string
      if (metaData && !MDFconversion) {
        //YXU dataOut.print(indent + shift + "<RTProperties>\n");
        dataOut.print(nodepath, indent + shift + "<DFProperties>\n");

        //YXU
        //dataOut.print(indent + shift + shift + "<Type>");
        //dataOut.print(//YXU indent + shift + shift + shift +
        //YXU "<DDFName>" +
        //(String)parameters.get("Type") +
        //YXU "</DDFName>"+
        //"");
        //dataOut.print(//YXU indent + shift + shift +
        //  "</Type>\n");

        //YXU dataOut.print(indent + shift + shift + "<Format>\n");
        dataOut.print(nodepath, indent + shift + shift + "<DFFormat>\n");
        dataOut.print(nodepath, indent + shift + shift + shift +
                      //"<" + (String)parameters.get("Format") + "/>\n");
                      "<" + (String) parameters.get("Type") + "/>\n");
        //dataOut.print(indent + shift + shift + "</Format>\n");
        dataOut.print(nodepath, indent + shift + shift + "</DFFormat>\n");

        //GWM
        dataOut.print(nodepath, indent + shift + shift + "<AccessType>\n");
        ArrayList tmpAccess = (ArrayList) parameters.get("AccessType");
        for (int i = 0; i < tmpAccess.size(); i++) {
          dataOut.print(nodepath, indent + shift + shift + shift +
                        "<" + (String) tmpAccess.get(i) + "/>\n");
        }
        dataOut.print(nodepath, indent + shift + shift + "</AccessType>\n");

        //YXU dataOut.print(indent + shift + "</RTProperties>\n");
        dataOut.print(nodepath, indent + shift + "</DFProperties>\n");

        //YXU
        /*
           if (parameters.get("Acl") !=null )
            {
         dataOut.print(indent + shift + "<RTProperties>\n");
         dataOut.print(indent + shift + shift + "<ACL>");
         dataOut.print( (String)parameters.get("Acl")   );
         dataOut.print("</ACL>\n");
         dataOut.print(indent + shift + "</RTProperties>\n");
            }
         */
        //YXU
        dataOut.print(nodepath, indent + shift + shift + "<Type>");
        dataOut.print(nodepath, (String) parameters.get("Mime"));
        dataOut.print(nodepath, "</Type>\n");

        if (nodepath.equals("./DevDetail/SwV")) {
          dataOut.print(nodepath, indent + shift + shift + "<Data>");
          dataOut.print(nodepath, softwareVersion);
          dataOut.print(nodepath, "</Data>\n");
        }
        else if (parameters.get("Value") != null) {
          dataOut.print(nodepath, indent + shift + shift + "<Data>");
          dataOut.print(nodepath, xmlEscape( (String) parameters.get("Value")));
          dataOut.print(nodepath, "</Data>\n");
        }
      }
      if (parameters.get("Acl") != null) {
        aclOut.print("[");
        sbAclData.append("[");
        if (depth <= 2) {
          aclOut.print(".");
          sbAclData.append(".");
        }
        else {
          for (int ii = 1; ii < depth - 1; ii++) {
            aclOut.print("/" + path[ii].trim());
            sbAclData.append("/" + path[ii].trim());
          }
        }
        aclOut.println("]");
        sbAclData.append("]\n");
        aclOut.println( (String) parameters.get("Acl"));
        addAclParmsToDat( (String) parameters.get("Acl"));
      }
      if (parameters.get("Event") != null) {
          sbEventDataFromParms.append("[");
        sbEventData.append("[");
        if (depth <= 2) {
            sbEventDataFromParms.append(".");
          sbEventData.append(".");
        }
        else {
          for (int ii = 1; ii < depth - 1; ii++) {
              sbEventDataFromParms.append("/" + path[ii].trim());
              sbEventData.append("/" + path[ii].trim());
          }
        }
        sbEventDataFromParms.append("]\n");
        sbEventData.append("]\n");
        sbEventDataFromParms.append( (String) parameters.get("Event") + "\n");
        addEventParmsToDat( (String) parameters.get("Event"));
      }
      
    } // *** end bracket uses for scope separation only

    if (children == null) {
      // Either dir does not exist or is not a directory
    }
    else {
      // first check if there is a multi node directory and generate subtree
      for (int ii = 0; ii < children.length; ii++) {
        if (children[ii].isDirectory() &&
            (children[ii].getName().charAt(0) == '[')) {
          String filename = children[ii].getAbsolutePath();
          MDFNode tmpMDF = new MDFNode();

          if (analyzeDir(filename, tmpMDF, metaData, noMetaDataGen,
                         parmNoMetaData, foundMultiNode) && !foundMultiNode) {
            mdf.addChild(children[ii].getName(), tmpMDF);
            totalMDFBytes += 4;
          }

          noMetaDataGen = true;
          foundMultiNode = true;

          System.out.println("Do not generate meta-data for sibling of : " +
                             nodepath);
        }
      }
      // if data branch of a multinode, then do not generate meta data
      for (int ii = 0; ii < children.length; ii++) {
        if (children[ii].isDirectory() &&
            (children[ii].getName().charAt(0) != '[')) {
          String filename = children[ii].getAbsolutePath();
          // If it's not the parm file, call the method recursively
          MDFNode tmpMDF = new MDFNode();

          if (analyzeDir(filename, tmpMDF, metaData, noMetaDataGen,
                         parmNoMetaData, foundMultiNode) && !foundMultiNode) {
            mdf.addChild(children[ii].getName(), tmpMDF);
            totalMDFBytes += 4;
          }
        }
      }

      if (!parmNoMetaData) {
        noMetaDataGen = false;
      }
    }

    if (metaData && !MDFconversion) {
      dataOut.print(nodepath, indent + "</Node>\n");
    }

    if (generateDDF && bAddDDFCalled) {
      ddfgen.nodeDelim(indent);

    }
    depth -= 1;

    if (!noMetaDataGen) {
      return true;
    }
    else {
      return false;
    }

  }

  //converts acl parms to dat and create/update  acl tables
  private void addAclParmsToDat(String parms) throws Exception {
    String strOnePerm, action, names, name;
    StringTokenizer stAnd, stEqual, stPlus;
    stAnd = new StringTokenizer(parms, "&");
    StringBuffer sb = new StringBuffer();

    while (stAnd.hasMoreTokens()) {
      strOnePerm = stAnd.nextToken();
      if (strOnePerm == null) {
        throw new Exception("ERROR: Wrong acl parameters (parsing \"&\").");
      }
      stEqual = new StringTokenizer(strOnePerm, "=");
      if (stEqual.countTokens() != 2) {
        throw new Exception("ERROR: Wrong acl parameters (parsing \"=\").");
      }
      action = stEqual.nextToken();
      names = stEqual.nextToken();
      if (action == null || names == null) {
        throw new Exception(
            "ERROR: Wrong acl parameters: action or names cannot be null.");
      }

      if (! (action.equals("Add") ||
             action.equals("Get") ||
             action.equals("Delete") ||
             action.equals("Replace") ||
             action.equals("Exec"))) {
        throw new Exception("ERROR: Not supported acl action: " + action);
      }
      action = action.substring(0, 1); // take first letter.

      if (sb.length() > 0) {
        sb.append("&");
      }
      sb.append(action + "=");

      stPlus = new StringTokenizer(names, "+");
      String s = "";
      while (stPlus.hasMoreTokens()) {
        name = stPlus.nextToken();
        if (name == null) {
          throw new Exception("ERROR: Wrong acl parameters (parsing \"+\").");
        }
        s += (s.length() > 0) ? "+" : "";
        s += getTabIndexFromArray(arrAclData, name);
      }
      sb.append(s);
    }
    sbAclData.append(sb.toString() + "\n");
  }
  
  //get index from mapping or add new name (for acl and event) to create data dictionary for .dat file
  private static int getTabIndexFromArray(ArrayList array, String name) {
    for (int i = 0; i < array.size(); i++) {
      if (name.equals( (String) array.get(i))) {
        return i + 1;
      }
    }
    array.add(name);
    return array.size();
  }

  //convert acl and event mapping to string (for data dictionary for .dat file)
  private static String dictionaryToString(ArrayList array) {
    StringBuffer sb = new StringBuffer();
    for (int i = 0; i < array.size(); i++) {
      sb.append( (i + 1) + ":" + (String) array.get(i) + "\n");
    }
    return sb.toString();
  }

  // write acl.dat file
  private static void writeAclDatFile() throws Exception {
    String maps = dictionaryToString(arrAclData);
    if (maps.length() == 0 && sbAclData.length() == 0) {
      return;
    }
    else if ( (maps.length() == 0 && sbAclData.length() > 0) ||
             (maps.length() > 0 && sbAclData.length() == 0)) {
      throw new Exception("Error: wrong acl index tab size or data length.");
    }
    PrintWriter aclOutDat = new PrintWriter(new BufferedWriter(Util.openUtf8FileWriter(
            aclFileDat)));
    aclOutDat.print(maps + "\n");
    aclOutDat.print(sbAclData.toString() + "\n");
    aclOutDat.close();
    aclOutDat = null;
  }
  
  
  //converts event parms to dat and create/update  event tables
  private void addEventParmsToDat(String parms) throws Exception {
      String strEvent, isEventKey, strAbbrev, paramKey, paramVals;
    StringTokenizer stAnd, stEqual, stPlus;
    
    stAnd = new StringTokenizer(parms, "&");
    StringBuffer sb = new StringBuffer();

    while (stAnd.hasMoreTokens()) {
      strEvent = stAnd.nextToken();
      if (strEvent == null) {
        continue;
      }
      HashMap mapResult = getEventAbbreviation(strEvent);
      if(mapResult.get("ERROR") != null){
          throw new Exception("ERROR: Unsupported name for the parameter event.");
      }
      isEventKey = (String)mapResult.get("ISKEY");
      strAbbrev = (String)mapResult.get("ABBREV");
      
      if (isEventKey.equals("FALSE")){  // for operations and formats
          if (sb.length() > 0) {
              sb.append("&");
          }
          sb.append(strAbbrev);    
      }else{  // for parameters (key=values pairs)
          stEqual = new StringTokenizer(strEvent, "=");
        if (stEqual.countTokens() != 2) {
          throw new Exception("ERROR: Wrong event parameters (parsing \"=\").");
        }
        paramKey = stEqual.nextToken(); //we already have Abbreviation for this key
        paramVals = stEqual.nextToken();
        
        if (paramVals == null || paramVals.length() == 0) {
            throw new Exception("ERROR: Wrong event parameters: values cannot be empty string.");           
        }
        if (sb.length() > 0) {
            sb.append("&");
        }
        sb.append(strAbbrev + "=");
        if("Topic".equals(paramKey)){
            sb.append(paramVals);
        }       
        else{           
            stPlus = new StringTokenizer(paramVals, "+");
            String s = "";
            String val;
            while (stPlus.hasMoreTokens()) {
                val = stPlus.nextToken();
                if (val == null) {
                  throw new Exception("ERROR: Wrong event parameters value (parsing \"+\").");
                }
                s += (s.length() > 0) ? "+" : "";
                s += getTabIndexFromArray(arrEventData, val);
            }
            sb.append(s);       
        }  
      }
    }// while loop
    sbEventData.append(sb.toString() + "\n");    
  }
  
  //get abbriviation for an Event operation, format and keys for Parameters
  private final HashMap getEventAbbreviation(String eventName){
      HashMap mapResult = new HashMap(3);
      if(eventName.equals("Add")){
          mapResult.put("ISKEY", "FALSE");
          mapResult.put("ABBREV", "A");
      }
      else if(eventName.equals("Replace")){
          mapResult.put("ISKEY", "FALSE");
          mapResult.put("ABBREV", "R");          
      }
      else if(eventName.equals("Delete")){
          mapResult.put("ISKEY", "FALSE");
          mapResult.put("ABBREV", "D"); 
      }
      else if(eventName.equals("Indirect")){
          mapResult.put("ISKEY", "FALSE");
          mapResult.put("ABBREV", "I"); 
      }
      else if(eventName.equals("Node")){
          mapResult.put("ISKEY", "FALSE");
          mapResult.put("ABBREV", "N"); 
      }
      else if(eventName.equals("Cumulative")){
          mapResult.put("ISKEY", "FALSE");
          mapResult.put("ABBREV", "C"); 
      }
      else if(eventName.equals("Detail")){
          mapResult.put("ISKEY", "FALSE");
          mapResult.put("ABBREV", "F"); 
      }
      else if(eventName.startsWith("Topic=")){
          mapResult.put("ISKEY", "TRUE");
          mapResult.put("ABBREV", "T"); 
      }
      else if(eventName.startsWith("Notify")){
          mapResult.put("ISKEY", "TRUE");
          mapResult.put("ABBREV", "S");
      }
      else if(eventName.startsWith("Ignore")){
          mapResult.put("ISKEY", "TRUE");
          mapResult.put("ABBREV", "P");
      }
      else{
          mapResult.put("ERROR", "ERROR");
      }
      return mapResult;
  }
  
  // write event.dat file
  private static void writeEventDatFile() throws Exception {
    String maps = dictionaryToString(arrEventData);
    if (maps.length() == 0 && sbEventData.length() == 0) {
      return;
    }
    else if (maps.length() > 0 && sbEventData.length() == 0) {
      throw new Exception("Error: wrong event index tab size or data length.");
    }
    PrintWriter eventOutDat = new PrintWriter(new BufferedWriter(Util.openUtf8FileWriter(
            eventFileDat)));
    eventOutDat.print(maps + "\n");
    eventOutDat.print(sbEventData.toString() + "\n");
    eventOutDat.close();
    eventOutDat = null;
  }
  
  private void processNodeAndConstraintOffsets(MDFNode mdfNode) {
    // Setup this node's offset value (account for file size bytes (4) and version bytes (2)
    // at beginning of file)

    mdfNode.nodeOffset = nodeConstraintBytes + 6;

    // Add total size of this node to running total

    nodeConstraintBytes += (11 + (mdfNode.children.size() * 4));

    // Account for extra four bytes of offset to meta-node ID (if required)

    if ( (mdfNode.nodeType & MDFNode.USE_NODE_ID) == MDFNode.USE_NODE_ID) {
      nodeConstraintBytes += 4;
    }

    // Check to see if we have constraints

    if (mdfNode.constraints.size() > 0) {
      Iterator tmpConstraints = mdfNode.constraints.iterator();

      while (tmpConstraints.hasNext()) {
        MDFConstraint tmpConstraint = (MDFConstraint) tmpConstraints.next();

        switch (tmpConstraint.constraintType) {
          case MDFConstraint.AUTONODE_CONSTRAINT:
          case MDFConstraint.DELETE_CHILD_CONSTRAINT:
          case MDFConstraint.DEPEND_CHILD_CONSTRAINT:
          case MDFConstraint.FOREIGN_KEY_CONSTRAINT:
          case MDFConstraint.NAME_REGEXP_CONSTRAINT:
          case MDFConstraint.NAME_VALUES_CONSTRAINT:
          case MDFConstraint.RECUR_AFTER_CONSTRAINT:
          case MDFConstraint.REGEXP_CONSTRAINT:
          case MDFConstraint.MIN_VALUE_CONSTRAINT:
          case MDFConstraint.MAX_VALUE_CONSTRAINT:
          case MDFConstraint.VALUES_CONSTRAINT:
            nodeConstraintBytes += 5;
            break;

          case MDFConstraint.MIN_LENGTH_CONSTRAINT:
          case MDFConstraint.MAX_LENGTH_CONSTRAINT:
          case MDFConstraint.MAX_CHILD_CONSTRAINT:
          case MDFConstraint.NAME_MAX_LENGTH_CONSTRAINT:
          case MDFConstraint.MAX_RECUR_CONSTRAINT:
            nodeConstraintBytes += 3;

          case MDFConstraint.DEFAULT_VALUE_CONSTRAINT:
            switch (tmpConstraint.defaultValueType) {
              case MDFConstraint.DEFAULT_BYTE_TYPE:
                nodeConstraintBytes += 2;
                break;

              case MDFConstraint.DEFAULT_INT_TYPE:
              case MDFConstraint.DEFAULT_FLOAT_TYPE:
              case MDFConstraint.DEFAULT_CHR_TYPE:
                nodeConstraintBytes += 5;
                break;
            }
            break;
        }
      }
    }

    // recursively call ourselves for each of our children

    if (mdfNode.children.size() > 0) {
      Set childKeys = mdfNode.children.keySet();
      Iterator childIterator = childKeys.iterator();

      while (childIterator.hasNext()) {
        processNodeAndConstraintOffsets( (MDFNode) mdfNode.children.get(
            childIterator.next()));
      }
    }

  }

  private void processMDFStrings(MDFNode mdfNode) {
    // Process the NodeName (add 6 bytes to every string value to account for file size/version bytes)

    if (stringTable.containsKey(mdfNode.nodeName)) {
      mdfNode.nodeNameOffset = ( (Integer) stringTable.get(mdfNode.nodeName)).
          intValue();
    }
    else {
      stringTable.put(mdfNode.nodeName, new Integer(totalMDFBytes + 6));
      revStringTable.put(new Integer(totalMDFBytes + 6), mdfNode.nodeName);
      mdfNode.nodeNameOffset = totalMDFBytes + 6;

      // increment totalMDFBytes by length of the nodename+1 (for \0 character)
      totalMDFBytes += (mdfNode.nodeName.length() + 1);
    }

    // Process the Meta Node ID (if present)

    if ( (mdfNode.nodeType & MDFNode.USE_NODE_ID) == MDFNode.USE_NODE_ID) {
      // Process the meta Node ID

      if (stringTable.containsKey(mdfNode.metaNodeID)) {
        mdfNode.metaNodeIDOffset = ( (Integer) stringTable.get(mdfNode.
            metaNodeID)).intValue();
      }
      else {
        stringTable.put(mdfNode.metaNodeID, new Integer(totalMDFBytes + 6));
        revStringTable.put(new Integer(totalMDFBytes + 6), mdfNode.metaNodeID);
        mdfNode.metaNodeIDOffset = totalMDFBytes + 6;

        // increment totalMDFBytes by length of the nodename+1 (for \0 character)
        totalMDFBytes += (mdfNode.metaNodeID.length() + 1);
      }
    }

    // Process the constraints for str values

    Iterator tmpConstraints = mdfNode.constraints.iterator();

    while (tmpConstraints.hasNext()) {
      MDFConstraint tmpConstraint = (MDFConstraint) tmpConstraints.next();

      switch (tmpConstraint.constraintType) {
        case MDFConstraint.AUTONODE_CONSTRAINT:
        case MDFConstraint.DELETE_CHILD_CONSTRAINT:
        case MDFConstraint.DEPEND_CHILD_CONSTRAINT:
        case MDFConstraint.FOREIGN_KEY_CONSTRAINT:
        case MDFConstraint.NAME_REGEXP_CONSTRAINT:
        case MDFConstraint.NAME_VALUES_CONSTRAINT:
        case MDFConstraint.RECUR_AFTER_CONSTRAINT:
        case MDFConstraint.REGEXP_CONSTRAINT:
        case MDFConstraint.VALUES_CONSTRAINT:
          if (stringTable.containsKey(tmpConstraint.chrData)) {
            tmpConstraint.strDataOffset = ( (Integer) stringTable.get(
                tmpConstraint.chrData)).intValue();
          }
          else {
            stringTable.put(tmpConstraint.chrData,
                            new Integer(totalMDFBytes + 6));
            revStringTable.put(new Integer(totalMDFBytes + 6),
                               tmpConstraint.chrData);
            tmpConstraint.strDataOffset = totalMDFBytes + 6;

            // increment totalMDFBytes by length of the nodename+1 (for \0 character)
            totalMDFBytes += (tmpConstraint.chrData.length() + 1);
          }
          break;

        case MDFConstraint.DEFAULT_VALUE_CONSTRAINT:
          if (tmpConstraint.defaultValueType == MDFConstraint.DEFAULT_CHR_TYPE) {
            if (stringTable.containsKey(tmpConstraint.chrData)) {
              tmpConstraint.strDataOffset = ( (Integer) stringTable.get(
                  tmpConstraint.chrData)).intValue();
            }
            else {
              stringTable.put(tmpConstraint.chrData,
                              new Integer(totalMDFBytes + 6));
              revStringTable.put(new Integer(totalMDFBytes + 6),
                                 tmpConstraint.chrData);
              tmpConstraint.strDataOffset = totalMDFBytes + 6;

              // increment totalMDFBytes by length of the nodename+1 (for \0 character)
              totalMDFBytes += (tmpConstraint.chrData.length() + 1);
            }
          }
          break;
      }
    }

    // recursively call ourselves for each of our children

    if (mdfNode.children.size() > 0) {
      Set childKeys = mdfNode.children.keySet();
      Iterator childIterator = childKeys.iterator();

      while (childIterator.hasNext()) {
        processMDFStrings( (MDFNode) mdfNode.children.get(childIterator.next()));
      }
    }
  }

  private String removeSpaces(String line) {
    int i = 0;

    if ( (i = line.indexOf(' ', i)) >= 0) {
      char[] line2 = line.toCharArray();
      StringBuffer buf = new StringBuffer(line2.length);
      buf.append(line2, 0, i).append("");

      i++;
      int j = i;

      while ( (i = line.indexOf(' ', i)) > 0) {
        buf.append(line2, j, i - j).append("");
        i++;
        j = i;
      }

      buf.append(line2, j, line2.length - j);
      return buf.toString();
    }

    return line;
  }

  //Convert .bmdf, ini, fstab to .c and .h files. Using for P2K
  private static void convertToCAndH() throws Exception {
      
    Node f; //changed to support FlexML, replaced File
    Node[] files; //changed to support FlexML, replaced File[]
    File2C convertor = new File2C();
    //convert fstab file
    convertor.convert(fstabFile, true);
    //convert ini files
    f = NodeLoader.getInstance(iniPath); //changed to support FlexML, replaced File()
    if (!f.exists()) {
      throw new Exception("File not exist: " + iniPath);
    }
    else if (f.isFile()) {
      convertor.convert(f.getAbsolutePath(), true);
    }
    else { //directory
      String path;
      files = f.listNodes(); //changed to support FlexML, replaced listFiles ()
      for (int i = 0; i < files.length; i++) {
        path = files[i].getAbsolutePath();
        if (path.endsWith("ini")) {
          convertor.convert(path, true);
        }
      }
    }
    //convert bmdf files
    f = NodeLoader.getInstance("."); //changed to support FlexML, replaced File()
    String path;
    files = f.listNodes(); //changed to support FlexML, replaced listFiles ()
    for (int i = 0; i < files.length; i++) {
      path = files[i].getAbsolutePath();
      if (path.endsWith("bmdf")) {
        convertor.convert(path, false);
      }
    }
  }

  private void getParms(String fileName,
                        HashMap parameters,
                        ArrayList access,
                        MDFNode mdf,
                        boolean notMultiNode,
                        boolean noMetaData) throws Exception {
      
    Reader reader = NodeLoader.getReader(fileName);
    BufferedReader in = new BufferedReader(reader); //changed to support FlexML, replaced File()
    String line;
    byte numConstraints = 0;
    int constraintBytes = 0;
    boolean noMetaGen = false;

    try {
      while ( (line = in.readLine()) != null) {
        if (line.trim().length() == 0 || line.trim().startsWith("#")) {
          continue;
        }
        int nameBoundary = line.indexOf(":");
        String parmName = line.substring(0, nameBoundary);
        String parmValue = line.substring(nameBoundary + 1);

        if (parmName.equalsIgnoreCase("type")) {
          //YXU
          if (parmValue.equalsIgnoreCase("boolean")) {
            parmValue = "bool";
          }
          parameters.put("Type", parmValue.trim());

          if (mdf != null) {
            if (parmValue.trim().equalsIgnoreCase("int")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.INT_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.INT_MN_TYPE;
              }
            }

            if (parmValue.trim().equalsIgnoreCase("bool")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.BOOL_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.BOOL_MN_TYPE;
              }
            }

            if (parmValue.trim().equalsIgnoreCase("bin")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.BIN_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.BIN_MN_TYPE;
              }
            }

            if (parmValue.trim().equalsIgnoreCase("node")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.NODE_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.NODE_MN_TYPE;
              }
            }

            if (parmValue.trim().equalsIgnoreCase("chr")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.CHR_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.CHR_MN_TYPE;
              }
            }

            if (parmValue.trim().equalsIgnoreCase("null")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.NULL_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.NULL_MN_TYPE;
              }
            }

            if (parmValue.trim().equalsIgnoreCase("test")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.TEST_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.TEST_MN_TYPE;
              }
            }

            if (parmValue.trim().equalsIgnoreCase("date")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.DATE_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.DATE_MN_TYPE;
              }
            }

            if (parmValue.trim().equalsIgnoreCase("time")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.TIME_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.TIME_MN_TYPE;
              }
            }

            if (parmValue.trim().equalsIgnoreCase("float")) {
              if (notMultiNode) {
                mdf.nodeType = MDFNode.FLOAT_TYPE;
              }
              else {
                mdf.nodeType = MDFNode.FLOAT_MN_TYPE;
              }
            }
          }
        }

        if (parmName.equalsIgnoreCase("storesPD")) {
          parameters.put("storesPD", parmValue.trim());

          if (mdf != null) {
            mdf.nodeType |= MDFNode.STORES_PD;
          }
        }

        if (parmName.equalsIgnoreCase("HandledByPlugin")) {
          parameters.put("HandledByPlugin", parmValue.trim());

          if (mdf != null) {
            mdf.nodeType |= MDFNode.HANDLE_BY_PLUGIN;
          }
        }

        if (parmName.equalsIgnoreCase("store")) {
          String pv = parmValue.trim();
          parameters.put("store", pv);

          if (mdf != null &&
              (pv.equalsIgnoreCase("true") || pv.equalsIgnoreCase("1"))) {
            mdf.nodeType |= MDFNode.LOB_STORE;
          }
        }

        if (parmName.equalsIgnoreCase("LOBProgressBAR")) {
          String pv = parmValue.trim();
          parameters.put("LOBProgressBAR", pv);

          if (mdf != null &&
              (pv.equalsIgnoreCase("true") || pv.equalsIgnoreCase("1"))) {
            mdf.nodeType |= MDFNode.LOB_PROGRESS_BAR;
          }
        }

        if (parmName.equalsIgnoreCase("ID")) {
          parameters.put("ID", parmValue.trim());

          //     Adjust the totalMDFBytes by 4 for the offset

          totalMDFBytes += 4;

          if (mdf != null) {
            mdf.nodeType |= MDFNode.USE_NODE_ID;
            mdf.metaNodeID = parmValue.trim();
          }
        }

        if (parmName.equalsIgnoreCase("minLen")) {
          parameters.put("minLen", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 3;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.MIN_LENGTH_CONSTRAINT;
            tmpConstraint.shortData = Short.parseShort(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("maxLen")) {
          parameters.put("maxLen", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 3;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.MAX_LENGTH_CONSTRAINT;
            tmpConstraint.shortData = Short.parseShort(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("maxChild")) {
          parameters.put("maxChild", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 3;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.MAX_CHILD_CONSTRAINT;
            tmpConstraint.shortData = Short.parseShort(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("min")) {
          parameters.put("min", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.MIN_VALUE_CONSTRAINT;
            tmpConstraint.intData = Integer.parseInt(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("max")) {
          parameters.put("max", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.MAX_VALUE_CONSTRAINT;
            tmpConstraint.intData = Integer.parseInt(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("values")) {
          parameters.put("values", trimSepSpaces(parmValue));

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.VALUES_CONSTRAINT;
            tmpConstraint.chrData = trimSepSpaces(parmValue);
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("regexp")) {
          parameters.put("regexp", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.REGEXP_CONSTRAINT;
            tmpConstraint.chrData = parmValue.trim();
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("nMaxLen")) {
          parameters.put("nMaxLen", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 3;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.
                NAME_MAX_LENGTH_CONSTRAINT;
            tmpConstraint.shortData = Short.parseShort(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("nValues")) {
          parameters.put("nValues", removeSpaces(parmValue.trim()));

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.NAME_VALUES_CONSTRAINT;
            tmpConstraint.chrData = removeSpaces(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("nRegexp")) {
          parameters.put("nRegexp", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.NAME_REGEXP_CONSTRAINT;
            tmpConstraint.chrData = parmValue.trim();
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("auto")) {
          parameters.put("auto", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.AUTONODE_CONSTRAINT;
            tmpConstraint.chrData = parmValue.trim();
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("fk")) {
          parameters.put("fk", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.FOREIGN_KEY_CONSTRAINT;
            tmpConstraint.chrData = parmValue.trim();
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("child")) {
          parameters.put("child", removeSpaces(parmValue.trim()));

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.
                DELETE_CHILD_CONSTRAINT;
            tmpConstraint.chrData = removeSpaces(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("depend")) {
          parameters.put("depend", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.
                DEPEND_CHILD_CONSTRAINT;
            tmpConstraint.chrData = removeSpaces(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("recur-after-segment")) {
          parameters.put("recur-after-segment", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 5;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.RECUR_AFTER_CONSTRAINT;
            tmpConstraint.chrData = parmValue.trim();
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("max-recurrence")) {
          parameters.put("max-recurrence", parmValue.trim());

          if (mdf != null) {
            numConstraints++;
            constraintBytes += 3;
            MDFConstraint tmpConstraint = new MDFConstraint();
            tmpConstraint.constraintType = MDFConstraint.MAX_RECUR_CONSTRAINT;
            tmpConstraint.shortData = Short.parseShort(parmValue.trim());
            mdf.addConstraint(tmpConstraint);
          }
        }

        if (parmName.equalsIgnoreCase("acl")) {
          //YXU
          parameters.put("Acl", parmValue.trim());
          // ACL does not go into MDF
          continue;
        }
        
        if (parmName.equalsIgnoreCase("event")) {
            parameters.put("Event", parmValue.trim());
            // Event does not go into MDF
            continue;
        }

        //YXU either default or Value cause --> Value
        if (parmName.equalsIgnoreCase("default")) {
          parameters.put("Default", parmValue.trim());
          // later, after this while loop, we are checking and add "default" constraint
          continue;
        }

        if (parmName.equalsIgnoreCase("value")) {
            //check if it is a factory bootstrap then HEX encode it.
            factoryBootstrapEncodeValue = FactBootEnc.checkForBootstrapValue(fileName);           
            
            if(factoryBootstrapEncodeValue == null){
                parameters.put("Value", parmValue.trim());
            }
            else{
                parameters.put("Value", factoryBootstrapEncodeValue);
            }
            
            continue; //Do not send to meta file
        }

        if (parmName.equalsIgnoreCase("mime")) {
          parameters.put("Mime", parmValue.trim());

          if (mdf != null) {
            if (parmValue.trim().equalsIgnoreCase("text/plain")) {
              mdf.mimeType = MDFNode.TEXT_PLAIN;
            }
          }
        }

        if (parmName.equalsIgnoreCase("access")) {
          StringTokenizer tok = new StringTokenizer(parmValue, ",");
          ArrayList tmpAccess = new ArrayList();

          while (tok.hasMoreTokens()) {
            String accessType = tok.nextToken().trim();
            tmpAccess.add(accessType);

            if (mdf != null) {
              if (accessType.equalsIgnoreCase("Add")) {
                mdf.accessType |= MDFNode.ADD_ACCESS;
              }
              else if (accessType.equalsIgnoreCase("Delete")) {
                mdf.accessType |= MDFNode.DELETE_ACCESS;
              }
              else if (accessType.equalsIgnoreCase("Get")) {
                mdf.accessType |= MDFNode.GET_ACCESS;
              }
              else if (accessType.equalsIgnoreCase("Replace")) {
                mdf.accessType |= MDFNode.REPLACE_ACCESS;
              }
              else if (accessType.equalsIgnoreCase("Exec")) {
                mdf.accessType |= MDFNode.EXEC_ACCESS;
              }
              else if (accessType.equalsIgnoreCase("Local")) {
                mdf.accessType |= MDFNode.LOCAL_ACCESS;
              }
            }
          }
          parameters.put("AccessType", tmpAccess);
        }

        if (parmName.equalsIgnoreCase("nodatagen")) {
          parameters.put("DataGen", new Boolean(false));
          continue;
        }
        if (parmName.equalsIgnoreCase("nometagen")) {
          parameters.put("MetaGen", new Boolean(false));
          noMetaGen = true;
          continue;
        }

        if (parmName.equalsIgnoreCase("description")) {
          String docstr = parmValue;
          while ( (line = in.readLine()) != null) {
            docstr += line;
          }
          parameters.put("Description", docstr);
          continue;
        }

        //write to meta data buffer
        if (!noMetaData) {
          metaDataBuffer.print(line + "\n");
        }
        else {
          System.out.println("Didn't print: " + line + " for node: " +
                             mdf.nodeName);
        }
      } // while loop
    }
    catch (Exception e) {
      throw e;
    }
    finally {
      try {
        in.close();
      }
      catch (Exception e) {}
      
      try {
        reader.close();
      }
      catch (Exception e) {}

      in = null;
      reader = null;
    }

    // Add "default" constraint. It should be separate since it is depended on a node type.
    // We have to change it by parsing parm.txt first into map and checking all constraint after it...
    String defParmVal = (String) parameters.get("Default"); // value from parm.txt for "default:..."
    boolean isAddConstraintReq = true; // flag to add/ignore "default" constraint
    if (mdf != null && defParmVal != null) {
      MDFConstraint tmpConstraint = new MDFConstraint();
      tmpConstraint.constraintType = MDFConstraint.DEFAULT_VALUE_CONSTRAINT;
      switch (mdf.nodeType) {
        case MDFNode.INT_MN_TYPE:
        case MDFNode.INT_TYPE:
          tmpConstraint.intData = Integer.parseInt(defParmVal);
          tmpConstraint.defaultValueType = MDFConstraint.DEFAULT_INT_TYPE;
          constraintBytes += 5;
          break;

        case MDFNode.FLOAT_MN_TYPE:
        case MDFNode.FLOAT_TYPE:
          tmpConstraint.floatData = Float.parseFloat(defParmVal);
          tmpConstraint.defaultValueType = MDFConstraint.DEFAULT_FLOAT_TYPE;
          constraintBytes += 5;
          break;

        case MDFNode.BOOL_MN_TYPE:
        case MDFNode.BOOL_TYPE:
          tmpConstraint.byteData = (defParmVal.equals("true") ? (byte) 1 :
                                    (byte) 0);
          tmpConstraint.defaultValueType = MDFConstraint.DEFAULT_BYTE_TYPE;
          constraintBytes += 2;
          break;

        case MDFNode.DATE_MN_TYPE:
        case MDFNode.DATE_TYPE:
        case MDFNode.TIME_MN_TYPE:
        case MDFNode.TIME_TYPE:
        case MDFNode.CHR_MN_TYPE:
        case MDFNode.CHR_TYPE:
          tmpConstraint.chrData = defParmVal;
          tmpConstraint.defaultValueType = MDFConstraint.DEFAULT_CHR_TYPE;
          constraintBytes += 5;
          break;

        case MDFNode.BIN_TYPE:
        case MDFNode.BIN_MN_TYPE:

          // Currently default constraint for node with type "bin" is not supported by tool and by Engine.
          isAddConstraintReq = false;
          break;

        default:
          throw new Exception("Unsupported MDF node type found: " + mdf.nodeType);
          //System.err.println(
           //   "Warning!!! Unsupported node type found and ignored : " +
           //   mdf.nodeType);
         // isAddConstraintReq = false;
      }

      if (isAddConstraintReq) {
        numConstraints++;
        mdf.addConstraint(tmpConstraint);
      }

      if (!noMetaData && isAddConstraintReq && defParmVal != null && mdf != null) {
        metaDataBuffer.print("default:" + defParmVal + "\n");
      }
      else {
        System.out.println("Didn't print: " + "default:" + defParmVal +
                           " for node: " + mdf.nodeName);
      }
    }

    if (mdf != null) {
      mdf.numConstraints = numConstraints;
      if (numConstraints > 0 && !noMetaGen && !noMetaData) {
        totalMDFBytes += constraintBytes;
      }
    }
  }

  private static String trimSepSpaces(String parmValue) throws Exception {
    String sep = ",";
    StringTokenizer tok = new StringTokenizer(parmValue.trim(), sep);
    StringBuffer sb = new StringBuffer();
    String tmp;
    while (tok.hasMoreTokens()) {
      tmp = tok.nextToken().trim();
      if (tmp != null && tmp.length() > 0) {
        if (sb.length() > 0) {
          tmp = sep + tmp;
        }
        sb.append(tmp);
      }
    }
    return sb.toString();
  }
}
