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
//   Module Name:main.c 
//
//   General Description: This file contains the main function for the SyncML factory boot strap credential generator tool 
//
//--------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "xptport.h"
#include "xpttypes.h"
#include "xpt-b64.h"
#include "md5.h"
#define MD5_HASH_LENGTH 16
#define ENCODED_MD5_CRED_LENGTH (MD5_HASH_LENGTH + (MD5_HASH_LENGTH/2) + 1)

#define getch getchar

void SyncML_DM_CombineCredDataAndIMEI(char *p_CreadData_IMEI,const char *p_CreadData,const char *p_IMEI);
void SyncML_DM_BuildB64EncodedMD5_Factory_Bootstrap_CredData(char **pp_credential_data,const char *p_CredData,const char *p_IMEI );
void SyncML_DM_BuildB64EncodedCredData_Factory_Bootstrap(char **pp_credential_data,const char *pCread_Input);
void user_help(void);

const char DM_STR_SLASH[]       = "/";

/*==================================================================================================

FUNCTION:SyncML_DM_BuildB64EncodedMD5_Factory_Bootstrap_CredData

DESCRIPTION:
   This function builds the B64 encoded MD5 credential information.
   The credential string is credential data:IMEI number . That string is then b64 encoded.

ARGUMENTS PASSED:
   
REFERENCE ARGUMENTS PASSED:
   char **pp_credential_data  - Output variable containing the encoded credential data
   const char *p_CredData - credential_data  .
   const char *p_IMEI - Phone IMEI number.
  

RETURN VALUE:
   N/A

PRE-CONDITIONS:

POST-CONDITIONS:
  
==================================================================================================*/



void SyncML_DM_BuildB64EncodedMD5_Factory_Bootstrap_CredData(char **pp_credential_data,const char *p_CredData,const char *p_IMEI )
 {
   char *p_CreadData_IMEI;
   
   MD5_CTX md5_context;
   char md5hash[MD5_HASH_LENGTH + 1];  /* Add 1 character for NULL */
   BufferSize_t offset = 0;
   BufferSize_t md5_hash_length = MD5_HASH_LENGTH;
   BufferSize_t total_length;

   memset(md5hash, '\0', (sizeof(char) * (MD5_HASH_LENGTH + 1)));

   *pp_credential_data = (char *)malloc(sizeof(char) * (ENCODED_MD5_CRED_LENGTH));
   memset(*pp_credential_data, '\0', (sizeof(char) * ENCODED_MD5_CRED_LENGTH));
   /* Add space for the ":" and NULL character */
   total_length= strlen(p_CredData) + strlen(p_IMEI) + 2;
   p_CreadData_IMEI = (char *) malloc(sizeof(char) * (total_length));
   memset(p_CreadData_IMEI, '\0', (sizeof(char) * total_length));
   SyncML_DM_CombineCredDataAndIMEI(p_CreadData_IMEI, p_CredData,p_IMEI);

   smlMD5Init(&md5_context);
   smlMD5Update(&md5_context, (unsigned char *)p_CreadData_IMEI,strlen(p_CreadData_IMEI));
   smlMD5Final((unsigned char*)md5hash, &md5_context);
   md5hash[MD5_HASH_LENGTH] = 0;
   free(p_CreadData_IMEI);
   p_CreadData_IMEI=NULL;
   base64Encode((unsigned char *)*pp_credential_data, ENCODED_MD5_CRED_LENGTH,
                (unsigned char *)md5hash,&md5_hash_length,&offset,
                1, /* Encode as single block */
                NULL); /* No incomplete bl*/

 }

/*==================================================================================================

FUNCTION: SyncML_DM_CombineCredDataAndIMEI

DESCRIPTION:
   This function combines the credential data  and IMEI  strings
   The credential string is CreadData:IMEI.

ARGUMENTS PASSED:
   
REFERENCE ARGUMENTS PASSED:
   char *p_CreadData_IMEI - Output variable containing the combined credential data  and IMEI
                               string "CreadData:IMEI"
   const char *p_CreadData - credential data
   const char *p_IMEI - phone IMEI no.

RETURN VALUE:
   N/A

PRE-CONDITIONS:

POST-CONDITIONS:

INVARIANTS:
==================================================================================================*/
 void SyncML_DM_CombineCredDataAndIMEI(char *p_CreadData_IMEI,const char *p_CreadData,const char *p_IMEI)
 {
   strcpy(p_CreadData_IMEI, p_CreadData);
   strcat(p_CreadData_IMEI, ":");
   strcat(p_CreadData_IMEI, p_IMEI);
}
/*==================================================================================================

FUNCTION: SyncML_DM_BuildB64EncodedCredData_Factory_Bootstrap

DESCRIPTION:
   This function builds the B64 encoded basic credential information.
   The credential string is username:password. That string is then
   B64 encoded.

ARGUMENTS PASSED:
  
REFERENCE ARGUMENTS PASSED:
   char **p_credential_data  - Output variable containing the encoded credential data


    const char *pCread_Input
RETURN VALUE:
   N/A

PRE-CONDITIONS:

POST-CONDITIONS:

INVARIANTS:
   None
==================================================================================================*/


void SyncML_DM_BuildB64EncodedCredData_Factory_Bootstrap(char **pp_credential_data,const char *pCread_Input)

{
 
   BufferSize_t offset = 0;
   BufferSize_t combined_length = 0;
   long total_length;
    long estimated_basic_cred_length;

   /* Add space for the ":" and NULL character */
   total_length = strlen(pCread_Input) + 1;
   /* RFC 2045 (First paragraph of Section 6.8)
    * "The encoding and decoding algorithms are simple, but the
    * encoded data are consistently only about 33% larger than the
    * unencoded data.". Calculate the estimated credential length
    * by dw_length + (dw_length/2)
    */
   estimated_basic_cred_length = total_length + (total_length/2);

   *pp_credential_data = (char *)malloc(sizeof(char) * estimated_basic_cred_length);
   memset(*pp_credential_data, '\0', (sizeof(char) * estimated_basic_cred_length));


   combined_length = strlen(pCread_Input);

   base64Encode((unsigned char *)*pp_credential_data, estimated_basic_cred_length,
       (unsigned char *)pCread_Input, &combined_length, &offset,
                1,  /* Encode as single block */
                NULL); /* No incomplete blocks */

 }

void user_help(void)
{

 printf("\nGen_Cred  < Input File Path>  < Input Filename> \n\n");
 
}


 int  main(int argc, char *argv[])
 {
   int  bytes_read;     
   int  total_length;
   int  filepath_length;
   char *p_Output_string;
   char pServerID[150];
   char pServer_Nonce[]="14YJ55NI65RS25WA";
   char pClient_Nonce[]="53LI62MI23HE33ST";
   char  pUsername_text[]="UserName";
   char *pUsername;
   char *pFilepath;
   char  pOutputFile[40];
   char *ptemp_input;
   char pcred[]="_Cred-";

   struct tm *stTm = NULL; 
   time_t Time; 
  // int err = noError;*/
 
   
  
  FILE *fd_IMEI,*fd_Cred;
    
  unsigned char pIMEI[16];

   if(argc < 3)
   {
    user_help();
    printf("\n ENTER A KEY TO EXIT \n");
    getch();
    return 0;
   }
   else
   {
       
       filepath_length=strlen(argv[1])+ strlen(argv[2]);
       pFilepath=(char *)malloc((filepath_length * sizeof(char ))+2);
       sprintf(pFilepath, "%s%s%s", argv[1],DM_STR_SLASH, argv[2]);
             
   }

   //fd_IMEI=fopen("IMEI.txt","r");
   fd_IMEI=fopen(pFilepath,"r");
    if(fd_IMEI == NULL)
    {
         printf("file fd_IMEI not opened\n");
         printf("\n ENTER A KEY TO EXIT \n");
         getch();
         free(pFilepath);
         return 0;

    }
    else
    {
        printf ("\nfile fd_IMEI opened  \n");
        printf(" \n\nENTER THE SERVER ID \n\n");
        scanf("%s",pServerID);
    /*    printf(" \n\nENTER THE SERVER NONCE \n\n");
        scanf("%s",pServer_Nonce);
        printf(" \n\nENTER THE CLIENT NONCE \n\n");
        scanf("%s",pClient_Nonce);*/
    
        if(strlen(pServerID) >= 136)
        {
         printf("\n\nLENGTH OF SERVERID IS GRETER THAN 135  IT SHOULD BE LTE 135 \n\n");
         printf("\n\nSERVER ID ENTERED == %s length == %d\n\n",pServerID,strlen(pServerID));
         printf("\n ENTER A KEY TO EXIT \n");
         getch();
         free(pFilepath);
         return 0;
        }

        printf("\n\nSERVER ID ENTERED == %s length == %d\n\n",pServerID,strlen(pServerID));
      
    }


        time(&Time); 
        stTm = localtime(&Time);
  

       // filepath_length=(strlen(argv[2])-4)+ (sizeof(int) * 6)+1;
        ptemp_input=(char *)malloc(((strlen(argv[2])-4) * sizeof(char))+1);
    //    memset(ptemp_input, '\0', (sizeof(char) * estimated_basic_cred_length));
        memcpy(ptemp_input,argv[2],(strlen(argv[2])-4));
        ptemp_input[strlen(argv[2])-4]='\0';
       // pOutputFile=(char *)malloc(filepath_length * sizeof(char));

        sprintf(pOutputFile,"%s%s%02d.%02d.%02d-%02d-%02d-%04d.txt",ptemp_input,pcred,stTm->tm_hour,stTm->tm_min,
         stTm->tm_sec,stTm->tm_mday,stTm->tm_mon+1,stTm->tm_year+1900);
        free(ptemp_input);
      
       
   fd_Cred=fopen(pOutputFile,"w+");
   
    if(fd_Cred ==  NULL )

    {
         printf("file fd_Cred not opened\n");
         printf("\n ENTER A KEY TO EXIT \n");
         free(pFilepath);
         //free(pOutputFile);
         

         getch();
         
         return 0;

    }
    else
        printf ("\nfilew fd_Cred opened \n");

  
  
  while(!feof(fd_IMEI))
  {
     bytes_read=fread(pIMEI,sizeof(char),16,fd_IMEI); //read from file
        
        pIMEI[15]='\0';
  
     /* WRITE IMEI TO THE FILE */
        fwrite(pIMEI,sizeof(char),strlen(pIMEI),fd_Cred);
       fwrite(" ",sizeof(char),1,fd_Cred);
    
       SyncML_DM_BuildB64EncodedMD5_Factory_Bootstrap_CredData(&p_Output_string, (const char *)pServerID,(const char *)pIMEI);
        
        /* Generate Server PW */
        

        
        //x.EncodeBuffer((char *)digest,sizeof(digest) /*strlen((char *)digest)*/,output_string);

        fwrite(p_Output_string,sizeof(char),strlen(p_Output_string),fd_Cred);
        
        fwrite(" ",sizeof(char),1,fd_Cred);
        free(p_Output_string);


        /* Generate Client  PW */

        SyncML_DM_BuildB64EncodedMD5_Factory_Bootstrap_CredData(&p_Output_string, (const char *)pIMEI,(const char *)pServerID);
       

        fwrite(p_Output_string,sizeof(char),strlen(p_Output_string),fd_Cred);
        
        fwrite(" ",sizeof(char),1,fd_Cred);
        free(p_Output_string);

        /*Generate UserName */
        total_length=strlen(pUsername_text)+strlen(pServerID)+2;


        pUsername=(char *)malloc(total_length * sizeof(char));
        memset(pUsername,'\0',total_length);
        strcpy(pUsername,pServerID);
        strcat(pUsername,":");
        strcat(pUsername,pUsername_text);

        SyncML_DM_BuildB64EncodedMD5_Factory_Bootstrap_CredData(&p_Output_string, (const char *)pIMEI,(const char *)pUsername);

        fwrite(p_Output_string,sizeof(char),strlen(p_Output_string),fd_Cred);
        
        free(p_Output_string);
        free(pUsername);
        /* NONCE CALCULATION */

        
        fwrite(" ",sizeof(char),1,fd_Cred);
        SyncML_DM_BuildB64EncodedCredData_Factory_Bootstrap(&p_Output_string,(const char *)pServer_Nonce);
        fwrite(p_Output_string,sizeof(char),strlen(p_Output_string),fd_Cred);
        fwrite(" ",sizeof(char),1,fd_Cred);
        free(p_Output_string);
        SyncML_DM_BuildB64EncodedCredData_Factory_Bootstrap(&p_Output_string,(const char *)pClient_Nonce);
        fwrite(p_Output_string,sizeof(char),strlen(p_Output_string),fd_Cred);
        free(p_Output_string);
        
        fwrite("\n",sizeof(char),1 ,fd_Cred);

    
    }

       printf("\n\nGENERATED CRED VALUES  EACH SEPARATED BY SPACE CHAR  IN %s \n",pOutputFile);
       printf("\nIMEI  ServerPW   ClientPW   UserName  ServerNonce  ClientNonce \n\n\n\n");
       fclose(fd_Cred);
       fclose(fd_IMEI);

       printf("\n ENTER A KEY TO EXIT \n");
       free(pFilepath);
       //free(pOutputFile);

       getch();

  return 0 ;
}
