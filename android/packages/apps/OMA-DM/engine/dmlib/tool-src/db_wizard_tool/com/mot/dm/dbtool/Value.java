package com.mot.dm.dbtool;

public class Value {
  public String val;
  public int pos;

  public Value(){
    val = null;
    pos = -1;
  }

  public Value(String val){
    this.val = val;
  }

  public Value(int  pos){
    this.pos = pos;
  }

  public Value(String val, int  pos){
    this.val = val;
    this.pos = pos;
  }

  public String toString(){
    return val;
  }

}
