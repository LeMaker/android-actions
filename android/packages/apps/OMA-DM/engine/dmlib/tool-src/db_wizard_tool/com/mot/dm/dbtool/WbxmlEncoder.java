package com.mot.dm.dbtool;

import java.io.*;
import java.util.*;

import javax.xml.parsers.*;
import org.w3c.dom.*;
import org.xml.sax.InputSource;

public class WbxmlEncoder {

  public int currentTab = 0x00;
  public ArrayList arrBytes = new ArrayList();

  public WbxmlEncoder() {
    arrBytes.add(new Byte( (byte) 0x03));
    arrBytes.add(new Byte( (byte) 0x0b));
    arrBytes.add(new Byte( (byte) 0x6a));
    arrBytes.add(new Byte( (byte) 0x00));
  }

  public byte[] encode(File fileXml) throws Exception {
    FileInputStream streamer = new FileInputStream(fileXml);
    byte[] byteArray = new byte[streamer.available()];
    streamer.read(byteArray);
    streamer.close();
    streamer=null;
    return encode(new String(byteArray));
  }

  public byte[] encode(String xml) throws Exception {
    NodeList list;
    Document document = getDocument("<root>\n" + xml + "\n</root>");
    list = document.getDocumentElement().getChildNodes();

    for (int i = 0; i < list.getLength(); i++) {
      Node node = list.item(i);
      if (node.getNodeType() == Node.ELEMENT_NODE) {
        proceedNode(node);
      }
    }
    return listToByteArr();
  }

  public void proceedNode(Node node) throws Exception {
    String nodeName = node.getNodeName();
    NamedNodeMap attributes = node.getAttributes();
    NodeList list = node.getChildNodes();

    // proceed tag.
    proceedTag(nodeName, node.hasAttributes(), node.hasChildNodes());

    //proceed all attributes
    for (int i = 0; i < attributes.getLength(); i++) {
      Node attribute = attributes.item(i);
      proceedAttribute(attribute.getNodeName(), attribute.getNodeValue());
    }
    // close tag after all attributes if required...
    if (node.hasAttributes()) {
      arrBytes.add(new Byte( (byte) 0x01));
    }

    //procced all children
    Node n;
    for (int i = 0; i < list.getLength(); i++) {
      n = list.item(i);
      if (n.getNodeType() == Node.ELEMENT_NODE) {
        proceedNode(list.item(i));
      }
    }
    // close tag after all childrens if required...
    if (node.hasChildNodes()) {
      arrBytes.add(new Byte( (byte) 0x01));
    }
  }

  public void proceedTag(String nodeName, boolean hasAttrs, boolean hasChildren) throws
      Exception {
    int shift = 0;
    if (hasAttrs) {
      shift = shift | 0x80;
    }
    if (hasChildren) {
      shift = shift | 0x40;
    }
    ValTab tag = OMACP.encodeTag(nodeName, shift);
    if (tag.encVal == null) {
      throw new Exception(" Error:  Unsupported tag '" + nodeName +
                          "' has been used.");
    }
    // check for table
    checkTablesSetting(tag);
    //add encoded tag
    arrBytes.add(tag.encVal);
  }

  public void proceedAttribute(String attrName, String attrValue) throws
      Exception {

    //get encoded "attribute=value" if exists
    ValTab attrVal = OMACP.encodeAttr(attrName + "=" + attrValue);
    if (attrVal.encVal != null) {
      checkTablesSetting(attrVal);
      //add encoded attribute and value
      arrBytes.add(attrVal.encVal);
      return;
    }

    //get encoded attribute
    ValTab attrN = OMACP.encodeAttr(attrName);
    if (attrN.encVal == null) {
      throw new Exception(" Error:  Unsupported attribute '" + attrName +
                          "' has been used.");
    }
    checkTablesSetting(attrN);
    arrBytes.add(attrN.encVal);

    //get encoded value if exists
    ValTab attrV = OMACP.encodeVal(attrValue);
    if (attrV.encVal != null) {
      checkTablesSetting(attrV);
      arrBytes.add(attrV.encVal);
      return;
    }
    else {
      // add string value
      arrBytes.add(new Byte( (byte) 0x03));
      //convert string to bytes
      byte[] b = attrValue.getBytes();
      for (int i = 0; i < b.length; i++) {
        arrBytes.add(new Byte(b[i]));
      }
      //end string
      arrBytes.add(new Byte( (byte) 0x00));
    }

  }

  public void checkTablesSetting(ValTab tag) {
    if (tag.table != currentTab) {
      arrBytes.add(new Byte( (byte) 0x00));
      arrBytes.add(new Byte( (byte) tag.table));
      currentTab = tag.table;
    }
  }

  private Document getDocument(String xml) throws Exception {
    DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
    factory.setValidating(false);
    byte[] utf8_bytes = xml.getBytes("UTF-8");
    return factory.newDocumentBuilder().parse(new InputSource(new
        ByteArrayInputStream(utf8_bytes)));
  }

  public byte[] listToByteArr() {
    Byte b;
    byte[] wbxml = new byte[arrBytes.size()];
    for (int i = 0; i < arrBytes.size(); i++) {
      b = (Byte) arrBytes.get(i);
      wbxml[i] = b.byteValue();
    }
    return wbxml;
  }

  public static void main(String[] args) throws Exception {
    FileInputStream fs = new FileInputStream("WAP.xml");
    byte[] arr = new byte[fs.available()];
    fs.read(arr);
    String s = new String(arr);

    WbxmlEncoder encoder = new WbxmlEncoder();
    byte[] wbxml = encoder.encode(s);
    FileOutputStream fos = new FileOutputStream("WAP.wbxml");
    fos.write(wbxml);
    fos.flush();
  }
}
