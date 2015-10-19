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
//   FCS14345 Implementation
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

#include "md5.h"
#include "xpl_Types.h"
#include "dmStringUtil.h"
#include "dmMemory.h"       

#include "GeneratePassword.H"

#define MD5_HASH_LENGTH 16

const char GeneratePassword::hexTable[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
const char GeneratePassword::clientPasswordDict[15] = { 0x0e, 0x06, 0x10,0x0c, 0x0a, 0x0e, 0x05, 0x0c, 0x12, 0x0a, 0x0b, 0x06, 0x0d, 0x0e, 0x05 };
const char GeneratePassword::serverPasswordDict[15] = { 0x0a, 0x06, 0x0e,0x0e, 0x0a, 0x0b, 0x06, 0x0e, 0x0b, 0x04, 0x04, 0x07, 0x11, 0x0c, 0x0c };

/**
 *  Initialize all the client/server password dictionaries and other values.
 */
GeneratePassword::GeneratePassword() {
  imei = NULL;
  serverId = NULL;
}

/**
 *  Free all dynamic allocated memories.
 */
GeneratePassword::~GeneratePassword() {

  if (imei != NULL) {
    DmFreeMem(imei);
    imei = NULL;
  }

  if (serverId != NULL) {
    DmFreeMem(serverId);
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
    DmFreeMem(serverId);
    serverId = NULL;
  }

  if (sid != NULL) {
    int len = sizeof(char) * (DmStrlen(sid) + 1);
    serverId = (char *) DmAllocMem(len);
    memset(serverId, '\0', (len));
    DmStrcpy(serverId, sid);
  }
}

/**
 *  Sets the IMEI for password generation
 *
 *  @param aIMEI a phone identification number
 */
void GeneratePassword::setIMEI(const char * aIMEI) {

  if (imei != NULL) {
    DmFreeMem(imei);
    imei = NULL;
  }

  if (aIMEI != NULL) {
    int len = sizeof(char) * (DmStrlen(aIMEI) + 1);
    imei = (char *) DmAllocMem(len);
    memset(imei, '\0', (len));
    DmStrcpy(imei, aIMEI);
  }
}

/**
 *  Returns the IMEI number.
 *
 *  @return a phone identification number
 */
const char * GeneratePassword::getIMEI() {
  return imei;
}

/**
 *  Returns the server ID
 *
 *  @return serverId the serverId that represent a DM server
 */
const char * GeneratePassword::getServerId() {
  return serverId;
}

/**
 *  Generate a client password key with a predefined client password dictionary
 *  based on the IMEI.
 *
 *  @param imei the imei use to generate the key
 *  @return the client password key
 */
char * GeneratePassword::generateClientPasswordKey(const char * imei) {
  return generateKeyFromDict(imei, clientPasswordDict);
}
  
/**
 *  Generate a server password key with a predefined server password dictionary
 *  based on the IMEI.
 *
 *  @param imei the imei use to generate the key
 *  @return the server password key
 */
char * GeneratePassword::generateServerPasswordKey(const char * imei) {
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
  char * password = generatePassword(imei, serverId, key);

  if (key != NULL) {
    DmFreeMem(key);
    key = NULL;
  }    

  return password;
}
  
/**
 *  Generate a server password using a generated server password key, the IMEI, and
 *  the server ID.
 *
 *  @return the server password
 */ 
char * GeneratePassword::generateServerPassword() {
  char * key = generateServerPasswordKey(imei);
  char * password = generatePassword(imei, serverId, key);

  if (key != NULL) {
    DmFreeMem(key);
    key = NULL;
  }    

  return password;
}

/**
 *  Generate a client password using a generated client password key, the IMEI, and
 *  the server ID.
 *
 *  @param imei a phone identification number
 *  @param serverId the serverId that represent a DM server
 *  @return the client password
 */ 
char * GeneratePassword::generateClientPassword(const char * imei, const char * serverId) {
  char * key = generateClientPasswordKey(imei);
  char * password = generatePassword(imei, serverId, key);

  if (key != NULL) {
    DmFreeMem(key);
    key = NULL;
  }    

  return password;
}
  
/**
 *  Generate a server password using a generated server password key, the IMEI, and
 *  the server ID.
 *
 *  @param imei a phone identification number
 *  @param serverId the serverId that represent a DM server
 *  @return the server password
 */   
char * GeneratePassword::generateServerPassword(const char * imei, const char * serverId) {
  char * key = generateServerPasswordKey(imei);
  char * password = generatePassword(imei, serverId, key);

  if (key != NULL) {
    DmFreeMem(key);
    key = NULL;
  }    

  return password;
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
char * GeneratePassword::generateKeyFromDict(const char * imei, const char dict[]) {
  UINT16 i;
  UINT16 length; 
  UINT64 serial1 = 0;
  UINT64 serial2 = 0;
  char * serial1_str;
  char * serial2_str;
  char * key;

  length = DmStrlen(imei);

  for (i = 0; i < length - 3; i++) {
    serial1 += imei[i + 3] * dict[i];
    serial2 += imei[i + 3] * imei[i + 2] * dict[i];
  }

  serial1_str = (char *) DmAllocMem(sizeof(char) * (24));
  serial2_str = (char *) DmAllocMem(sizeof(char) * (24));
  DmSprintf(serial1_str, "%llu", serial1);
  DmSprintf(serial2_str, "%llu", serial2);

  key = (char *) DmAllocMem(sizeof(char) * (DmStrlen(serial1_str) + DmStrlen(serial2_str) + 2));
  memset(key, '\0', (sizeof(char) * (DmStrlen(serial1_str) + DmStrlen(serial2_str) + 2)));

  DmStrcat(key, (const char *)serial1_str );
  DmStrcat(key, "-");
  DmStrcat(key, (const char *)serial2_str );

  DmFreeMem(serial1_str);
  serial1_str = NULL;
  DmFreeMem(serial2_str);
  serial2_str = NULL;

  return key;
}

/**
 * Convert an array of characters that represents a large number to a decimal number type
 *
 * @param input the array of characters thar represents a large number
 * @return the decimal number
 */
UINT64 GeneratePassword::convertchar2Long(const char * input)
{
  char ch[2];
  UINT16 i;
  UINT64 tmp;       
  tmp = 0;
	
  for (i=0 ; i < DmStrlen((const char *)input) ; i++ ) {
    ch[0]=*(input+i);
    ch[1]='\0';
    tmp = (UINT64) ( ( (tmp) * 10) + atol((const char *)ch) );
  }
  return tmp;
}

/**
 * Convert an array of characters that represents a number in decimal based to 36 based.
 *
 * @param target_imei the array of characters thar represents a number in decimal based
 * @param the 36 based number represented by array of characters.
 */
char * GeneratePassword::get36BasedNumber(const char * target) {
  
  char NumericBaseData[]= "0123456789abcdefghijklmnopqrstuvwxyz";
  char tmp_number[11];
  UINT64 Quotient;
  UINT32 Remainder;
  UINT16 i;
  char tmpchar;
  
  char * number36 = (char *) DmAllocMem(sizeof(char) * ( 10 + 1 ));
  memset(number36, '\0', (sizeof(char) * (10 + 1)));

  UINT64 number = 0;
  number = convertchar2Long(target);

  i=0;
  while ( number > 0 ) {
    Quotient = (UINT64)(number/36);
    Remainder = (UINT32)(number%36);    
    tmp_number[i++] = NumericBaseData[Remainder];    
    number = Quotient;
  }
  tmp_number[i]='\0';
  
  //If the length is <10 pad the remaining chracter to '0' to make the length 10
  if( DmStrlen(tmp_number) < 10 ) {
    for (i=DmStrlen(tmp_number); i<10 ; i++) {
      tmp_number[i]='0';
    }
    tmp_number[i]='\0';
  }
  
  for( i=0 ; i < DmStrlen(tmp_number)/2 ; i++ ) {
    tmpchar = tmp_number[i];
    tmp_number[i] = tmp_number[ DmStrlen(tmp_number)-i-1];
    tmp_number[ DmStrlen(tmp_number)-i-1] = tmpchar;
  }

  memcpy(number36,tmp_number,DmStrlen(tmp_number));
  return number36;
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

  UINT16 length;
  UINT16 secondHalfPos;
  UINT16 insertPos;
  char * buf;
  char tmpchar;
  UINT16 i;
  UINT16 j;

  insertPos = 0;
  i=0;
  j=0;  
  buf = & buffer;
  length = DmStrlen((char *)buf);
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
char * GeneratePassword::encodeHex(const char data[]) {

  UINT16 i;
  char tmpchar;
  UINT16 len = MD5_HASH_LENGTH;
  UINT16 size = len * 2 + 1;
  char * output = (char *) DmAllocMem(sizeof(char) * size);
  memset(output, '\0', (sizeof(char) * size));

  for ( i = 0; i < len ; i++) {
    tmpchar = data[i];
    output[2*i] = hexTable[ (tmpchar & 0x0F) ];		// Get low 4 bits
    output[(2*i)+1] = hexTable[ ((tmpchar >> 4) & 0x0F ) ];	// Get high 4 bits
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
char * GeneratePassword::generatePassword(const char * imei, const char * serverId, const char * key) {

  char * MD5DigestStr = (char *) DmAllocMem(sizeof(char) * (DmStrlen ((const char *)imei) + DmStrlen(key) + DmStrlen(serverId) +1));
  memset( MD5DigestStr , '\0', (sizeof(char) * ( DmStrlen ((const char *)imei) + DmStrlen(key) + DmStrlen(serverId) +1 ) ) );
  
  DmStrcpy( MD5DigestStr , (const char *)imei );
  DmStrcat( MD5DigestStr , key );
  DmStrcat( MD5DigestStr , serverId );
  
  MD5_CTX md5_context;

  char md5hash[MD5_HASH_LENGTH + 1];  /* Add 1 character for NULL */
  memset(md5hash, '\0', (sizeof(char) * (MD5_HASH_LENGTH + 1)));
  
  smlMD5Init(&md5_context);
  smlMD5Update(&md5_context, (unsigned char *)MD5DigestStr,DmStrlen(MD5DigestStr));
  smlMD5Final((unsigned char*)md5hash, &md5_context);
  md5hash[MD5_HASH_LENGTH] = 0;
    
  DmFreeMem(MD5DigestStr );
  MD5DigestStr =NULL;
  
  char * MD5DigestStr32 = encodeHex(md5hash);
  
  // Pick only no. 2,7,8,12,25,30 characters from MD5 Digest String
	
  MD5DigestStr = (char *) DmAllocMem(sizeof(char) * ( 6 + 1 ));
  memset(MD5DigestStr, '\0', (sizeof(char) * ( 6 + 1)));
  
  MD5DigestStr[0] = MD5DigestStr32[2];
  MD5DigestStr[1] = MD5DigestStr32[7];
  MD5DigestStr[2] = MD5DigestStr32[8];
  MD5DigestStr[3] = MD5DigestStr32[12];
  MD5DigestStr[4] = MD5DigestStr32[25];
  MD5DigestStr[5] = MD5DigestStr32[30];
  
  DmFreeMem(MD5DigestStr32 );
  MD5DigestStr32 =NULL;

  //Convert IMEI to 36(base) radixnumber to generate NewIMEI
  
  char * IMEI36 = NULL; 
  
  IMEI36 = get36BasedNumber(imei);

  //Create Password

  UINT16 PWLength = DmStrlen (MD5DigestStr) + DmStrlen((const char *)IMEI36);  //Length will be 16

  char * password = (char *) DmAllocMem(sizeof(char) * ( PWLength + 1));
  memset( password , '\0', (sizeof(char) * ( PWLength + 1 ) ) ); //this will be of 16 + 1
  
  DmStrcpy( password , MD5DigestStr );
  DmStrcat( password , (const char *)IMEI36 );
  
  DmFreeMem(MD5DigestStr );
  MD5DigestStr =NULL;
  
  DmFreeMem(IMEI36);
  IMEI36 =NULL;

  DmFreeMem(MD5DigestStr32 );
  MD5DigestStr32 =NULL;

  shuffle(*password);
  shuffle(*password);
  shuffle(*password);
 
  return password;
}
