/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bte_conf.c
 *
 *  Description:   Contains functions to conduct run-time module configuration
 *                 based on entries present in the .conf file
 *
 ******************************************************************************/

#define LOG_TAG "bte_conf"

#include <utils/Log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "bt_target.h"


/******************************************************************************
**  Externs
******************************************************************************/
extern BOOLEAN hci_logging_enabled;
extern char hci_logfile[256];
extern BOOLEAN trace_conf_enabled;
extern BOOLEAN trace_h5_enabled;

void bte_trace_conf(char *p_name, char *p_conf_value);
int device_name_cfg(char *p_conf_name, char *p_conf_value);
int device_class_cfg(char *p_conf_name, char *p_conf_value);
int logging_cfg_onoff(char *p_conf_name, char *p_conf_value);
int logging_set_filepath(char *p_conf_name, char *p_conf_value);
int trace_cfg_onoff(char *p_conf_name, char *p_conf_value);
int trace_h5_onoff(char *p_conf_name, char *p_conf_value);


BD_NAME local_device_default_name = BTM_DEF_LOCAL_NAME;
DEV_CLASS local_device_default_class = {0x40, 0x02, 0x0C};

/******************************************************************************
**  Local type definitions
******************************************************************************/
#define CONF_DBG          0
#define info(format, ...) ALOGI (format, ## __VA_ARGS__)
#define debug(format, ...) if (CONF_DBG) ALOGD (format, ## __VA_ARGS__)
#define error(format, ...) ALOGE (format, ## __VA_ARGS__)

#define CONF_KEY_LEN   32
#define CONF_VALUE_LEN 96

#define CONF_COMMENT '#'
#define CONF_DELIMITERS " =\n\r\t"
#define CONF_VALUES_DELIMITERS "\"=\n\r\t"
#define CONF_COD_DELIMITERS " {,}\t"
#define CONF_MAX_LINE_LEN 255

typedef int (conf_action_t)(char *p_conf_name, char *p_conf_value);

typedef struct {
    const char *conf_entry;
    conf_action_t *p_action;
} conf_entry_t;

typedef struct {
    char key[CONF_KEY_LEN];
    char value[CONF_VALUE_LEN];
} tKEY_VALUE_PAIRS;

enum {
    CONF_DID,
    CONF_DID_RECORD_NUM,
    CONF_DID_PRIMARY_RECORD,
    CONF_DID_VENDOR_ID,
    CONF_DID_VENDOR_ID_SOURCE,
    CONF_DID_PRODUCT_ID,
    CONF_DID_VERSION,
    CONF_DID_CLIENT_EXECUTABLE_URL,
    CONF_DID_SERVICE_DESCRIPTION,
    CONF_DID_DOCUMENTATION_URL,
    CONF_DID_MAX
};
typedef UINT8 tCONF_DID;
/******************************************************************************
**  Static variables
******************************************************************************/

/*
 * Current supported entries and corresponding action functions
 */
/* TODO: Name and Class are duplicated with NVRAM adapter_info. Need to be sorted out */
static const conf_entry_t conf_table[] = {
    /*{"Name", device_name_cfg},
    {"Class", device_class_cfg},*/
    {"BtSnoopLogOutput", logging_cfg_onoff},
    {"BtSnoopFileName", logging_set_filepath},
    {"TraceConf", trace_cfg_onoff},
    {"TraceH5", trace_h5_onoff},
    {(const char *) NULL, NULL}
};

static tKEY_VALUE_PAIRS did_conf_pairs[CONF_DID_MAX] = {
    { "[DID]",               "" },
    { "recordNumber",        "" },
    { "primaryRecord",       "" },
    { "vendorId",            "" },
    { "vendorIdSource",      "" },
    { "productId",           "" },
    { "version",             "" },
    { "clientExecutableURL", "" },
    { "serviceDescription",  "" },
    { "documentationURL",    "" },
};
/*****************************************************************************
**   FUNCTIONS
*****************************************************************************/

int device_name_cfg(char *p_conf_name, char *p_conf_value)
{
    strcpy((char *)local_device_default_name, p_conf_value);
    return 0;
}

int device_class_cfg(char *p_conf_name, char *p_conf_value)
{
    char *p_token;
    unsigned int x;

    p_token = strtok(p_conf_value, CONF_COD_DELIMITERS);
    sscanf(p_token, "%x", &x);
    local_device_default_class[0] = (UINT8) x;
    p_token = strtok(NULL, CONF_COD_DELIMITERS);
    sscanf(p_token, "%x", &x);
    local_device_default_class[1] = (UINT8) x;
    p_token = strtok(NULL, CONF_COD_DELIMITERS);
    sscanf(p_token, "%x", &x);
    local_device_default_class[2] = (UINT8) x;

    return 0;
}

int logging_cfg_onoff(char *p_conf_name, char *p_conf_value)
{
    if (strcmp(p_conf_value, "true") == 0)
        hci_logging_enabled = TRUE;
    else
        hci_logging_enabled = FALSE;
    return 0;
}

int logging_set_filepath(char *p_conf_name, char *p_conf_value)
{
    strcpy(hci_logfile, p_conf_value);
    return 0;
}

int trace_cfg_onoff(char *p_conf_name, char *p_conf_value)
{
    trace_conf_enabled = (strcmp(p_conf_value, "true") == 0) ? TRUE : FALSE;
    return 0;
}

int trace_h5_onoff(char *p_conf_name, char *p_conf_value)
{
    trace_h5_enabled = (strcmp(p_conf_value, "true") == 0) ? TRUE : FALSE;
    return 0;
}

/*****************************************************************************
**   CONF INTERFACE FUNCTIONS
*****************************************************************************/

/*******************************************************************************
**
** Function        bte_load_conf
**
** Description     Read conf entry from p_path file one by one and call
**                 the corresponding config function
**
** Returns         None
**
*******************************************************************************/
void bte_load_conf(const char *p_path)
{
    FILE    *p_file;
    char    *p_name;
    char    *p_value;
    conf_entry_t    *p_entry;
    char    line[CONF_MAX_LINE_LEN+1]; /* add 1 for \0 char */
    BOOLEAN name_matched;

    ALOGI("Attempt to load stack conf from %s", p_path);

    if ((p_file = fopen(p_path, "r")) != NULL)
    {
        /* read line by line */
        while (fgets(line, CONF_MAX_LINE_LEN+1, p_file) != NULL)
        {
            if (line[0] == CONF_COMMENT)
                continue;

            p_name = strtok(line, CONF_DELIMITERS);

            if (NULL == p_name)
            {
                continue;
            }

            p_value = strtok(NULL, CONF_VALUES_DELIMITERS);

            if (NULL == p_value)
            {
                ALOGW("bte_load_conf: missing value for name: %s", p_name);
                continue;
            }

            name_matched = FALSE;
            p_entry = (conf_entry_t *)conf_table;

            while (p_entry->conf_entry != NULL)
            {
                if (strcmp(p_entry->conf_entry, (const char *)p_name) == 0)
                {
                    name_matched = TRUE;
                    if (p_entry->p_action != NULL)
                        p_entry->p_action(p_name, p_value);
                    break;
                }

                p_entry++;
            }

            if ((name_matched == FALSE) && (trace_conf_enabled == TRUE))
            {
                /* Check if this is a TRC config item */
                bte_trace_conf(p_name, p_value);
            }
        }

        fclose(p_file);
    }
    else
    {
        ALOGI( "bte_load_conf file >%s< not found", p_path);
    }
}

/*******************************************************************************
**
** Function        bte_parse_did_conf
**
** Description     Read conf entry from p_path file one by one and get
**                 the corresponding config value
**
** Returns         TRUE if success, else FALSE
**
*******************************************************************************/
static BOOLEAN bte_parse_did_conf (const char *p_path, UINT32 num,
    tKEY_VALUE_PAIRS *conf_pairs, UINT32 conf_pairs_num)
{
    UINT32 i, param_num=0, count=0, start_count=0, end_count=0, conf_num=0;
    BOOLEAN key=TRUE, conf_found=FALSE;

    FILE    *p_file;
    char    *p;
    char    line[CONF_MAX_LINE_LEN+1]; /* add 1 for \0 char */

    ALOGI("Attempt to load did conf from %s", p_path);

    if ((p_file = fopen(p_path, "r")) != NULL)
    {
        /* read line by line */
        while (fgets(line, CONF_MAX_LINE_LEN+1, p_file) != NULL)
        {
            count++;
            if (line[0] == CONF_COMMENT)
                continue;

            if (conf_found && (conf_num == num) && (*line == '[')) {
                conf_found = FALSE;
                end_count = count-1;
                break;
            }

            p = strtok(line, CONF_DELIMITERS);
            while (p != NULL) {
                if (conf_num <= num) {
                    if (key) {
                        if (!strcmp(p, conf_pairs[0].key)) {
                            if (++conf_num == num) {
                                conf_found = TRUE;
                                start_count = count;
                                strncpy(conf_pairs[0].value, "1", CONF_VALUE_LEN);
                            }
                        } else {
                            if (conf_num == num) {
                                for (i=1; i<conf_pairs_num; i++) {
                                    if (!strcmp(p, conf_pairs[i].key)) {
                                        param_num = i;
                                        break;
                                    }
                                }
                                if (i == conf_pairs_num) {
                                    error("Attribute %s does not belong to %s configuration",
                                        p, conf_pairs[0].key);
                                    fclose(p_file);
                                    return FALSE;
                                }
                            }
                            key = FALSE;
                        }
                    } else {
                        if ((conf_num == num) && param_num) {
                            strncpy(conf_pairs[param_num].value, p, CONF_VALUE_LEN-1);
                            param_num = 0;
                        }
                        key = TRUE;
                    }
                }
                p = strtok(NULL, CONF_DELIMITERS);
            }
        }

        fclose(p_file);
   }
   else
   {
        ALOGI( "bte_parse_did_conf file >%s< not found", p_path);
   }
   if (!end_count)
       end_count = count;

   if (start_count) {
        debug("Read %s configuration #%u from lines %u to %u in file %s",
            conf_pairs[0].key, (unsigned int)num, (unsigned int)start_count,
            (unsigned int)end_count, p_path);
        return TRUE;
   }

   error("%s configuration not found in file %s", conf_pairs[0].key, p_path);
        return FALSE;
}
