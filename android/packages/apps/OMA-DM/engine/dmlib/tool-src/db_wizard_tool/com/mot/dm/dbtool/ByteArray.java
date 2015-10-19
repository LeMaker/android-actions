package com.mot.dm.dbtool;

public class ByteArray {
  byte[] bytes = new byte[0];
  public ByteArray() {
  }

  public void addByte(byte b) throws Exception {
    byte[] arr = {
        b};
    addBytes(arr);
  }

  public void addBytes(byte[] b) throws Exception {
    int count = 0;
    byte[] newBytes = new byte[bytes.length + b.length];
    for (int i = 0; i < bytes.length; i++) {
      newBytes[count++] = bytes[i];
    }
    for (int i = 0; i < b.length; i++) {
      newBytes[count++] = b[i];
    }
    bytes = newBytes;
  }

  public byte[] getBytes() throws Exception {
    return bytes;
  }

  public int length() throws Exception {
    return bytes.length;
  }

}
