#!/bin/bash


###########################################
# 1. 生成actionsconfig.java到framework/base/actions/com/actions
# 2. 生成actionsconfig.h到bionic/libc/include
#
###########################################

ACTIONC_CONFIG_INPUT=$1
ACTIONC_CONFIG_JAVA_PATH=$2
ACTIONC_CONFIG_H_PATH=$3



# .h
echo "/* auto generate from actions_owl.config, DONNOT EDIT!*/" >$ACTIONC_CONFIG_H_PATH
echo "#ifndef __ACTIONS_CONFIGH_H__" >>$ACTIONC_CONFIG_H_PATH
echo "#define __ACTIONS_CONFIGH_H__" >>$ACTIONC_CONFIG_H_PATH

sed -n  -r '/^\s*[a-zA-Z_0-9]+\s*=\s*[0-1]+\s*/p' $ACTIONC_CONFIG_INPUT |awk -F '=' '
{ if($NF=1) print  "#define ", $1, $2}' >> $ACTIONC_CONFIG_H_PATH

echo "#endif" >>$ACTIONC_CONFIG_H_PATH


 
# .java
echo "/* auto generate from actions_owl.config, DONNOT EDIT!*/" >$ACTIONC_CONFIG_JAVA_PATH
echo "package com.actions;" >>$ACTIONC_CONFIG_JAVA_PATH
echo "public class ActionsConfig {" >>$ACTIONC_CONFIG_JAVA_PATH
sed -n  -r '/^\s*[a-zA-Z_0-9]+\s*=\s*[0-1]+\s*/p' $ACTIONC_CONFIG_INPUT |awk -F '=' '
{ if($NF=1) print  "   public static int ", $1, "=", $2, ";"}' >>$ACTIONC_CONFIG_JAVA_PATH

echo "}" >>$ACTIONC_CONFIG_JAVA_PATH