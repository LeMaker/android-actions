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
// Module Name: FactBootEnc
//
// General Description: Factory bootstrap encoder. Converts special bytes from parm.txt to their HEX 
//                      presentation for the factory bootstrap.
//
//==================================================================================================

package com.mot.dm.core;

import java.io.*;
import com.mot.dm.io.Node;
import com.mot.dm.io.NodeLoader;
import com.mot.dm.tool.Util;
     

public class FactBootEnc {
  //GUID
  public static final String GUID_HEX_BOOTSTRAP =
      "Factory_bootstrap_settings_encoding_HEX:";

  public static final String SEP = File.separator;

  // HEX encoding
  public static final String DMACC_FACTORY_BOOTSTRAP_CLIENTPW_HEX =
      "EBE8EFEEECEC";
  public static final String DMACC_FACTORY_BOOTSTRAP_SERVERPW_HEX =
      "FCE9E2E4E0E0";
  public static final String DMACC_FACTORY_BOOTSTRAP_USERNAME_HEX =
      "E0E5E7EAEBEB";

  //for DM 1.1.2    ./SyncML/DMAcc/[]/....
  public static String BOOTSTRAP_ROOT_112 = SEP.concat("Dmt").concat(SEP).concat("SyncML").concat(SEP).concat("DMAcc").concat(SEP);
  public static String CLIENTPW_NODE_112 = "ClientPW".concat(SEP).concat("parm.txt");
  public static String SERVERPW_NODE_112 = "ServerPW".concat(SEP).concat("parm.txt");
  public static String USERNAME_NODE_112 = "UserName".concat(SEP).concat("parm.txt");
  
  //for DM 1.2    ./DMAcc/[]/AppAuth/[]/....
  public static String BOOTSTRAP_ROOT_12 = SEP.concat("Dmt").concat(SEP).concat("DMAcc").concat(SEP);
  public static String BOOTSTRAP_MID_12 = "AppAuth".concat(SEP);
  public static String CLIENTPW_NODE_12 = "AAuthSecret".concat(SEP).concat("parm.txt");
  public static String SERVERPW_NODE_12 = "AAuthSecret".concat(SEP).concat("parm.txt");
  public static String USERNAME_NODE_12 = "AAuthName".concat(SEP).concat("parm.txt");

  //returns GUID_HEX_BOOTSTRAP + HEX factory bootstrap value or null in case if
  //it is  not factory bootstrap
  public static final String checkForBootstrapValue(String path) throws
      Exception {
      
    //for DM 1.1.2
    if ( path.indexOf(BOOTSTRAP_ROOT_112) > -1) {
      if (path.endsWith(CLIENTPW_NODE_112) ||
          path.endsWith(SERVERPW_NODE_112) ||
          path.endsWith(USERNAME_NODE_112)) {
        return hexBootstrapValue(path);
      }
    }
    //for DM 1.2
    else if (path.indexOf(BOOTSTRAP_ROOT_12)  > -1 &&
                path.indexOf(BOOTSTRAP_MID_12) > -1) {   
      if (path.endsWith(CLIENTPW_NODE_12) ||
          path.endsWith(SERVERPW_NODE_12) ||
          path.endsWith(USERNAME_NODE_12)) {
        return hexBootstrapValue(path);
      }
    }
   
    return null;
  }

  //returns GUID_HEX_BOOTSTRAP + HEX factory bootstrap value or null in case if
  //it is  not a factory bootstrap
  public static final String hexBootstrapValue(String path) throws Exception {
      Reader reader = null;
      BufferedReader br = null;    
      String hexEncodedValue = null;
      try
    {
        Node node = NodeLoader.getInstance(path);
        reader = NodeLoader.getReader(node);
        br = new BufferedReader(reader);
        String line;
        //Read lines from parm.txt
        while ( (line = br.readLine()) != null) {
          line = line.toUpperCase();
                 //check for ClientPW
          if (line.indexOf(DMACC_FACTORY_BOOTSTRAP_CLIENTPW_HEX) != -1) {
            hexEncodedValue = DMACC_FACTORY_BOOTSTRAP_CLIENTPW_HEX;
            break;
          }      //check for ServerPW
          else if (line.indexOf(DMACC_FACTORY_BOOTSTRAP_SERVERPW_HEX) != -1) {
            hexEncodedValue = DMACC_FACTORY_BOOTSTRAP_SERVERPW_HEX;
            break;
          }      //check for UserName
          else if (line.indexOf(DMACC_FACTORY_BOOTSTRAP_USERNAME_HEX) != -1) {
            hexEncodedValue = DMACC_FACTORY_BOOTSTRAP_USERNAME_HEX;
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
    }
    

      
    
    if (hexEncodedValue != null) {
      //factory bootstrap profile found
      hexEncodedValue = GUID_HEX_BOOTSTRAP.concat(hexEncodedValue);
    }
    
    return hexEncodedValue;   
  }

  
  // Test for the functions checkForBootstrapValue() and hexBootstrapValue()
  public void testMe(String path) throws Exception {
    File f = new File(path);
    if (f.isDirectory()) {
      File[] listFiles = f.listFiles();
      for (int i = 0; i < listFiles.length; i++) {
        if (listFiles[i].isDirectory()) {
          testMe(listFiles[i].getAbsolutePath());
        }
        else {
          String val = checkForBootstrapValue(listFiles[i].getAbsolutePath());
          if (val != null) {
            System.out.println("path: " + listFiles[i].getAbsolutePath());
            System.out.println("val:  " + val + "\n");
          }
        }
      }
    }
    else {
      String val = checkForBootstrapValue(f.getAbsolutePath());
      if (val != null) {
        System.out.println("path: " + f.getAbsolutePath());
        System.out.println("val:  " + val + "\n");
      }
    }
  }

  public static void main(String[] args) throws Exception {
    if (args.length != 1) {
      System.out.println("usage:  java FactBootEnc <path>/Dmt");
      System.exit(1);
    }
    FactBootEnc t = new FactBootEnc();
    t.testMe(args[0]);
  }
}
