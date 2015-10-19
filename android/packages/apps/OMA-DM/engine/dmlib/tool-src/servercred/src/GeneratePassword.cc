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

//--------------------------------------------------------------------------------------------------
//
//   Module Name: GeneratePassword.cc
//
//   Based on an IMEI and serverId input, the algorithm for generating the OMADM client password 
//   and server password actually has following three steps,
//
//   Step 1 Generate the client password key and server password key:  
//   char[] clientPasswordDict = new char[] { 0x0e, 0x06, 0x10,0x0c, 0x0a, 0x0e, 0x05, 0x0c,
//                                            0x12, 0x0a, 0x0b, 0x06, 0x0d, 0x0e, 0x05 };
//   char[] serverPasswordDict = new char[] { 0x0a, 0x06, 0x0e,0x0e, 0x0a, 0x0b, 0x06, 0x0e, 
//                                            0x0b, 0x04, 0x04, 0x07, 0x11, 0x0c, 0x0c };
//
//   It defines a client password dictionary and a server password dictionary which contain 
//   15 numbers, we use client password dictionary to generate client password key and server 
//   password dictionary to generate server password key.  Suppose IMEI string length is n, for 
//   each character in IMEI string {imei[i], 0<= i <n-3}, we generate two serial numbers using 
//   following calculation,  
//
//   Serial1 += imei[i + 3] * dict[i];   
//   Serial2 += imei[i + 3] * imei[i + 2]*dict[i];
//
//   Note: Serial numbers are in decimal.
//   Hence we get a password KEY which is Serial1+"-"+Serial2.
//
//   Step 2 Generate Temporary passwords:
//   We generate a MD5 digest from IMEI+KEY+serverId, then we pick No. 2,7,8,12,25,30 characters 
//   from MD5 DigestStr , let is be md5key. Convert IMEI to 36 radix number, let it be newImei 
//   which is always 10 characters, then we get a temporary password which is md5key+newImei.       
//
//   Step 3 Shuffle the temporary password:  
//   The last step is to shuffle the temporary password got from Step 2.
//   Since the password length is 16 and n is the length which is equal to 16, 
//   let P1,P2,P3,P4,...,P[n/2]-1, P[n/2], P[n/2]+1 , ... , Pn are the characters in the 
//   temporary password string, we do following shuffle,
//
//   move P[n/2]+1 between P[n/2]-1 and P[n/2],
//   ....
//   move Pn before P1.
//   Then we get Pn,P1,Pn-1,P2,....,P[n/2]-1,P[n/2]+1,P[n/2].
//   Do the same shuffle three times, then after third time shuffle we get the password which 
//   is 16 characters string.
//
//   Usage: GeneratePassword [IMEI] [SERVER_ID]
//
//   Example: GeneratePassword 000000011234564 motorola
//
//   Default: IMEI = 123456789012345
//       SERVER_ID = openwave.com
//

#include <stdio.h>          // printf()
#include <stdlib.h>         // exit() malloc()
#include <string.h>         // strcpy() strlen()
#include "md5.h"

#include "GeneratePassword.H"

/**
 *  Initialize all the client/server password dictionaries and other values.
 */
GeneratePassword::GeneratePassword() {

  clientPasswordDict[0] = 0x0e;
  clientPasswordDict[1] = 0x06;
  clientPasswordDict[2] = 0x10;
  clientPasswordDict[3] = 0x0c;
  clientPasswordDict[4] = 0x0a;
  clientPasswordDict[5] = 0x0e;
  clientPasswordDict[6] = 0x05;
  clientPasswordDict[7] = 0x0c;
  clientPasswordDict[8] = 0x12;
  clientPasswordDict[9] = 0x0a;
  clientPasswordDict[10] = 0x0b;
  clientPasswordDict[11] = 0x06;
  clientPasswordDict[12] = 0x0d;
  clientPasswordDict[13] = 0x0e;
  clientPasswordDict[14] = 0x05;
  
  serverPasswordDict[0] = 0x0a;
  serverPasswordDict[1] = 0x06;
  serverPasswordDict[2] = 0x0e;
  serverPasswordDict[3] = 0x0e;
  serverPasswordDict[4] = 0x0a;
  serverPasswordDict[5] = 0x0b;
  serverPasswordDict[6] = 0x06;
  serverPasswordDict[7] = 0x0e;
  serverPasswordDict[8] = 0x0b;
  serverPasswordDict[9] = 0x04;
  serverPasswordDict[10] = 0x04;
  serverPasswordDict[11] = 0x07;
  serverPasswordDict[12] = 0x11;
  serverPasswordDict[13] = 0x0c;
  serverPasswordDict[14] = 0x0c;
  
  hexTable[0] = '0';
  hexTable[1] = '1';
  hexTable[2] = '2';
  hexTable[3] = '3';
  hexTable[4] = '4';
  hexTable[5] = '5';
  hexTable[6] = '6';
  hexTable[7] = '7';
  hexTable[8] = '8';
  hexTable[9] = '9';
  hexTable[10] = 'a';
  hexTable[11] = 'b';
  hexTable[12] = 'c';
  hexTable[13] = 'd';
  hexTable[14] = 'e';
  hexTable[15] = 'f';

  imei = "123456789012345";
  serverId = "openwave.com";

  char * tmpIMEI = "123456789012345";
  char * tmpServerId = "openwave.com";

  int len = sizeof(char) * strlen(tmpIMEI) + 1;
  imei = (char *) malloc(len);
  memset(imei, '\0', (len));
  strcpy(imei, tmpIMEI);

  len = sizeof(char) * strlen(tmpServerId) + 1;
  serverId = (char *) malloc(len);
  memset(serverId, '\0', (len));
  strcpy(serverId, tmpServerId);

  MD5_HASH_LENGTH = 16;
}

/**
 *  Free all dynamic allocated memories.
 */
GeneratePassword::~GeneratePassword() {

  if (imei != NULL) {
    free(imei);
    imei = NULL;
  }

  if (serverId != NULL) {
    free(serverId);
    serverId = NULL;
  }
}

/**
 *  Sets the sever ID for password generation
 * 
 *  @param serverId the serverId that represent a DM server
 */
void GeneratePassword::setServerId(const char * sid) {
  if (serverId != NULL) {
    free(serverId);
    serverId = NULL;
  }

  int len = sizeof(char) * strlen(sid) + 1;
  serverId = (char *) malloc(len);
  memset(serverId, '\0', len);
  strcpy(serverId, sid);
}

/**
 *  Sets the IMEI for password generation
 *
 *  @param aIMEI a phone identification number
 */
void GeneratePassword::setIMEI(const char * aIMEI) {

  if (imei != NULL) {
    free(imei);
    imei = NULL;
  }

  int len = sizeof(char) * strlen(aIMEI) + 1;
  imei = (char *) malloc(len);
  memset(imei, '\0', len);
  strcpy(imei, aIMEI);
}

/**
 *  Returns the IMEI number.
 *
 *  @return a phone identification number
 */
char * GeneratePassword::getIMEI() {
  return imei;
}

/**
 *  Returns the server ID
 *
 *  @return serverId the serverId that represent a DM server
 */
char * GeneratePassword::getServerId() {
  return serverId;
}

/**
 *  Generate a client password key with a predefined client password dictionary
 *  based on the IMEI.
 *
 *  @param imei the imei use to generate the key
 *  @return the client password key
 */
char * GeneratePassword::generateClientPasswordKey(char * imei) {
  return generateKeyFromDict(imei, clientPasswordDict);
}
  
/**
 *  Generate a server password key with a predefined server password dictionary
 *  based on the IMEI.
 *
 *  @param imei the imei use to generate the key
 *  @return the server password key
 */
char * GeneratePassword::generateServerPasswordKey(char * imei) {
  return generateKeyFromDict(imei, serverPasswordDict);
}
 
/**
 *  Generate a client password using a generated client password key, the IMEI, and
 *  the server ID.
 *
 *  @return the client password
 */ 
char * GeneratePassword::generateClientPassword() {
  char * key = generateClientPasswordKey(imei);
  return generatePassword(imei, serverId, key);
}

/**
 *  Generate a server password using a generated server password key, the IMEI, and
 *  the server ID.
 *
 *  @return the server password
 */ 
char * GeneratePassword::generateServerPassword() {
  char * key = generateServerPasswordKey(imei);
  return generatePassword(imei, serverId, key);
}

/**
 *  Generate a client password using a generated client password key, the IMEI, and
 *  the server ID.
 *
 *  @param imei a phone identification number
 *  @param serverId the serverId that represent a DM server
 *  @return the client password
 */ 
char * GeneratePassword::generateClientPassword(char * imei, char * serverId) {
  char * key = generateClientPasswordKey(imei);
  return generatePassword(imei, serverId, key);
}

/**
 *  Generate a server password using a generated server password key, the IMEI, and
 *  the server ID.
 *
 *  @param imei a phone identification number
 *  @param serverId the serverId that represent a DM server
 *  @return the server password
 */   
char * GeneratePassword::generateServerPassword(char * imei, char * serverId) {
  char * key = generateServerPasswordKey(imei);
  return generatePassword(imei, serverId, key);
}

/**
 *  Generate a key with given IMEI and password dictionary.
 *  Suppose IMEI string length is n, for each character in IMEI string {imei[i], 0<= i <n-3}, 
 *  we generate two serial numbers using following calculation,  
 *
 *  Serial1 += imei[i + 3] * dict[i];   
 *  Serial2 += imei[i + 3] * imei[i + 2]*dict[i];
 *
 *  Note: Serial numbers are in decimal.
 *  Hence we get a password KEY which is Serial1+"-"+Serial2.
 *
 *  @param imei a phone indentification number
 *  @param dict[] a password dictionary
 *  @return a password key
 */
char * GeneratePassword::generateKeyFromDict(char * imei, char dict[]) {
  int i;
  int length; 
  long serial1 = 0;
  long serial2 = 0;
  char * serial1_str;
  char * serial2_str;
  char * key;

  length = strlen(imei);

  for (i = 0; i < length - 3; i++) {
    serial1 += imei[i + 3] * dict[i];
    serial2 += imei[i + 3] * imei[i + 2] * dict[i];
  }

  serial1_str = (char *) malloc(sizeof(char) * (24));
  serial2_str = (char *) malloc(sizeof(char) * (24));
  sprintf(serial1_str, "%d", serial1);
  sprintf(serial2_str, "%d", serial2);

  key = (char *) malloc(sizeof(char) * (strlen(serial1_str) + strlen(serial2_str) + 2));
  memset(key, '\0', (sizeof(char) * (strlen(serial1_str) + strlen(serial2_str) + 2)));

  strcat(key, (const char *)serial1_str );
  strcat(key, "-");
  strcat(key, (const char *)serial2_str );

  free(serial1_str);
  serial1_str = NULL;
  free(serial2_str);
  serial2_str = NULL;

  return key;
}

/**
 * Convert an array of characters that represents a large number to a decimal number type
 *
 * @param input the array of characters thar represents a large number
 * @return the decimal number
 */
long long GeneratePassword::convertChar2Long(char * input)
{
  char ch[2];
  int i;
  long long tmp;       
  tmp = 0;
	
  for (i=0 ; i < strlen((const char *)input) ; i++ ) {
    ch[0]=*(input+i);
    ch[1]='\0';
    tmp = (long long) ( ( (tmp) * 10) + atol((const char *)ch) );
  }
  return tmp;
}

/**
 * Convert an array of characters that represents a number in decimal based to 36 based.
 *
 * @param target_imei the array of characters thar represents a number in decimal based
 * @param the 36 based number represented by array of characters.
 */
char * GeneratePassword::get36BasedIMEI(char * target_imei) {
  
  char NumericBaseData[]= "0123456789abcdefghijklmnopqrstuvwxyz";
  char tmp_IMEI[11];
  long long Quotient;
  long Remainder;
  int i;
  char tmpchar;
  
  char * IMEI36 = (char *) malloc(sizeof(char) * ( 10 + 1 ));
  memset(IMEI36, '\0', (sizeof(char) * (10 + 1)));

  long long IMEI = 0;
  IMEI = convertChar2Long(target_imei);

  i=0;
  while ( IMEI > 0 ) {
    Quotient = (long long)(IMEI/36);
    Remainder = (long)(IMEI%36);    
    tmp_IMEI[i++] = NumericBaseData[Remainder];    
    IMEI = Quotient;
  }
  tmp_IMEI[i]='\0';
  
  //If the length is <10 pad the remaining chracter to '0' to make the length 10
  if( strlen(tmp_IMEI) < 10 ) {
    for (i=strlen(tmp_IMEI); i<10 ; i++) {
      tmp_IMEI[i]='0';
    }
    tmp_IMEI[i]='\0';
  }
  
  for( i=0 ; i < strlen(tmp_IMEI)/2 ; i++ ) {
    tmpchar = tmp_IMEI[i];
    tmp_IMEI[i] = tmp_IMEI[ strlen(tmp_IMEI)-i-1];
    tmp_IMEI[ strlen(tmp_IMEI)-i-1] = tmpchar;
  }

  memcpy(IMEI36,tmp_IMEI,strlen(tmp_IMEI));
  return IMEI36;
}

/**
 *  Shuffle an array of characters.
 *
 *  let P1,P2,P3,P4,...,P[n/2]-1, P[n/2], P[n/2]+1 , ... , Pn  
 *  are the characters in the string, we do following shuffle,
 *
 *  move P[n/2]+1 between P[n/2]-1 and P[n/2],
 *  ....
 *  move Pn before P1.
 *  Then we get 
 *  Pn,P1,Pn-1,P2,....,P[n/2]-1,P[n/2]+1,P[n/2].
 *
 *  @param buffer the string to be shuffle
 */
void GeneratePassword::shuffle(char & buffer) {

  int length;
  int secondHalfPos;
  int insertPos;
  char * buf;
  char tmpchar;
  int i;
  int j;

  insertPos = 0;
  i=0;
  j=0;  
  buf = & buffer;
  length = strlen((char *)buf);
  secondHalfPos = (length / 2);
  
  for ( i= secondHalfPos ; i < length ; i++ ) {
    tmpchar = (char)buf[i];
    insertPos = (length - i - 1);    
    for ( j = i ; j > insertPos ; j-- ) {
      buf[j] = buf[j-1];      
    }    
    buf[j] = tmpchar;
  }
}

/**
 * Convert the input data from a decimal based number to Heximal based number.
 * 
 * @param data the decimal based number to be covert
 * @return the Heximal based number
 */
char * GeneratePassword::encodeHex(char data[]) {

  int i;
  char tmpChar;
  int len = MD5_HASH_LENGTH;
  printf("LEN: %d\n", len);
  int size = len * 2 + 1;
  char * output = (char *) malloc(sizeof(char) * size);
  memset(output, '\0', (sizeof(char) * size));

  for ( i = 0; i < len ; i++) {
    tmpChar = data[i];
    output[2*i] = hexTable[ (tmpChar & 0x0F) ];		// Get low 4 bits
    output[(2*i)+1] = hexTable[ ((tmpChar >> 4) & 0x0F ) ];	// Get high 4 bits
  }
  output[2*i] = '\0';
  
  return output;
}


/**
 *  Generate a password with given IMEI, serverID, and key.  
 *  We generate a MD5 digest from IMEI+KEY+serverId, then we pick No. 2,7,8,12,25,30 characters 
 *  from MD5 DigestStr , let is be md5key. Convert IMEI to 36 radix number, let it be newImei 
 *  which is always 10 characters, then we get a temporary password which is md5key+newImei.
 *  Finally, we shuffle the temporary password three time.  
 *
 *  @param imei the phone identification number
 *  @param serverId the server ID of a DM server.
 *  @param key the key needs to generate password
 *  @return a password
 */
char * GeneratePassword::generatePassword(char * imei, char * serverId, char * key) {

  char * MD5DigestStr = (char *) malloc(sizeof(char) * (strlen ((const char *)imei) + strlen(key) + strlen(serverId) +1));
  memset( MD5DigestStr , '\0', (sizeof(char) * ( strlen ((const char *)imei) + strlen(key) + strlen(serverId) +1 ) ) );
  
  strcpy( MD5DigestStr , (const char *)imei );
  strcat( MD5DigestStr , key );
  strcat( MD5DigestStr , serverId );

  printf("Before MD5: %s\n", MD5DigestStr);
  
  MD5_CTX md5_context;

  char md5hash[MD5_HASH_LENGTH + 1];  /* Add 1 character for NULL */
  memset(md5hash, '\0', (sizeof(char) * (MD5_HASH_LENGTH + 1)));
  
  smlMD5Init(&md5_context);
  smlMD5Update(&md5_context, (unsigned char *)MD5DigestStr,strlen(MD5DigestStr));
  smlMD5Final((unsigned char*)md5hash, &md5_context);
  md5hash[MD5_HASH_LENGTH] = 0;
    
  free(MD5DigestStr );
  MD5DigestStr =NULL;

  printf("MD5 HASH: %s\n", md5hash);  

  char * MD5DigestStr32 = encodeHex(md5hash);

  printf("Digest Str: %s\n", MD5DigestStr32);

  // Pick only no. 2,7,8,12,25,30 characters from MD5 Digest String
	
  MD5DigestStr = (char *) malloc(sizeof(char) * ( 6 + 1 ));
  memset(MD5DigestStr, '\0', (sizeof(char) * ( 6 + 1)));
  
  MD5DigestStr[0] = MD5DigestStr32[2];
  MD5DigestStr[1] = MD5DigestStr32[7];
  MD5DigestStr[2] = MD5DigestStr32[8];
  MD5DigestStr[3] = MD5DigestStr32[12];
  MD5DigestStr[4] = MD5DigestStr32[25];
  MD5DigestStr[5] = MD5DigestStr32[30];
  
  free(MD5DigestStr32 );
  MD5DigestStr32 =NULL;

  //Convert IMEI to 36(base) radixnumber to generate NewIMEI
  
  char * IMEI36 = (char *) malloc(sizeof(char) * ( 10 + 1 ));
  memset(IMEI36, '\0', (sizeof(char) * (10 + 1)));
  
  IMEI36 = get36BasedIMEI(imei);

  //Create Password

  int PWLength = strlen (MD5DigestStr) + strlen((const char *)IMEI36);  //Length will be 16

  char * password = (char *) malloc(sizeof(char) * ( PWLength + 1));
  memset( password , '\0', (sizeof(char) * ( PWLength + 1 ) ) ); //this will be of 16 + 1
  
  strcpy( password , MD5DigestStr );
  strcat( password , (const char *)IMEI36 );
  
  free(MD5DigestStr );
  MD5DigestStr =NULL;
  
  free(IMEI36);
  IMEI36 =NULL;

  free(MD5DigestStr32 );
  MD5DigestStr32 =NULL;

  printf("BUFFER: %s\n", password);

  shuffle(*password);
  shuffle(*password);
  shuffle(*password);

  free( key );
  key=NULL;

  return password;
}
