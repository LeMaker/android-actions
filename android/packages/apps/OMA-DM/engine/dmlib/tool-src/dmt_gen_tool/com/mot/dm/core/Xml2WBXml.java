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
// Module Name: Xml2WBXml
//
// General Description: Convert XML file to WBXML
//
//==================================================================================================

package com.mot.dm.core;

import java.util.*;
import java.io.*;
import javax.xml.parsers.*;
import org.w3c.dom.*;
import org.xml.sax.InputSource;

public class Xml2WBXml {
  HashMap accessMap = null;
  HashMap formatMap = null;
  ArrayList arrResult = new ArrayList();
  String gformat = null;

  public Xml2WBXml() {
    initAccessMap();
    initFormatMap();
    resetArrResult();

  }

  private void resetArrResult() {
    //Emit WBXML header with version number, doc ID, charset, and string table.
    //['\x01\x01\x6A\x00']
    arrResult.clear();
    arrResult.add(new Character( (char) 0x01));
    arrResult.add(new Character( (char) 0x01));
    arrResult.add(new Character( (char) 0x6A));
    arrResult.add(new Character( (char) 0x00));
  }

  //Access Types.
  //TODO: Should add "Local" later...
  private void initAccessMap() {
    accessMap = new HashMap();
    accessMap.put("Add", new Integer(0x01));
    accessMap.put("Delete", new Integer(0x02));
    accessMap.put("Exec", new Integer(0x04));
    accessMap.put("Get", new Integer(0x08));
    accessMap.put("Replace", new Integer(0x10));
  }

  private void initFormatMap() {
    formatMap = new HashMap();
    formatMap.put("bin", new Character( (char) 0));
    formatMap.put("bool", new Character( (char) 1));
    formatMap.put("b64", new Character( (char) 2));
    formatMap.put("chr", new Character( (char) 3));
    formatMap.put("int", new Character( (char) 4));
    formatMap.put("node", new Character( (char) 5));
    formatMap.put("null", new Character( (char) 6));
    formatMap.put("xml", new Character( (char) 7));
    formatMap.put("test", new Character( (char) 9)); 
    formatMap.put("float", new Character( (char) 10));
    formatMap.put("date", new Character( (char) 11));
    formatMap.put("time", new Character( (char) 12));
  }

  private Document getDocument(String filename) throws Exception {
    DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
    factory.setValidating(false);

    // The following if way around (ugly but working) to support chinese chars for DOM parser:
    //   - read  file to string
    //   - get UTF8 byte array
    //   - send this array to xmp parser.
    
    FileInputStream fis = new FileInputStream(filename);
    int x = fis.available();
    byte raw_bytes[] = new byte[x];
    fis.read(raw_bytes);
    String content = new String(raw_bytes);
    raw_bytes = null;
    byte[] utf8_bytes = content.getBytes("UTF8");

    return factory.newDocumentBuilder().parse(new InputSource(new
        ByteArrayInputStream(utf8_bytes)));
  }

  public void convert(File file) throws Exception {
    resetArrResult();

    ArrayList arrNodes = new ArrayList();
    Document document = getDocument(file.getCanonicalPath());
    NodeList list;
    list = document.getDocumentElement().getChildNodes();

    for (int i = 0; i < list.getLength(); i++) {
      Node node = list.item(i);
      if (node.getNodeType() == Node.ELEMENT_NODE &&
          node.getNodeName().equals("Node")) {
        arrNodes.add(node);
      }
    }

    if (arrNodes.size() > 0) {
      this.arrResult.add(new Character( (char) 0x5E));

      for (int i = 0; i < arrNodes.size(); i++) {
        Node node = (Node) arrNodes.get(i);
        arrResult.addAll(processNode(node));
      }
      this.arrResult.add(new Character( (char) 0x01));
    }
    else {
      this.arrResult.add(new Character( (char) 0x1E));
    }

    // write to file wbxml from arrResult
    String wbxmlPath = file.getCanonicalPath();
    wbxmlPath = (wbxmlPath.substring(0, wbxmlPath.length() - 3)).concat(
        "wbxml");
    byte[] arrByte = new byte[arrResult.size()];
    //convert chars to bytes
    for (int i = 0; i < arrResult.size(); i++) {
      char c = ( (Character) arrResult.get(i)).charValue();
      arrByte[i] = (byte) c;
    }
    // write
    FileOutputStream binaryMetaDataOut = new FileOutputStream(wbxmlPath);
    DataOutputStream bmdfOut = new DataOutputStream(binaryMetaDataOut);
    bmdfOut.write(arrByte);
    bmdfOut.close();
  }

  private ArrayList processNode(Node node) throws Exception {
    ArrayList arrCurrResult = new ArrayList();
    NodeList list = node.getChildNodes();
    ArrayList arrNodes = new ArrayList();

    for (int i = 0; i < list.getLength(); i++) {
      Node n = list.item(i);
      if (n.getNodeType() == Node.ELEMENT_NODE) { ///@@@ add node validation here NodeChildren
        arrNodes.add(n);
      }
    }

    if (arrNodes.size() > 0) {
      arrCurrResult.add(new Character( (char) 0x5D));

      for (int i = 0; i < arrNodes.size(); i++) {
        Node n = (Node) arrNodes.get(i);

        //  result.append(NodeChildren[child.tagName](child))
        if (n.getNodeName().equals("Node")) {
          arrCurrResult.addAll(processNode(n));
        }
        else if (n.getNodeName().equals("Type")) {
          arrCurrResult.addAll(processNodeType(n));
        }
        else if (n.getNodeName().equals("Data")) {
          arrCurrResult.addAll(processNodeData(n));
        }
        else if (n.getNodeName().equals("Plural")) {
          arrCurrResult.addAll(processNodePlural(n));
        }
        else if (n.getNodeName().equals("ClassID")) {
          arrCurrResult.addAll(processClassID(n));
        }
        else if (n.getNodeName().equals("NodeName")) {
          arrCurrResult.addAll(processNodeName(n));
        }
        else if (n.getNodeName().equals("Path")) {
          arrCurrResult.addAll(processNodePath(n));
        }
        else if (n.getNodeName().equals("RTProperties")) {
          arrCurrResult.addAll(processRTProperties(n));
        }
        else if (n.getNodeName().equals("DFProperties")) {
          arrCurrResult.addAll(processDFProperties(n));
        }
        else {
          throw new Exception("Unsupported node: " + n.getNodeName());
        }
      }
      arrCurrResult.add(new Character( (char) 0x01));
    }
    else {
      arrCurrResult.add(new Character( (char) 0x1D));
    }
    return arrCurrResult;
  }

  private ArrayList processNodeType(Node node) throws Exception {
    ArrayList arrCurrResult = new ArrayList();
    String text = getText(node);
    arrCurrResult.add(new Character( (char) 0x56));
    arrCurrResult.addAll(opaqueStringData(text));
    arrCurrResult.add(new Character( (char) 0x01));
    return arrCurrResult;
  }

  private ArrayList processNodeData(Node node) throws Exception {
    ArrayList arrCurrResult = new ArrayList();
    String nodeData = getText(node);
    arrCurrResult.add(new Character( (char) 0x48));
    if ("bin".equals(gformat)) {
      arrCurrResult.addAll(opaqueHexData(nodeData));
    }
    else {
      arrCurrResult.addAll(opaqueStringData(nodeData));
    }
    arrCurrResult.add(new Character( (char) 0x01));
    return arrCurrResult;
  }

  private ArrayList processNodePlural(Node node) throws Exception {
    ArrayList arrCurrResult = new ArrayList();
    Node nodeYesNo = null;
    NodeList list = node.getChildNodes();

    for (int i = 0; i < list.getLength(); i++) {
      Node n = list.item(i);
      if (n.getNodeType() == Node.ELEMENT_NODE &&
          (n.getNodeName().equals("Yes") || n.getNodeName().equals("No"))) {
        nodeYesNo = n;
        break;
      }
    }

    if (nodeYesNo == null) {
      throw new Exception("Plural element missing Yes or No tag");
    }
    else {
      char yesOrNo = (nodeYesNo.getNodeName().equals("Yes")) ? (char) 1 :
          (char) 0;
      arrCurrResult.add(new Character( (char) 0x49));
      arrCurrResult.addAll(opaqueCharData(yesOrNo));
      arrCurrResult.add(new Character( (char) 0x01));
    }
    return arrCurrResult;
  }

  private ArrayList processClassID(Node node) throws Exception {
    ArrayList arrCurrResult = new ArrayList();
    String strClassId = getText(node);
    int intClassId = Integer.parseInt(strClassId);
    arrCurrResult.add(new Character( (char) (0x18 | 0x40)));
    arrCurrResult.addAll(opaquePackedData(intClassId));
    arrCurrResult.add(new Character( (char) 0x01));
    return arrCurrResult;
  }

  private ArrayList processNodeName(Node node) throws Exception {
    //text = getText(element)
    //element.parentNode.SyncMLDM_URI += '/' + text
    return processTextElement(node, 0x1C);
  }

  private ArrayList processNodePath(Node node) throws Exception {
    return processTextElement(node, 0x12);
  }

  private ArrayList processRTProperties(Node node) throws Exception {
    throw new Exception("Support for RTProperties not implemented !!!");
  }

  private ArrayList processDFProperties(Node node) throws Exception {
    ArrayList arrCurrResult = new ArrayList();

    NodeList list = node.getChildNodes();
    for (int i = 0; i < list.getLength(); i++) {
      Node n = list.item(i);
      if (n.getNodeType() == Node.ELEMENT_NODE) {
        if (n.getNodeName().equals("AccessType")) {
          arrCurrResult.addAll(processAccessType(n));
        }
        else if (n.getNodeName().equals("DFFormat")) {
          arrCurrResult.addAll(processDFFormat(n));
        }
        else if (n.getNodeName().equals("Scope")) {
          arrCurrResult.addAll(processScope(n));
        }
        else {
          throw new Exception("The tag " + n.getNodeName() +
                              " not supprted for DFProperties !!!");
        }
      }
    }
    return arrCurrResult;
  }

  private ArrayList processAccessType(Node node) throws Exception {
    ArrayList arrCurrResult = new ArrayList();
    NodeList list = node.getChildNodes();
    int access = 0;
    int nodeHasGetAccess = 0;
    Integer integer;
    for (int i = 0; i < list.getLength(); i++) {
      Node n = list.item(i);
      if (n.getNodeType() == Node.ELEMENT_NODE) {
        integer = (Integer) accessMap.get(n.getNodeName());
        if (integer != null) {
          access |= integer.intValue();
          if ("Get".equalsIgnoreCase(n.getNodeName())) {
            nodeHasGetAccess = 1;
          }
        }
      }
    }
    arrCurrResult.add(new Character( (char) 0x5A));
    arrCurrResult.addAll(opaquePackedData(access));
    arrCurrResult.add(new Character( (char) 0x01));
    if (nodeHasGetAccess == 0) {
      //result = result + '\x55'+ opaqueData(chr(180))  + '\x01'
      arrCurrResult.add(new Character( (char) 0x55));
      arrCurrResult.addAll(opaqueCharData( (char) 180));
      arrCurrResult.add(new Character( (char) 0x01));
    }
    return arrCurrResult;
  }

  private ArrayList processDFFormat(Node node) throws Exception {
    ArrayList arrCurrResult = new ArrayList();
    Character format = null;
    NodeList list = node.getChildNodes();

    for (int i = 0; i < list.getLength(); i++) {
      Node n = list.item(i);
      if (n.getNodeType() == Node.ELEMENT_NODE) {
        format = (Character) formatMap.get(n.getNodeName());
        if (format != null) {
          gformat = n.getNodeName();
          //WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
          //System.out.println("format: " + gformat + "  value: " + format.charValue() + " opaq: " + opaqueCharData(format.charValue()));
          
          //@@@@@ py ??? gformat = format
          //@@@@@ py ??? element.parentNode.parentNode.SyncMLDM_Format = format;
          break;
        }
      }
    }

    if (format == null) {
      throw new Exception("DFFormat is an unknown or missing format tag");
    }
    else {
      arrCurrResult.add(new Character( (char) 0x57));
      arrCurrResult.addAll(opaqueCharData(format.charValue()));
      arrCurrResult.add(new Character( (char) 0x01));
    }
    return arrCurrResult;
  }

  private ArrayList processScope(Node node) throws Exception {
    ArrayList arrCurrResult = new ArrayList();
    Node scope = null;
    NodeList list = node.getChildNodes();

    for (int i = 0; i < list.getLength(); i++) {
      Node n = list.item(i);
      if (n.getNodeType() == Node.ELEMENT_NODE &&
          (n.getNodeName().equals("Permanent") ||
           n.getNodeName().equals("Dynamic"))) {
        scope = n;
        break;
      }
    }

    if (scope == null) {
      throw new Exception("Scope element missing Permanent or Dynamic tag");
    }
    else {
      char charScope = (scope.getNodeName().equals("Permanent")) ? (char) 1 :
          (char) 2;
      arrCurrResult.add(new Character( (char) 0x59));
      arrCurrResult.addAll(opaqueCharData(charScope));
      arrCurrResult.add(new Character( (char) 0x01));
    }
    return arrCurrResult;
  }

  private String getText(Node node) throws Exception {
    NodeList list = node.getChildNodes();
    for (int i = 0; i < list.getLength(); i++) {
      Node n = list.item(i);
      if (n.getNodeType() == Node.TEXT_NODE) {
        return n.getNodeValue();
      }
    }
    return "";
  }

  public ArrayList opaqueStringData(String data) throws Exception {
      if(data.startsWith(FactBootEnc.GUID_HEX_BOOTSTRAP)){
          return opaqueHexData(data.replaceFirst(FactBootEnc.GUID_HEX_BOOTSTRAP, ""));
      }    
    byte[] utf8_bytes = data.getBytes("UTF8");
    ArrayList arrOpaqueDate = new ArrayList();
    arrOpaqueDate.add(new Character( (char) 0xC3));
    arrOpaqueDate.addAll(multiByte(utf8_bytes.length));
    for (int i = 0; i < utf8_bytes.length; i++) {
      arrOpaqueDate.add(new Character(  (char) (((int)utf8_bytes[i]) & 0xFF)  ) );
    }
    return arrOpaqueDate;

   /* ArrayList arrOpaqueDate = new ArrayList();
    arrOpaqueDate.add(new Character( (char) 0xC3));
    arrOpaqueDate.addAll(multiByte(data.length()));
    char[] arrChar = data.toCharArray();
    for (int i = 0; i < arrChar.length; i++) {
      arrOpaqueDate.add(new Character(arrChar[i]));
    }
    return arrOpaqueDate;*/
  }

  public ArrayList opaqueCharData(char data) throws Exception {
    ArrayList arrOpaqueDate = new ArrayList();
    arrOpaqueDate.add(new Character( (char) 0xC3));
    arrOpaqueDate.addAll(multiByte(1));
    arrOpaqueDate.add(new Character(data));
    return arrOpaqueDate;
  }

  public ArrayList opaquePackedData(int data) throws Exception {
    ArrayList arrOpaqueDate = new ArrayList();
    arrOpaqueDate.add(new Character( (char) 0xC3));
    arrOpaqueDate.addAll(multiByte(2));
    arrOpaqueDate.add(new Character( (char) (data / 256)));
    arrOpaqueDate.add(new Character( (char) (data % 256)));
    return arrOpaqueDate;
  }

  public ArrayList opaqueHexData(String data) throws Exception {
    if (data.length() % 2 != 0) {
      throw new Exception(
          "HEX-encoded data has incorrect format: data length is not even number.");
    }
    ArrayList arrOpaqueDate = new ArrayList();
    arrOpaqueDate.add(new Character( (char) 0xC3));
    arrOpaqueDate.addAll(multiByte(data.length() / 2));
    arrOpaqueDate.addAll(hexToBin(data));
    return arrOpaqueDate;
  }

  public ArrayList hexToBin(String data) {
    ArrayList arrBinDate = new ArrayList();
    int tmp;
    for (int i = 0; i < data.length(); i += 2) {
      tmp = Integer.decode("0x" + data.substring(i, i + 2)).intValue();
      arrBinDate.add(new Character( (char) tmp));
    }
    return arrBinDate;
  }

  public ArrayList multiByte(int value) throws Exception {
    ArrayList result = new ArrayList();
    int continuation = 0;
    int bits = 0;

    for (int shift = 28; shift > 0; shift -= 7) {
      bits = (value >> shift) & 0x7F;
      if (bits > 0 || continuation > 0) {
        result.add(new Character( (char) (bits | 0x80)));
      }
      if (bits > 0) {
        continuation = 1;
      }
    }
    result.add(new Character( (char) (value & 0x7F)));
    return result;
  }

  //Return the WBXML representation of a element with text content
  // Takes a node and the value of the WBXML tag without content.
  public ArrayList processTextElement(Node node, int wbxmlTag) throws Exception {
    ArrayList arrText = new ArrayList();
    String text = getText(node);
    if (text.length() > 0) {
      arrText.add(new Character( (char) (wbxmlTag | 0x40)));
      arrText.addAll(opaqueStringData(text));
      arrText.add(new Character( (char) 0x01));
    }
    else {
      arrText.add(new Character( (char) wbxmlTag));
    }
    return arrText;
  }

  public static void main(String[] args) {
    try {
      Xml2WBXml x2b = new Xml2WBXml();
      File file = new File(args[0]);
      x2b.convert(file);
    }
    catch (Exception e) {
      e.printStackTrace();
    }
  }
}
