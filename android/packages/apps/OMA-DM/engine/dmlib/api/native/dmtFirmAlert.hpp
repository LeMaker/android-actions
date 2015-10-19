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

#ifndef __DMTFIRMALERT_H__
#define __DMTFIRMALERT_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
 \file dmtFirmAlert.hpp
 \brief  The dmtFirmAlert.hpp header file contains DmtFirmAlert class definition. \n
           This class is a helper class for sending repair status with ALERT 1226.\n 
          <b>Warning:</b>  All functions, structures, and classes from this header file are for internal usage only!!!
*/

#include "jem_defs.hpp"

/**
 * Helper class for sending repair status with ALERT 1226
 * \warning This class is using <b>ONLY</b> for firmware update session!
 * \par Category: General  
 * \par Persistence: Transient
 * \par Security: Non-Secure
 * \par Migration State: FINAL
 */
class DmtFirmAlert
{ 
private:
  DMString  m_strPackageURI;                // URI of update package
  DMString  m_strCorrelator;
  DMString  m_strResultData;
  DMString  m_strAlertType;
  DMString  m_strAlertFormat;
  DMString  m_strAlertMark;

public:
  /**
  * Default constructor - no memory allocation performed.
  */
   DmtFirmAlert()
  {
  }

  /**
  * Constructor Firm Alert base on the parameters. The memory will be allocated.
  * \param packageURI [in] - package URI, constant character pointer.
  * \param resultData [in] - result data, constant character pointer.
  * \param alertType [in] - alert type, constant character pointer.
  * \param alertFormat [in] - alert format, constant character pointer.
  * \param alertMark [in] - alert mark, constant character pointer.
  * \param szCorrelator [in] - correlator, constant character pointer.
  */
  DmtFirmAlert(
      const char* packageURI, 
      const char* resultData, 
      const char* alertType, 
      const char* alertFormat,
      const char* alertMark,
      const char* szCorrelator )
  {
      m_strPackageURI = packageURI;
      m_strCorrelator = szCorrelator;
      m_strResultData = resultData;
      m_strAlertType = alertType;
      m_strAlertFormat = alertFormat;
      m_strAlertMark = alertMark;
  }
  
  
 /** 
  * Retrieves alert type
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \returns string alert type 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 const DMString & getAlertType() const
 {
    return m_strAlertType;
 }

 /**
  * Retrieves alert format
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \returns alert format
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 const DMString& getAlertFormat() const
 {
    return m_strAlertFormat;
 }

 /**
  * Retrieves alert mark
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \returns alert mark
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 const DMString& getAlertMark() const
 {
    return m_strAlertMark;
 }


 /**
  * Retrieves result data
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \returns firmware update result code 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 const DMString& getResultData() const
 {
    return m_strResultData;
 }


 /**
  * Retrieves package URI
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \returns update package URI in DMString.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 const DMString & getPackageURI() const
 {
    return m_strPackageURI;
 }

 /**
  * Retrieves correlator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \returns Correlator in DMString.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 const DMString & getCorrelator() const
 {
    return m_strCorrelator;
 }


 /**
  * Sets alert type
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param alertType [in] - alert type as a string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 SYNCML_DM_RET_STATUS_T setAlertType(const char* alertType)
 {
    m_strAlertType = alertType;
    return SYNCML_DM_SUCCESS;
 }

 /**
  * Sets alert format
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param alertFormat [in] - alert format as a string
  * \return  Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 SYNCML_DM_RET_STATUS_T setAlertFormat(const char* alertFormat)
 {
    m_strAlertFormat = alertFormat;
    return SYNCML_DM_SUCCESS;
 }

 /**
  * Sets alert mark
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param alertMark [in] - alert mark as a string
  * \return  Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 SYNCML_DM_RET_STATUS_T setAlertMark(const char* alertMark)
 {
    m_strAlertMark = alertMark;
    return SYNCML_DM_SUCCESS;
 }


 /**
  * Sets result code
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param resultData [in] -  result data as a string, usually a result code
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 SYNCML_DM_RET_STATUS_T setResultData(const char* resultData )
 {
    m_strResultData = resultData;
    return SYNCML_DM_SUCCESS;
 }

 /**
  * Sets correlator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param correlator [in] - correlator as a string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 SYNCML_DM_RET_STATUS_T setCorrelator(const char* correlator )
 {
    m_strCorrelator= correlator;
    return SYNCML_DM_SUCCESS;
 }

 /**
  * Sets package URI
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param strPackageURI [in] - package URI as a string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_INVALID_PARAMETER - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 SYNCML_DM_RET_STATUS_T setPackageURI(const DMString & strPackageURI)
 {
    if ( !strPackageURI.length() )
      return SYNCML_DM_INVALID_PARAMETER; 
    
    m_strPackageURI = strPackageURI;
    return SYNCML_DM_SUCCESS;
 }
 
};

#endif   //End of include file
