package com.mot.dm.dbtool;

public class BlockTable {
  public int byteCount = 0;
  public int bitCount = 0;
  public byte[] data;
  public final int length;

  public BlockTable(int size) {
    data = new byte[size];
    for(int i=0; i< size; i++){
      data[i] = 0;
    }
    this.length = size;
  }

  public void appendData(byte[] newData) throws Exception{
    for(int i=0; i< newData.length; i++){
      this.data[byteCount++] = newData[i];
    }
  }
}
