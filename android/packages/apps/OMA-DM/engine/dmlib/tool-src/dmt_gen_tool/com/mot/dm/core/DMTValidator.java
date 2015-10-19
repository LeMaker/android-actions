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
// Module Name: DMTValidator
//
// General Description: The classes contained here provide a depth first recursive traversal of the
//                       Dmt.zip directory heirarchy, and have numerous sanity checks built in which
//                       validate the contents of the parm.txt files that make up the DMT.
//
//==================================================================================================

package com.mot.dm.core;

import java.io.BufferedReader;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.StringTokenizer;
import java.util.ListIterator;

// Joda is an Open Source date/time parsing library (used to parse ISO8601 date/time formats)
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;
import org.joda.time.DateTime;

// Jakarta regexp is an Open Source regular expression parsing library.  
import org.apache.regexp.RE;
import org.apache.regexp.RESyntaxException;

import java.io.Reader;
import com.mot.dm.io.Node; 
import com.mot.dm.io.NodeLoader;

// Main DMTValidator class
public class DMTValidator 
{    
    private final String[] illegalCharsForNodeName = {"#", "$", "\\", "/", "?", "<", ">", "%", "\"", "|", ":"}; //"&", "!"
    private int depth = 1;
    final static private String parm = "parm.txt";
    static private String initDir = "D:\\Dmt";
    static private ArrayList problemsList = new ArrayList();
    private String [] path = new String [20];
    
    private static void usage() 
    {
      System.err.println("Usage: java DMTValidator -d dirName \n" +
                         "\t\t -d      : dirName points to the directory where Dmt is located\n");
    }
    
    public static void main(String [] args) throws Exception 
    {                      
      if (args.length != 2)
      {
          System.err.println("Error: Incorrect number of arguments!");
        usage();
        System.exit(1);    
      }
      else
      {
          if (args[0].equals("-d"))
          {
            initDir = args[1];
          }
          else
          {
          System.err.println("Error: No directory specified!");
          usage();
          System.exit(1);    
          }
      }
      
      // Check to make sure the directory we are passed is valid
      if (!NodeLoader.getInstance(initDir).exists())
      {
        System.err.println("Error: Invalid directory specified!");
        usage();
        System.exit(1);
      }
    
      // Run the validator on our dir structure
      
      DMTValidator validator = new DMTValidator();
      validator.analyzeDir(initDir);
      
      // Exit with the proper return code, and print out any problems
      
      if (problemsList.isEmpty())
      {
          //System.out.println("DMT validation passed...");
          System.exit(0);
      }
      else
      {
          ListIterator iterator = problemsList.listIterator();
          
          //System.out.println("DM Tree cannot be generated: validation failed!");
          
          while (iterator.hasNext())
        {
            System.err.println("Error: " + iterator.next());
        }
        
          System.exit(1);
      }
    }

    // Starting point for our tree traversal - called recursively to process the tree
    public void analyzeDir(String dirName) throws Exception 
    {
      boolean isLeaf=false;
      boolean isMultiNode = false;
      
      // Increment our depth
          
      depth += 1;
        
      // Remove leading (leaf) and trailing (reserved name) "#"
      
      String nodeName;
      
      if (depth <= 2) 
      {
        nodeName = ".";
      }
      else 
      {
        nodeName = dirName.substring(dirName.lastIndexOf(System.getProperty("file.separator")) + 1);
        
        if (nodeName.startsWith("#")) 
        {
          isLeaf=true;
        }
        
        nodeName = nodeName.replace('#', ' ');
        
        if (nodeName.startsWith("@")) 
        {
          nodeName = nodeName.substring(1);
        }
        
        nodeName = nodeName.trim();
        
        if (nodeName.charAt(0) == '[') 
        {
          nodeName = "*";
          isMultiNode = true;
        }
        path [depth - 2] = nodeName;
      }

      // Get the current node path
      
      String nodepath=".";
      
      for (int i=1; i<depth-1; i++)
      {
        nodepath=nodepath+"/"+path[i];
      }
    
      // Process the directory
      
      Node directory = NodeLoader.getInstance(dirName);
      Node[] children = directory.listNodes();
        
      // Create our parameters HashMap and access ArrayList
      
      HashMap parameters = new HashMap();
      ArrayList access = new ArrayList();
    
      String filename = dirName+"/"+parm;
        
      // Does the parm file exist?  If not, flag it, and continue
      
      if (NodeLoader.getInstance(filename).exists())
      {                    
        // Get parameters from the current branch
      
        try 
        { 
          getParms(filename, parameters, access);
        } 
        catch (Exception e)
        {
            e.printStackTrace();
          problemsList.add("Malformed parm.txt file for node: " + nodepath);
        }
        
        // DMT VALIDATIONS GO HERE
        // =======================
            
        // -----------------------------------------------------          
        // Check to see if a valid type is defined for this node
        // -----------------------------------------------------
                          
        String nodeType = (String) parameters.get("Type");
        
        if (nodeType == null)
        {
          problemsList.add("No type defined for node: " + nodepath);
        }
        else if (!nodeType.equalsIgnoreCase("int") && !nodeType.equalsIgnoreCase("bool") &&
                !nodeType.equalsIgnoreCase("bin") && !nodeType.equalsIgnoreCase("node") &&
                !nodeType.equalsIgnoreCase("chr") && !nodeType.equalsIgnoreCase("null") &&
                !nodeType.equalsIgnoreCase("test") && !nodeType.equalsIgnoreCase("date") &&
                !nodeType.equalsIgnoreCase("time") && !nodeType.equalsIgnoreCase("float"))
            
        {
          problemsList.add("Invalid node type '" + nodeType + "' for the node: " + nodepath + 
                                 ". The valid types are: 'node', 'int', 'bin', 'chr', 'bool', 'float', 'time', 'date'.");
          
        }
        
        // -------------------------------------------------------
        // Sanity checks for 'default' MA (format,constraints, etc)
        // -------------------------------------------------------
        
        String defValue = (String) parameters.get("Default");
        
        if (defValue != null)
        {
          //     -----------------------------------------------------
          // Do not allow default values for node types:
          // 'test', 'null', 'node'
          // -----------------------------------------------------
          
          if (nodeType != null && (nodeType.equalsIgnoreCase("test") || nodeType.equalsIgnoreCase("null") ||
              nodeType.equalsIgnoreCase("node")))
          {
            problemsList.add("Default values are not allowed for nodes with type '" + nodeType + "' for the node: " + nodepath +
                             ". The attribute 'default' should be removed");
          }
          
          // Check these values for min/max and other constraints
          
          checkValues(defValue,parameters,nodepath,nodeType,"Default");
        }
        
        // -------------------------------------------------------
        // Make sure that only multi-nodes have storesPD parameter
        // -------------------------------------------------------
        
        if (!isMultiNode && (parameters.get("storesPD") != null))
        {
          problemsList.add("The 'storesPD' attribute is not allowed for the non-multinode: " + nodepath +
                              ". This attribute should be removed.");    
        }
        
        // --------------------------------------------------------------
        // Make sure that 'auto' MA is only used on interior & test nodes
        // --------------------------------------------------------------
        
        if ((parameters.get("auto") != null) && (nodeType != null && !nodeType.equalsIgnoreCase("node")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("test")))
        {
          problemsList.add("The 'auto' attribute is not allowed for the non-interior node: " + nodepath +
                           ". This attribute should be removed.");    
        }
        
        // ------------------------------------------------------
        // Sanity checks for 'fk' MA (format,constraints, etc)
        // ------------------------------------------------------
        
        String fkStr = (String) parameters.get("fk");
        
        if (fkStr != null && (nodeType != null && nodeType.equalsIgnoreCase("node")))
        {
          problemsList.add("The 'fk' attribute is not allowed for the interior node: " + nodepath +
                           ". This attribute should be removed.");    
        }
        
        if (fkStr != null && fkStr.length() == 0)
        {
          problemsList.add("The 'fk' attribute can not be an empty string for the node: " + nodepath +
                             ". This attribute should be removed or value for 'fk' should be set.");
        }
        
        // -------------------------------------------------------
        // Sanity checks for 'value' MA (format,constraints, etc)
        // -------------------------------------------------------
        
        String value = (String) parameters.get("Value");
        
        if (value != null)
        {
          if (nodeType != null && nodeType.equalsIgnoreCase("node"))
          {
              problemsList.add("The 'value' attribute is not allowed for the interior node: " + nodepath +
                             ". This attribute should be removed.");    
          }
            
          //  Check these values for min/max and other constraints
          
          checkValues(value,parameters,nodepath,nodeType,"Value");
        }
        
        // --------------------------------------------------------
        // Sanity checks for 'values' MA (format,constraints, etc)
        // --------------------------------------------------------
        
        String values = (String) parameters.get("values");
        
        if (values != null)
        {
          if (nodeType != null && nodeType.equalsIgnoreCase("node"))
          {
            problemsList.add("The 'values' attribute is not allowed for the interior node: " + nodepath +
                             ". This attribute should be removed.");        
          }
          
          // Check these values for min/max and other constraints
          
          checkValues(values,parameters,nodepath,nodeType,"Values");
          
          // Check values and default values against possible values
          
          if (value != null)
          {
              if (values.indexOf(value) == -1)
            {
              problemsList.add("Initial value '" + value +
                                      "' is not one of the valid values in 'values' constraint '" +
                              values + "' for the node: " + nodepath +
                              ". The attribute 'value' should be set to one of the valid values.");
            }
          }
          
          if (defValue != null)
          {
              if (values.indexOf(defValue) == -1)
            {
              problemsList.add("Default value '" + defValue +
                                   "' is not one of the valid values in 'values' constraint '" +
                               values + "' for node: " + nodepath +
                               ". The attribute 'default' should be set to one of the valid values.");
            }
          }
        }
        
        // ----------------------------------------------------------------------
        // Make sure that 'min' MA is only used on int/float/date/time/test nodes
        // ----------------------------------------------------------------------
        
        if ((parameters.get("min") != null) && (nodeType != null && !nodeType.equalsIgnoreCase("int")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("test")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("float")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("date")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("time")))
        {
          problemsList.add("The 'min' attribute is not allowed for nodes with type '" + nodeType + "' at the node: " + nodepath +
                           ". This attribute should be removed.");            
        }
        
        // ----------------------------------------------------------------------
        // Make sure that 'max' MA is only used on int/float/date/time/test nodes
        // ----------------------------------------------------------------------
        
        if ((parameters.get("max") != null) && (nodeType != null && !nodeType.equalsIgnoreCase("int")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("test")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("float")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("date")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("time")))
        {
          problemsList.add("The 'max' attribute is not allowed for nodes with type '" + nodeType + "' at the node: " + nodepath +
                           ". This attribute should be removed.");            
        }
        
        // ---------------------------------------------------------
        // Make sure that 'minLen' MA is only used on chr/bin/test nodes
        // ---------------------------------------------------------
        
        if ((parameters.get("minLen") != null) && (nodeType != null && !nodeType.equalsIgnoreCase("chr")) &&
              (nodeType != null && !nodeType.equalsIgnoreCase("bin")) && (nodeType != null && !nodeType.equalsIgnoreCase("test")))
        {
          problemsList.add("The 'minLen' attribute not allowed for nodes with type '" + nodeType + "' at the node: " + nodepath +
                           ". This attribute should be removed.");            
        }
        
        // ---------------------------------------------------------
        // Make sure that 'maxLen' MA is only used on chr/bin/test nodes
        // ---------------------------------------------------------
        
        if ((parameters.get("maxLen") != null) && (nodeType != null && !nodeType.equalsIgnoreCase("chr")) &&
                (nodeType != null && !nodeType.equalsIgnoreCase("bin")) && (nodeType != null && !nodeType.equalsIgnoreCase("test")))
        {
          problemsList.add("The 'maxLen' attribute not allowed for nodes with type '" + nodeType + "' at the node: " + nodepath +
                           ". This attribute should be removed.");            
        }
        
        // ---------------------------------------------------------
        // Sanity checks for 'regexp' MA (format,constraints, etc)
        // ---------------------------------------------------------
        
        String regexpStr = (String) parameters.get("regexp");
        
        if (regexpStr != null && (nodeType != null && !nodeType.equalsIgnoreCase("chr")) &&
            (nodeType != null && !nodeType.equalsIgnoreCase("test")))
        {
          problemsList.add("The 'regexp' attribute not allowed for the non-character node: " + nodepath+
                           ". This attribute should be removed.");                
        }
        else if (regexpStr != null && regexpStr.length() == 0)
        {
          problemsList.add("The 'regexp' attribute may not be an empty string for the node: " + nodepath +
               ". This attribute should be removed or value for 'regexp' should be set.");
        }
        else if (regexpStr != null)
        { 
          try
          {
            RE regexpValue = new RE(regexpStr);
            
            // Check default and value MA's against this regular expression
              
            if (value != null)
            {
              if (!regexpValue.match(value))
              {
                  problemsList.add("Value attribute '" + value + "' does not match regular expression '" + regexpStr + "' for node: " + nodepath);
              }
            }
              
            if (defValue != null)
            {
              if (!regexpValue.match(defValue))
              {
                problemsList.add("Default attribute '" + defValue + "' does not match regular expression '" + regexpStr + "' for node: " + nodepath);
              }
            }
          }
          catch (RESyntaxException e)
          {
              problemsList.add("Invalid regular expression string '" + regexpStr +  "' for the node: " + nodepath);
          }      
        }
        
        // ----------------------------------------------------
        // Checks for 'nMaxLen' MA (format,current nodename)
        // ----------------------------------------------------
        
        String nMaxLenStr = (String) parameters.get("nMaxLen");
        
        if (nMaxLenStr != null)
        {
          if (nMaxLenStr.length() != 0)
          {
            try
            {
              int maxIntValue = Integer.parseInt(nMaxLenStr);
                        
              if (nodeName.length() > maxIntValue)
              {
                problemsList.add("Node name '" + nodeName +
                                        "' is longer than the maximum node name length constraint '" +
                                maxIntValue + "' for the node: " + nodepath);
              }    
            }
            catch (NumberFormatException e)
            {
              problemsList.add("The attribute nMaxLen has invalid value '" + nMaxLenStr + "' for the node: " + nodepath +
                                 ". This attribute should be valid integer from 1 to 2147483647.");
            }
          }
          else
          {
            problemsList.add("The attribute nMaxLen may not be an empty string for the node: " + nodepath);    
          }
        }
        
        // ----------------------------------------------------
        // Checks for 'nValues' MA (format,current nodename)
        // ----------------------------------------------------
        
        String nValuesStr = (String) parameters.get("nValues");
        
        if (nValuesStr != null)
        {
          if (nValuesStr.length() != 0)
          {            
            if (nValuesStr.indexOf(nodeName) == -1 && !nodeName.equals("*"))
            {
              problemsList.add("Node name '" + nodeName +
                                      "' is not one of the valid values in the attribute nValues constraint '" +
                              nValuesStr + "' for the node: " + nodepath);
            }
          }
          else
          {
            problemsList.add("The attribute nValues  may not be an empty string for node: " + nodepath);    
          }
        }
                
        // ----------------------------------------------------
        // Checks that file name (current nodename) doesn't contain illegal chars
        // ----------------------------------------------------        
        for(int i=0; i<illegalCharsForNodeName.length; i++){
          if(nodeName.indexOf(illegalCharsForNodeName[i]) > -1){
              problemsList.add("The node name contains illegal char '" + illegalCharsForNodeName[i] + "' for the " + nodepath);    
          }
        }
        
        // ----------------------------------------------------
        // Checks for 'nRegexp' MA (format,current nodename)
        // ----------------------------------------------------
        
        String nRegexpStr = (String) parameters.get("nRegexp");
        
        /*
        if (nRegexpStr != null && nodeName != "*")
        {
          if (nRegexpStr.length() != 0)
          {       
              try
            {
              RE nRegexpValue = new RE(nRegexpStr);
                
              // Check the nodename against this regular expression
                     
              if (!nRegexpValue.match(nodeName))
              {
                problemsList.add("Node name '" + nodeName + "' does not match regular expression '" + nRegexpStr + "' for node: " + nodepath);
              }    
            }
            catch (RESyntaxException e)
            {
              problemsList.add("Invalid regular expression string '" + nRegexpStr +  "' for node: " + nodepath);
            }
          }
          else
          {
            problemsList.add("nRegexp attribute may not be an empty string for node: " + nodepath);    
          }
        }*/
        
        // ---------------------------------------------------------
        // Make sure that 'access' MA has valid values
        // ---------------------------------------------------------
        
        ArrayList accessType = (ArrayList) parameters.get("AccessType");
        
        if (accessType == null || accessType.isEmpty())
        {
          problemsList.add("The attribute 'access' is not present for the node: " + nodepath 
                  + ". This attribute is required");
        }
        else
        {
          Object accessTokens[] = accessType.toArray();
          
          for (int i=0;i<accessTokens.length;i++)
          {
              if (!((String)accessTokens[i]).equalsIgnoreCase("Add") &&
                  !((String)accessTokens[i]).equalsIgnoreCase("Delete") && 
                !((String)accessTokens[i]).equalsIgnoreCase("Get") && 
                !((String)accessTokens[i]).equalsIgnoreCase("Exec")&& 
                !((String)accessTokens[i]).equalsIgnoreCase("Replace")&& 
                !((String)accessTokens[i]).equalsIgnoreCase("Local"))
              {
                problemsList.add("Invalid access type '" + accessTokens[i] + "' for node: " + nodepath 
                    + ". The valid types are: 'Add', 'Delete', 'Get', 'Replace', 'Exec', 'Local'");
             }    
          }    
        }
        
        //----------------------------------------------------
        // Checks for 'store' values (true,false,1,0) and that parms 'default' and 'value' hasn't been set
        // ----------------------------------------------------
        String storeStr = (String) parameters.get("store");
        
        if (storeStr != null)
        {
          //validate that parms 'default' and 'value' hasn't been set
          if(parameters.get("Default") != null){
              problemsList.add("Attribute 'default' cannot be set with attribute 'store' for the node: " + nodepath);    
          }
          if(parameters.get("Value") != null){
              problemsList.add("Attribute 'value' cannot be set with attribute 'store' for the node: " + nodepath);    
          }
            
          if (storeStr.length() != 0)
          {            
            if (!storeStr.equalsIgnoreCase("true") && !storeStr.equalsIgnoreCase("false") && 
                    !storeStr.equalsIgnoreCase("1") && !storeStr.equalsIgnoreCase("0"))
            {
              problemsList.add("The value '" + storeStr + "' for parm 'store' is not one of the valid values for node: " + nodepath
                      + ". The valid values are: 'true', 'false'");
            }
          }
          else
          {
            problemsList.add("The 'store' attribute may not be an empty string for the node: " + nodepath);    
          }
        }
    
        //----------------------------------------------------
        // Checks for 'LOBProgressBAR' values (true,false,1,0) and that parm "store" is presented
        // ----------------------------------------------------
        String LOBProgressBARStr = (String) parameters.get("LOBProgressBAR");
        
        if (LOBProgressBARStr != null)
        {
          //parm 'store' is required and should be true or false    
          if(parameters.get("store") == null){
              problemsList.add("The attribute 'LOBProgressBAR' cannot be specified without attribute 'store' for the node: " + nodepath);
          }
          else if(!((String) parameters.get("store")).trim().equalsIgnoreCase("true") && 
                  !((String) parameters.get("store")).trim().equalsIgnoreCase("1")){
              problemsList.add("The attribute 'LOBProgressBAR' can be specified only if attribute 'store' equals 'true' or '1' for the node: " + nodepath);
          }
          
          if (LOBProgressBARStr.length() != 0)
          {            
            if (!LOBProgressBARStr.equalsIgnoreCase("true") && !LOBProgressBARStr.equalsIgnoreCase("false") && 
                    !LOBProgressBARStr.equalsIgnoreCase("1") && !LOBProgressBARStr.equalsIgnoreCase("0"))
            {
              problemsList.add("The value '" + LOBProgressBARStr + "' for parm 'LOBProgressBAR' is not one of the valid values for the node: " + nodepath
                      + ". The valid values are: 'true', 'false'");
            }
          }
          else
          {
            problemsList.add("The LOBProgressBAR attribute may not be an empty string for node: " + nodepath);    
          }
        }
        
        // ---------------------------------------------------------
        // Make sure that parameter 'event' has valid values
        // ---------------------------------------------------------
        ArrayList arrEvents = (ArrayList) parameters.get("Event");
        
        if (arrEvents != null && !arrEvents.isEmpty())
        {
          String event;
          HashMap mapNotifyAndIgnoreVals = new HashMap(); //keeps values for "Notify" and " Ignore"
          for (int i=0;i<arrEvents.size();i++)
          {
              event = (String)arrEvents.get(i);
              if (!validateEventValues(event, arrEvents, isLeaf, mapNotifyAndIgnoreVals))
              {
                problemsList.add("The  attribute 'event' contains invalid value for the node: " + nodepath 
                    + ". The valid operations are: 'Add', 'Delete', 'Replace', 'Indirect'. "
                + " The valid format can be 'Node' or 'Detail' or 'Cumulative' (for interior nodes only)."
                + " The keys in key=value(s) pairs can be 'Topic' (single), 'Ignore' (list), 'Notify' (list)."
                + "Sample: Topic=AAA&Notify=BBB&CCC. 'Ignore' and 'Notify' cannot have the same values.");
                break;
             }    
          }    
        }
        
        
        
        
        // --- END OF DMT VALIDATIONS ---
      }
      else
      {
          problemsList.add("Missing parm.txt file for node: " + nodepath);
      }
                  
      // Process child directories
             
      if (children == null) 
      {
        // Either dir does not exist or is not a directory
      } 
      else 
      {
        // process any directories
        
        for (int ii=0; ii<children.length; ii++) 
        {
          if (children[ii].isDirectory()) 
          {
            analyzeDir(children[ii].getAbsolutePath());        
          }                                
        }
      }
    
      depth -= 1;
    }

    // Validate passed in value against empty string criteria, as well as violation of any constraints
    private void checkValues(String value,HashMap parameters,String nodepath,String nodeType,String type)
    {
          if (value.length() != 0)
      {
              StringTokenizer valueTokens = new StringTokenizer(value,",");
              String valueToken;
              
              while (valueTokens.hasMoreTokens())
              {
                valueToken = valueTokens.nextToken();
                
          if (nodeType != null && nodeType.equalsIgnoreCase("int"))
          {
            // Attempt to parse this value into an int - if we get a NumberFormat Exception, flag this
            // as an invalid value.
              
            boolean validValue = true;
            int intValue = 0;
              
            try 
            {
              intValue = Integer.parseInt(valueToken);    
            }
            catch (NumberFormatException e)
            {
              problemsList.add(" The '" + valueToken + "' is an invalid integer value for the attribute '"+ type 
                      + "' for node: " + nodepath + ". The valid value should be an integer from -2147483648 to 2147483647");
                validValue = false;
            }
              
            // Check this value against any min/max constraints specified for this node
              
            String minStrValue = (String) parameters.get("min");
              
            if (minStrValue != null)
            {
              try
              {
                  int minIntValue = Integer.parseInt(minStrValue);
                    
                  if (validValue && intValue < minIntValue)
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                          "' is less than minimum value constraint '" +
                                  minStrValue + "' for node: " + nodepath);
                  }                    
              }
                catch (NumberFormatException e)
              {
                  problemsList.add("Invalid min value '" + minStrValue + "' for node: " + nodepath 
                          + ". The valid value should be an integer from -2147483648 to 2147483647");
              }
            }
              
            String maxStrValue = (String) parameters.get("max");
              
            if (maxStrValue != null)
            {
              try
              {
                  int maxIntValue = Integer.parseInt(maxStrValue);
                    
                  if (validValue && intValue > maxIntValue)
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                          "' is greater than maximum value constraint '" +
                                  maxStrValue + "' for node: " + nodepath);
                  }    
              }
                catch (NumberFormatException e)
              {
                  problemsList.add("Invalid max value '" + maxStrValue + "' for node: " + nodepath  
                          + ". The valid value should be an integer from -2147483648 to 2147483647");
              }
            }    
          }
          else if (nodeType != null && nodeType.equalsIgnoreCase("float"))
          {
            // Attempt to parse this value into a float - if we get a NumberFormat Exception, flag this
            // as an invalid value.
              
            boolean validValue = true;
            float floatValue = 0;
              
            try 
            {
              floatValue = Float.parseFloat(valueToken);    
            }
            catch (NumberFormatException e)
            {
              problemsList.add(type + " attribute '" + valueToken + "' is an invalid float value for node: " + nodepath);
                validValue = false;
            }
              
            // Check this value against any min/max constraints specified for this node
              
            String minStrValue = (String) parameters.get("min");
              
            if (minStrValue != null)
            {
              try
              {
                  float minFloatValue = Float.parseFloat(minStrValue);
                    
                  if (validValue && floatValue < minFloatValue)
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                          "' is less than minimum value constraint '" +
                                  minStrValue + "' for node: " + nodepath);
                  }                    
              }
                catch (NumberFormatException e)
              {
                  problemsList.add("The attribute 'min' contains invalid float value '" + minStrValue + "' for the node: " + nodepath);
              }
            }
              
            String maxStrValue = (String) parameters.get("max");
              
            if (maxStrValue != null)
            {
              try
              {
                  float maxFloatValue = Float.parseFloat(maxStrValue);
                    
                  if (validValue && floatValue > maxFloatValue)
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                          "' is greater than maximum value constraint '" +
                                  maxStrValue + "' for node: " + nodepath);
                  }    
              }
                catch (NumberFormatException e)
              {
                  problemsList.add("The attribute 'max' contains invalid float value '" + maxStrValue + "' for the node: " + nodepath);
              }
            }    
          }
          else if (nodeType != null && nodeType.equalsIgnoreCase("date"))
          {
            // Attempt to parse this value into a date - if we get an IllegalArgument Exception, flag this
            // as an invalid value.
              
            boolean validValue = true;
            DateTimeFormatter fmt = ISODateTimeFormat.dateParser();
            DateTime dateValue = null;
              
            try 
            {
              dateValue = fmt.parseDateTime(valueToken);    
            }
            catch (IllegalArgumentException e)
            {
              problemsList.add(type + " attribute '" + valueToken + "' is an invalid date value for node: " + nodepath
                      + ". The valid value type 'date' should be in the one of valid ISO8601 formats, for example 'YYYY-MM-DD', 'YYYY-MM', 'YYY-DDD', 'YYYY-Wxx-d', 'YYYYMMDD', etc.");
                validValue = false;
            }
              
            // Check this value against any min/max constraints specified for this node
              
            String minStrValue = (String) parameters.get("min");
              
            if (minStrValue != null)
            {
              try
              {
                  DateTime minDateValue = fmt.parseDateTime(minStrValue);
                    
                  if (validValue && dateValue.isBefore(minDateValue))
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                          "' is less than minimum value constraint '" +
                                  minStrValue + "' for node: " + nodepath);
                  }                    
              }
                catch (IllegalArgumentException e)
              {
                  problemsList.add("The attribute 'min' contains invalid value '" + minStrValue + "' for the node: " + nodepath
                          + ". The valid value type 'date' should be in the one of valid ISO8601 formats, for example 'YYYY-MM-DD', 'YYYY-MM', 'YYY-DDD', 'YYYY-Wxx-d', 'YYYYMMDD', etc.");
              }
            }
              
            String maxStrValue = (String) parameters.get("max");
              
            if (maxStrValue != null)
            {
              try
              {
                  DateTime maxDateValue = fmt.parseDateTime(maxStrValue);
                    
                  if (validValue && dateValue.isAfter(maxDateValue))
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                          "' is greater than maximum value constraint '" +
                                  maxStrValue + "' for node: " + nodepath);
                  }    
              }
                catch (IllegalArgumentException e)
              {
                  problemsList.add("The attribute 'max' contains invalid value '" + maxStrValue + "' for the node: " + nodepath
                          + ". The valid value type 'date' should be in the one of valid ISO8601 formats, for example 'YYYY-MM-DD', 'YYYY-MM', 'YYY-DDD', 'YYYY-Wxx-d', 'YYYYMMDD', etc.");
              }
            }    
          }
          else if (nodeType != null && nodeType.equalsIgnoreCase("time"))
          {
            // Attempt to parse this value into a time - if we get an IllegalArgument Exception, flag this
            // as an invalid value.
              
            boolean validValue = true;
            DateTimeFormatter fmt = ISODateTimeFormat.timeParser();
            DateTime timeValue = null;
              
            try 
            {
              timeValue = fmt.parseDateTime(valueToken);    
            }
            catch (IllegalArgumentException e)
            {
              problemsList.add(type + " attribute '" + valueToken + "' is an invalid time value for node: " + nodepath
                      + ". The valid value type 'time' should be in the one of valid ISO8601 formats, for example 'hh:mm:ss', 'hh:mm', 'hhmmss', 'hhmm', 'hh', etc.");
                validValue = false;
            }
              
            // Check this value against any min/max constraints specified for this node
              
            String minStrValue = (String) parameters.get("min");
              
            if (minStrValue != null)
            {
              try
              {
                  DateTime minTimeValue = fmt.parseDateTime(minStrValue);
                    
                  if (validValue && timeValue.isBefore(minTimeValue))
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                          "' is less than minimum value constraint '" +
                                  minStrValue + "' for node: " + nodepath);
                  }                    
              }
                catch (IllegalArgumentException e)
              {
                  problemsList.add("The attribute 'min' contains invalid value '" + minStrValue + "' for the node: " + nodepath
                          + ". The valid value type 'time' should be in the one of valid ISO8601 formats, for example 'hh:mm:ss', 'hh:mm', 'hhmmss', 'hhmm', 'hh', etc.");
              }
            }
              
            String maxStrValue = (String) parameters.get("max");
              
            if (maxStrValue != null)
            {
              try
              {
                  DateTime maxTimeValue = fmt.parseDateTime(maxStrValue);
                    
                  if (validValue && timeValue.isAfter(maxTimeValue))
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                          "' is greater than maximum value constraint '" +
                                  maxStrValue + "' for node: " + nodepath);
                  }    
              }
                catch (IllegalArgumentException e)
              {
                  problemsList.add("The attribute 'min' contains invalid value '" + maxStrValue + "' for the node: " + nodepath
                          + ". The valid value type 'time' should be in the one of valid ISO8601 formats, for example 'hh:mm:ss', 'hh:mm', 'hhmmss', 'hhmm', 'hh', etc.");
              }
            }    
          }
          else if (nodeType != null && nodeType.equalsIgnoreCase("chr"))
          {
            // Checks for min/maxLength violations
              
            String minLenValue = (String) parameters.get("minLen");
              
            if (minLenValue != null)
            {
              try
              {
                  int minLenIntValue = Integer.parseInt(minLenValue);
                    
                  if (valueToken.length() < minLenIntValue)
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                        "' is less than minimum length value constraint '" +
                                  minLenValue + "' for node: " + nodepath);
                  }
              }
                catch (NumberFormatException e)
              {
                  problemsList.add("Invalid minLen value '" + minLenValue + "' for node: " + nodepath);
              }
            }
              
            String maxLenValue = (String) parameters.get("maxLen");
              
            if (maxLenValue != null)
            {
              try
              {
                  int maxLenIntValue = Integer.parseInt(maxLenValue);
                    
                  if (valueToken.length() > maxLenIntValue)
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                        "' is greater than maximum length value constraint: '" +
                                    maxLenValue + "' for node: " + nodepath);
                  }    
              }
                catch (NumberFormatException e)
              {
                  problemsList.add("Invalid maxLen value '" + maxLenValue + "' for node: " + nodepath);
              }
            }
          }
          else if (nodeType != null && nodeType.equalsIgnoreCase("bin"))
          {
              //Check for the correct binary value
              
              if (valueToken.length() % 2 > 0) {
                  problemsList.add("The attribute '" + type.toLowerCase() + "' for the node " + nodepath 
                          + " contains invalid binary value '" + valueToken + "'. \n"
                        + "The valid value should be a string with the following constrains: \n" 
                        + "  - No empty spaces and leadings '0x' and '%'; \n" 
                        + "  - The length of the value should be even number; \n" 
                        + "  - Value may contain only digits form '0' to '9' and letters from 'A' to 'F' ('a' to 'f'); \n" 
                        + "Sample:   0130AB0fc33d");
            }else{
                try
                {
                    String tmp;
                    for (int i = 0; i < valueToken.length() - 1; i++) {
                      tmp = valueToken.substring(i, i+2);
                      Integer.parseInt(tmp, 16);
                      i++;
                    }                  
                }catch(NumberFormatException e){
                    problemsList.add("The attribute '" + type.toLowerCase() + "' for the node " + nodepath 
                              + " contains invalid binary value '" + valueToken + "'. \n"
                            + "The valid value should be a string with the following constrains: \n" 
                            + "  - No empty spaces and leadings '0x' and '%'; \n" 
                            + "  - The length of the value should be even number; \n" 
                            + "  - Value may contain only digits form '0' to '9' and letters from 'A' to 'F' ('a' to 'f'); \n" 
                            + "Sample:   0130AB0fc33d");
                }                
            }           
            // Checks for min/maxLength violations. Compare with 1/2 length of the string representation
              
            String minLenValue = (String) parameters.get("minLen");
              
            if (minLenValue != null)
            {
              try
              {
                  int minLenIntValue = Integer.parseInt(minLenValue);
                    
                  if ((valueToken.length())/2 < minLenIntValue)
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                        "' is less than minimum length value constraint '" +
                                  minLenValue + "' for node: " + nodepath);
                  }
              }
                catch (NumberFormatException e)
              {
                  problemsList.add("Invalid minLen value '" + minLenValue + "' for node: " + nodepath);
              }
            }
              
            String maxLenValue = (String) parameters.get("maxLen");
              
            if (maxLenValue != null)
            {
              try
              {
                  int maxLenIntValue = Integer.parseInt(maxLenValue);
                    
                  if ((valueToken.length())/2 > maxLenIntValue)
                  {
                    problemsList.add(type + " attribute '" + valueToken +
                                        "' is greater than maximum length value constraint: '" +
                                    maxLenValue + "' for node: " + nodepath);
                  }    
              }
                catch (NumberFormatException e)
              {
                  problemsList.add("Invalid maxLen value '" + maxLenValue + "' for node: " + nodepath);
              }
            }        
          }      
        }
      }
      else
      {
          if(!"Value".equalsIgnoreCase(type)){
          problemsList.add(type + " attribute may not be an empty string for the node: " + nodepath);    
          }
      }
    }
    
    // Utility function to remove spaces in a comma-seperated list (for values MA)
    private String removeSpaces(String line)
    {
      int i = 0;
      
      if ((i = line.indexOf(' ', i)) >= 0)
      {
        char [] line2 = line.toCharArray();
        StringBuffer buf = new StringBuffer(line2.length);
        buf.append(line2, 0, i).append("");
        
        i++;
        int j = i;
        
        while ((i = line.indexOf(' ', i)) > 0)
        {
          buf.append(line2, j, i - j).append("");
          i++;
          j = i;
        }
        
        buf.append(line2, j, line2.length - j);
        return buf.toString();
      }
      
      return line;
    }

    // Utility function to parse the contents of parm.txt files in each DMT directory
    private void getParms(String fileName,HashMap parameters,ArrayList access) throws Exception 
    {
      Reader reader = NodeLoader.getReader(fileName);    
      BufferedReader in = new BufferedReader(reader);
      String line;
      
      try
      {
       while ((line = in.readLine()) != null) 
       {
          if(line.trim().length() == 0 || line.trim().startsWith("#")){
              continue;
          }
          
        int nameBoundary = line.indexOf(":");
        if(nameBoundary <= 0){
            problemsList.add("The line '"+line+"' doesn't contain ':' simbol in the file " + fileName 
                    + ". The valid attribute/value pair should be separated by ':'.");
            continue;
        }
        
        String parmName = line.substring(0,nameBoundary);
        String parmValue = line.substring(nameBoundary + 1);
        
       // if(parmValue == null || parmValue.trim().length() == 0){
       //     problemsList.add("The line '"+line+"' doesn't contain any value after  simbol ':' in the file " + fileName + ". Please add value or remove unused parameter.");
       // }
        if (parmName.equalsIgnoreCase("type")) 
        {
          if (parmValue.equalsIgnoreCase("boolean"))
          {
            parmValue="bool";
          }
                      
          parameters.put("Type", parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("storesPD"))
        {
              parameters.put("storesPD",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("HandledByPlugin"))
        {
              parameters.put("HandledByPlugin",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("ID"))
        {
              parameters.put("ID",parmValue.trim()); 
        }
        
        else if (parmName.equalsIgnoreCase("minLen"))
        {
          parameters.put("minLen",parmValue.trim());
        }
            
        else if (parmName.equalsIgnoreCase("maxLen"))
        {
          parameters.put("maxLen",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("maxChild"))
        {
          parameters.put("maxChild",parmValue.trim());    
        }
        
        else if (parmName.equalsIgnoreCase("min"))
        {
          parameters.put("min",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("max"))
        {
          parameters.put("max",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("values"))
        {
          parameters.put("values",trimSepSpaces(parmValue));
        }
        
        else if (parmName.equalsIgnoreCase("regexp"))
        {
          parameters.put("regexp",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("nMaxLen"))
        {
          parameters.put("nMaxLen",parmValue.trim());
        }
            
        else if (parmName.equalsIgnoreCase("nValues"))
        {
          parameters.put("nValues",removeSpaces(parmValue.trim()));
        }
        
        else if (parmName.equalsIgnoreCase("nRegexp"))
        {
          parameters.put("nRegexp",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("auto"))
        {
          parameters.put("auto",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("fk"))
        {
          parameters.put("fk",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("child"))
        {
          parameters.put("child",removeSpaces(parmValue.trim()));
        }
        
        else if (parmName.equalsIgnoreCase("depend"))
        {
          parameters.put("depend",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("recur-after-segment"))
        {
          parameters.put("recur-after-segment",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("max-recurrence"))
        {
          parameters.put("max-recurrence",parmValue.trim());
        }
            
        else if (parmName.equalsIgnoreCase("acl")) 
        {
          parameters.put("Acl", parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("event")) 
        {
          ArrayList arrEvents = new ArrayList();
          if(parmValue.trim().indexOf(' ') > -1){
              arrEvents.add(parmValue.trim()); //error... cannot contain empty space... will be handled later
          }
          else{
              StringTokenizer tok = new StringTokenizer(parmValue.trim(), "&");
              while (tok.hasMoreTokens())          
            {
                  String event = tok.nextToken().trim();
                  arrEvents.add(event);
            }                            
          }
          parameters.put("Event", arrEvents);
        }
        
        else if (parmName.equalsIgnoreCase("default")) 
        {
          parameters.put("Default", parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("value")) 
        {
          parameters.put("Value", parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("mime")) 
        {
          parameters.put("Mime", parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("access")) 
        {
          StringTokenizer tok = new StringTokenizer(parmValue, ",");
          ArrayList tmpAccess = new ArrayList();
          
          while (tok.hasMoreTokens()) 
          {
            String accessType = tok.nextToken().trim();
            tmpAccess.add(accessType);
          }
          parameters.put("AccessType", tmpAccess);
        }
        
        else if (parmName.equalsIgnoreCase("nodatagen")) 
        {
          parameters.put("DataGen", new Boolean(false));
        }
        
        else if (parmName.equalsIgnoreCase("nometagen")) 
        {
          parameters.put("MetaGen", new Boolean(false));
        }
        
        else if (parmName.equalsIgnoreCase("store"))
        {
              parameters.put("store",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("LOBProgressBAR"))
        {
              parameters.put("LOBProgressBAR",parmValue.trim());
        }
        
        else if (parmName.equalsIgnoreCase("description")) 
        {
            String docstr = parmValue;
            while ((line = in.readLine()) != null)
                docstr += line;
            parameters.put("Description", docstr);
            continue;
        }
        else{            
            problemsList.add("The attribute name '" + parmName + "' is not supported in the file: " + fileName);
        }
      }// while loop
     }finally{
        try
        {
           in.close();
        }catch(Exception e){} 
        try
        {
           reader.close();
        }catch(Exception e){}
        
        in = null;
        reader = null;
     }      
    }
    
    private  String trimSepSpaces(String parmValue) throws Exception { 
        String sep = ",";
        StringTokenizer tok = new StringTokenizer(parmValue, sep);
      StringBuffer sb = new StringBuffer();
      String tmp; 
      while (tok.hasMoreTokens()) 
      {
        tmp = tok.nextToken().trim();
        if(sb.length() > 0){
            tmp = sep + tmp;
        }
        sb.append(tmp);
        
      }
      return sb.toString();
    }
    
    //validate values from attribute event
    private boolean validateEventValues(String event, ArrayList arrEvents, 
                                boolean isLeaf, HashMap mapNotifyAndIgnoreVals){
        
        if(event.indexOf(' ') > -1){
            return false; //not allow empty spaces...
        }
        else if("Add".equals(event) || "Delete".equals(event) || 
                "Replace".equals(event) || "Indirect".equals(event) ){
            return true;
        }
        else if("Node".equals(event)){
            if(arrEvents.contains("Detail") || arrEvents.contains("Cumulative")){ // cannot be together with "Node"
                return false;
            }
            else{
                return true;
            }
        }
        else if("Detail".equals(event)){
            if(arrEvents.contains("Node") || arrEvents.contains("Cumulative")){ // cannot be together with "Detail"
                return false;
            }
            else{
                return true;
            }
        }
        else if("Cumulative".equals(event)){
            if(arrEvents.contains("Detail") || arrEvents.contains("Node") || isLeaf){ // cannot be together with "Cumulative" and interior nodes only
                return false;
            }
            else{
                return true;
            }
        }
        else if(event.startsWith("Topic=")){
            if (event.length() > "Topic=".length()){
                return true; // check if has value like Topic=ABC
            }else{
                return false;
            }
        }
        else if(event.startsWith("Ignore=") || event.startsWith("Notify=")){
            // !!! length of "Ignore=" and "Notify=" is the same and equals 7 !!!!
            if (event.length() == 7){
                return false; // check that value is not empty  like Ignore=
            }else{
                String vals = event.substring(7);
                StringTokenizer tok = new StringTokenizer(vals, "+");
                String tmp;
                while (tok.hasMoreTokens()){
                    tmp = tok.nextToken();
                    if(mapNotifyAndIgnoreVals.containsKey(tmp.toUpperCase())){
                        return false; // This value for key "Ignore" or  "Notify" already has been used
                    }
                    else{
                        mapNotifyAndIgnoreVals.put(tmp.toUpperCase(), tmp.toUpperCase());
                    }
                }
                return true;
            }
        }
        else {
            return false;   // unsupported parameter for attribute 'event'        
        }
    }
}
