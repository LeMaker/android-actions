
/*************************************************************************/
/* module:          Communication Services, base64 encoding/decoding fns.*/
/* file:            src/xpt/all/xpt-b64.c                                */
/* target system:   all                                                  */
/* target OS:       all                                                  */
/*************************************************************************/


/*
 * Copyright Notice
 * Copyright (c) Ericsson, IBM, Lotus, Matsushita Communication 
 * Industrial Co., Ltd., Motorola, Nokia, Openwave Systems, Inc., 
 * Palm, Inc., Psion, Starfish Software, Symbian, Ltd. (2001).
 * All Rights Reserved.
 * Implementation of all or part of any Specification may require 
 * licenses under third party intellectual property rights, 
 * including without limitation, patent rights (such a third party 
 * may or may not be a Supporter). The Sponsors of the Specification 
 * are not responsible and shall not be held responsible in any 
 * manner for identifying or failing to identify any or all such 
 * third party intellectual property rights.
 * 
 * THIS DOCUMENT AND THE INFORMATION CONTAINED HEREIN ARE PROVIDED 
 * ON AN "AS IS" BASIS WITHOUT WARRANTY OF ANY KIND AND ERICSSON, IBM, 
 * LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO. LTD, MOTOROLA, 
 * NOKIA, PALM INC., PSION, STARFISH SOFTWARE AND ALL OTHER SYNCML 
 * SPONSORS DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION 
 * HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF 
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
 * SHALL ERICSSON, IBM, LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO., 
 * LTD, MOTOROLA, NOKIA, PALM INC., PSION, STARFISH SOFTWARE OR ANY 
 * OTHER SYNCML SPONSOR BE LIABLE TO ANY PARTY FOR ANY LOSS OF 
 * PROFITS, LOSS OF BUSINESS, LOSS OF USE OF DATA, INTERRUPTION OF 
 * BUSINESS, OR FOR DIRECT, INDIRECT, SPECIAL OR EXEMPLARY, INCIDENTAL, 
 * PUNITIVE OR CONSEQUENTIAL DAMAGES OF ANY KIND IN CONNECTION WITH 
 * THIS DOCUMENT OR THE INFORMATION CONTAINED HEREIN, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH LOSS OR DAMAGE.
 * 
 * The above notice and this paragraph must be included on all copies 
 * of this document that are made.
 * 
 */


#include "xptport.h"
#include "xpttypes.h"
#include "xpt-b64.h"


#define MAX_COLUMNS 45


/***************************************************************************/
/* The function decodes the next character of the input buffer and updates */
/* the pointers to the input buffer. The function skips CRLF characters    */
/* and whitespace characters.                                              */
/* Returns: 0..63, the logical value of the next valid character in the    */
/*                 input buffer                                            */
/*          64,    padding character                                       */
/*          -1,    No more characters to read                              */
/*          -2,    an error occurred: invalid characters in the data stream*/
/***************************************************************************/

int nextBase64Char (DataBuffer_t *ppbData, BufferSize_t *pcbDataLength)
   {
   int r = -1;
   DataBuffer_t pbData = *ppbData;
   BufferSize_t cbDataLength = *pcbDataLength;
#ifndef __EPOC_OS__
   static char *pszSkipChars = "\t\r\n ";
#endif
#ifdef __EPOC_OS__
   char *pszSkipChars = "\t\r\n ";
#endif
   char ch;

   if (cbDataLength == 0) return r;

   do {
      ch = *pbData;
      if ((ch>='0')&&(ch<='9'))      r = (int)ch+4;  //  |
      else if ((ch>='A')&&(ch<='Z')) r = (int)ch-65; //  |
      else if ((ch>='a')&&(ch<='z')) r = (int)ch-71; //  | Valid characters
      else if (ch == '/')            r = 63;         //  |
      else if (ch == '+')            r = 62;         //  |
      else if (ch == '=')            r = 64;         // padding character
      else if (!xppStrchr (pszSkipChars, ch))  r = -2; // invalid character
      cbDataLength --;
      pbData ++;
      } while ((r == -1) && (cbDataLength > 0));

   /***************************************************/
   /* Pass the updated parameter values to the caller */
   /***************************************************/
   if (r != -1)
      {
      *ppbData = pbData;
      *pcbDataLength = cbDataLength;
      }
   return r;
   }

BufferSize_t base64GetSize (BufferSize_t cbRealDataSize)
   {
   int iMod = cbRealDataSize % 3;
   /* The encoded data size ... */
   BufferSize_t cbEncodedSize = ((cbRealDataSize - iMod) / 3 ) * 4;
   if (iMod != 0) cbEncodedSize += 4;
   /* ... and the # of CRLF characters */
   cbEncodedSize += ((cbEncodedSize-1) / ((MAX_COLUMNS*4)/3)) * 2;
   return cbEncodedSize;
   }


/*****************************************************************/
/* Function: pre-compute the size of the base64 encoded document */
/****************************************************************/

BufferSize_t base64Encode (DataBuffer_t pbTarget,       // o: target buffer
                     BufferSize_t cbTargetSize,   // i: target buffer size
                     DataBuffer_t pbData,         // i: Data buffer
                     BufferSize_t *pcbDataLength, // i/o: Data buffer size
                     BufferSize_t *pcbOffset,     // i/o: absolute # of bytes encoded so far
                     unsigned int bLast,          // i: 0=first block, 1= next block, 2=last block
                     unsigned char *pbSaveBytes)  // i/o: last incomplete data block
   {
   DataBuffer_t pbSourceData = pbData;
   BufferSize_t cbCopySize = 0;
   BufferSize_t cbDataProcessed = *pcbDataLength;
   unsigned int i0, i1, i2, i3;
   unsigned int byt;
   int iSave = 1;
#ifndef __EPOC_OS__
   static char t [] =   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"    // 26
                        "abcdefghijklmnopqrstuvwxyz"    // 26
                        "0123456789+/"                  // 12
                        "=";                            // 1
#endif
#ifdef __EPOC_OS__
char t [] =   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"    // 26
              "abcdefghijklmnopqrstuvwxyz"    // 26
              "0123456789+/"                  // 12
              "=";                            // 1
#endif
   // Check for NULL data buffer,
   if (pbData == NULL ) {
       // See if last block and there is any "saved" data that needs to go now.
       if ( bLast && ( pbSaveBytes && pbSaveBytes [0] )) {
          /**************************************/
          /* Check if it is time to send a CRLF */
          /**************************************/
          if ((*pcbOffset) > 0 && ((*pcbOffset) % MAX_COLUMNS == 0))
             {
             //if (cbTargetSize < 6)        // there is not enough space in the target buffer:
             //   break;                    // return to the caller.
             *pbTarget = '\r';
             *(pbTarget+1) = '\n';
             cbCopySize += 2;
             cbTargetSize -= 2;
             pbTarget += 2;
             }

          byt = (unsigned int) pbSaveBytes [iSave]; iSave ++; pbSaveBytes [0] --;

          i0 =  byt >> 2;
          i1 =  (byt & 0x0003) << 4;

          (*pcbOffset) ++;

          if (pbSaveBytes && pbSaveBytes [0]) {
              byt = (unsigned int) pbSaveBytes [iSave]; iSave ++; pbSaveBytes [0] --;
              i1 += (byt >> 4);
              i2 =  (byt & 0x000F) << 2;
          } else {
              i2 = i3 = 64;  // index to the padding char '=';
          }

          pbTarget [0] = t[i0];
          pbTarget [1] = t[i1];
          pbTarget [2] = t[i2];
          pbTarget [3] = t[i3];

          cbCopySize += 4;
          cbTargetSize -= 4;
          pbTarget += 4;
       }

   } else {
       while ((cbTargetSize >= 4) &&
              ( ((cbDataProcessed >= 3) && (bLast == 0)) ||
                ((cbDataProcessed >  0) && (bLast == 1)) ))
          {
          /**************************************/
          /* Check if it is time to send a CRLF */
          /**************************************/
          if ((*pcbOffset) > 0 && ((*pcbOffset) % MAX_COLUMNS == 0))
             {
             if (cbTargetSize < 6)        // there is not enough space in the target buffer:
                break;                    // return to the caller.
             *pbTarget = '\r';
             *(pbTarget+1) = '\n';
             cbCopySize += 2;
             cbTargetSize -= 2;
             pbTarget += 2;
             }

          if (pbSaveBytes && pbSaveBytes [0])
             { byt = (unsigned int) pbSaveBytes [iSave]; iSave ++; pbSaveBytes [0] --; }
          else
             { byt = (unsigned int) *pbSourceData; pbSourceData ++; cbDataProcessed --;}

          i0 =  byt >> 2;
          i1 =  (byt & 0x0003) << 4;

          (*pcbOffset) ++;

          if (cbDataProcessed > 0)
             {
             if (pbSaveBytes && pbSaveBytes [0])
                { byt = (unsigned int) pbSaveBytes [iSave]; iSave ++; pbSaveBytes [0] --; }
             else
                { byt = (unsigned int) *pbSourceData; pbSourceData ++; cbDataProcessed --;}

             i1 += (byt >> 4);
             i2 =  (byt & 0x000F) << 2;

             (*pcbOffset) ++;

             if (cbDataProcessed > 0)
                {
                if (pbSaveBytes && pbSaveBytes [0])
                   { byt = (unsigned int) pbSaveBytes [iSave]; iSave ++; pbSaveBytes [0] --; }
                else
                   { byt = (unsigned int) *pbSourceData; pbSourceData ++; cbDataProcessed --;}

                i2 += (byt & 0x00C0) >> 6;
                i3 =  byt & 0x003F;
                (*pcbOffset) ++;
                }
             else
                i3 = 64;  // index to the padding char '=';
             }
          else
             i2 = i3 = 64;  // index to the padding char '=';
          pbTarget [0] = t[i0];
          pbTarget [1] = t[i1];
          pbTarget [2] = t[i2];
          pbTarget [3] = t[i3];

          cbCopySize += 4;
          cbTargetSize -= 4;
          pbTarget += 4;
       }
   }



   /*************************************************************/
   /* Save the bytes that must be processed in the following    */
   /* call (max. 2 Bytes).                                      */
   /*************************************************************/
   if ((bLast == 0) && (cbDataProcessed <= 2) && (pbSaveBytes != NULL))
      {
      pbSaveBytes[0] = cbDataProcessed;
      while (cbDataProcessed)
         {
         *(++pbSaveBytes) = pbSourceData[0];
         cbDataProcessed --; pbSourceData ++;
         }
      }

   /*****************************************************************/
   /* Shift all non-processed data to the start of the input buffer */
   /*****************************************************************/

   if (cbDataProcessed > 0)
      {
      xppMemmove (pbData, pbSourceData, cbDataProcessed);
      }
   *pcbDataLength = cbDataProcessed;

   return cbCopySize;
   }


/***************************************************************/
/* Function: decode a base64- encoded block of data.           */
/* The function returns the count of data that are decoded, or */
/* 0 in case of a data error, or if cbTargetSize < 4           */
/***************************************************************/

BufferSize_t base64Decode (DataBuffer_t pbTarget,       // o: target buffer
                     BufferSize_t cbTargetSize,   // i: target buffer size
                     DataBuffer_t pbData,         // i: data buffer
                     BufferSize_t *pcbDataLength) // i/o: Data buffer size
   {
   DataBuffer_t pbSource = pbData;
   BufferSize_t cbDataCopied = 0L;
   BufferSize_t cbRemaining = *pcbDataLength; // remaining source data
   int i0 = 0, i1 = 0, i2 = 0, i3 = 0;

   while (cbTargetSize > 0)
      {
      BufferSize_t cbNext = cbRemaining;
      DataBuffer_t pbNext = pbSource;

      i0 = nextBase64Char (&pbNext, &cbNext);
      i1 = nextBase64Char (&pbNext, &cbNext);
      i2 = nextBase64Char (&pbNext, &cbNext);
      i3 = nextBase64Char (&pbNext, &cbNext);
      if ((i0 < 0) || (i1 < 0) || (i2 < 0) || (i3 < 0))
         break; // end-of-block, or data error.

      else if ( ((cbTargetSize <= 2) && (i3 != 64)) ||
                ((cbTargetSize <= 1) && (i2 != 64)) )
         break; // end of transmission.

      else
         {
         pbSource = pbNext;
         cbRemaining = cbNext;
         /************************/
         /* decode the quadruple */
         /************************/
         *pbTarget = (i0 << 2) + (i1 >> 4);
         pbTarget ++; cbDataCopied ++; cbTargetSize --;
         if (i2 != 64)
            {
            *pbTarget = ((i1 & 0x000f) << 4) + (i2 >> 2);
            pbTarget ++; cbDataCopied ++; cbTargetSize --;
            if (i3 != 64)
               {
               *pbTarget = ((i2 & 0x0003) << 6) + i3;
               pbTarget ++; cbDataCopied ++; cbTargetSize --;
               }
            }
         }
      }

   /*******************************/
   /* Handle invalid data errors! */
   /*******************************/

   if ((i0 == -2) || (i1 == -2) || (i2 == -2) || (i3 == -2))
      cbDataCopied = 0;

   /*****************************************************************/
   /* Shift all non-processed data to the start of the input buffer */
   /*****************************************************************/
   if (cbRemaining > 0)
      xppMemmove (pbData, pbSource, cbRemaining);
   *pcbDataLength = cbRemaining;
   return cbDataCopied;
   }


