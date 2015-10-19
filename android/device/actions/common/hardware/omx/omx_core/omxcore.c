/**
 src/omxcore.c

 OpenMAX Integration Layer Core. This library implements the OpenMAX core
 responsible for environment setup, components tunneling and communication.

 Copyright (C) 2007-2009 STMicroelectronics
 Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2.1 of the License, or (at your option)
 any later version.

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License
 along with this library; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301  USA

 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>
#include <dlfcn.h>
#include <OMX_Core.h>
#include <OMX_ContentPipe.h>

#define LOG_TAG "omx_core"

#include "omxcore.h"
#include "expat.h"

//extern CPresult file_pipe_Constructor(CP_PIPETYPE* pPipe, CPstring szURI);
//extern CPresult inet_pipe_Constructor(CP_PIPETYPE* pPipe, CPstring szURI);
#define MAXNAMESIZE (130)
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/** The static field initialized is equal to 0 if the core is not initialized.
 * It is equal to 1 when the OMX_Init has been called
 */
static int initialized;
static int xmlflg;
/** The int bosa_loaders contains the number of loaders available in the system.
 */
static unsigned int act_loaders;
static unsigned int roleNum[50];
static unsigned int roleCount = 0;
/** The pointer to the loaders list. This list contains the all the different component loaders
 * present in the system or added by the IL Client with the BOSA_AddComponentLoader function.
 * The component loader is a implementation specific way to handle a set of components. The implementation
 * of the IL core accesses to the loaders in a standard way, but the different loaders can handle
 * different types of components, or handle in different ways the same components. It can be used also
 * to create a multi-OS support
 */
ActComponentType *loadersList = NULL;

#define MAXCOMP (50)
static unsigned int tableCount = 14;

static char *tComponentName[MAXCOMP][2] = {
    /*video Decoder components */
    {"OMX.Action.Video.Decoder", "video_decoder.avc"},
    {"OMX.Action.Video.Decoder", "video_decoder.vc1"},
    {"OMX.Action.Video.Decoder", "video_decoder.mpeg2"},
    {"OMX.Action.Video.Decoder", "video_decoder.mpeg4"},
    {"OMX.Action.Video.Decoder", "video_decoder.div3"},
    {"OMX.Action.Video.Decoder", "video_decoder.rv"},
    {"OMX.Action.Video.Decoder", "video_decoder.avs"},
    {"OMX.Action.Video.Decoder", "video_decoder.h263"},
    {"OMX.Action.Video.Decoder", "video_decoder.mjpg"},
    {"OMX.Action.Video.Encoder", "video_encoder.avc"},  
    {"OMX.Action.Video.Encoder", "video_encoder.mjpeg"},  

    {"OMX.Action.Video.Decoder", "video_decoder.jpeg"},
    {"OMX.Action.Video.Camera",  "video_camera.capture"},
		{"OMX.Action.Audio.Decoder", "audio_decoder.all"},

    /* terminate the table */
    {NULL, NULL},
};

typedef struct
{
    OMX_COMPONENTTYPE phandle;
    void *pModule;
} ACT_OMX_COMPONENTTYPE;

typedef enum xml_status
{
    nothing, cpEntry, cpName, cpLib, cpRole, cpRoleName, cpVersion, cpOmxVersion
} xml_status;

typedef struct parser_info
{
    int depth;
    int compIndex;
    int roleIndex;
    ActComponentType *loadlist;
    xml_status status;
} parser_info;

/*
void printloadlist(ActComponentType *loadlist)
{
	unsigned int i = 0;
    DEBUG(DEB_LEV_PARAMS, "name:%s\n", loadlist->name);
    DEBUG(DEB_LEV_PARAMS, "libname:%s\n", loadlist->libname);
    DEBUG(DEB_LEV_PARAMS, "name_specific_length:%d\n", loadlist->name_specific_length);
	for(i = 0; i < loadlist->name_specific_length; i++)
	{
		DEBUG(DEB_LEV_PARAMS, "name_specific:%s\n", loadlist->name_specific[i]);
		DEBUG(DEB_LEV_PARAMS, "role_specific:%s\n", loadlist->role_specific[i]);
	}
}
*/

static void XMLCALL
startElement(void *userData, const char *name, const char **atts)
{
    int *compNum = (int *) userData;
    if (!strcmp(name, "roles"))
    {
        (*compNum)++;
        roleCount = 0;
    }
    if (!strcmp(name, "role"))
    {
        roleCount++;
    }
    return;
}

static void XMLCALL
endElement(void *userData, const char *name)
{
    int *compNum = (int *) userData;
    if (!strcmp(name, "roles"))
    {
        roleNum[(*compNum)] = roleCount;
    }
    return;
}

static void XMLCALL
startElement2(void *userData, const char *name, const char **atts)
{
    parser_info *dat = (parser_info *) userData;
    dat->depth++;
    dat->status = nothing;
    if (!strcmp(name, "component"))
    {
        dat->compIndex++;
        dat->loadlist[dat->compIndex].name_specific_length = roleNum[dat->compIndex + 1];
        dat->loadlist[dat->compIndex].name_specific = (char**) malloc(sizeof(char*) * roleNum[dat->compIndex + 1]);
        dat->loadlist[dat->compIndex].role_specific = (char**) malloc(sizeof(char*) * roleNum[dat->compIndex + 1]);
        memset(dat->loadlist[dat->compIndex].name_specific, 0, sizeof(char*) * roleNum[dat->compIndex + 1]);
        memset(dat->loadlist[dat->compIndex].role_specific, 0, sizeof(char*) * roleNum[dat->compIndex + 1]);
        dat->loadlist[dat->compIndex].name = (char *) malloc(sizeof(char) * 64);
		dat->loadlist[dat->compIndex].name[0] = '\0';
        dat->loadlist[dat->compIndex].libname = (char *) malloc(sizeof(char) * 64);
		dat->loadlist[dat->compIndex].libname[0] = '\0';
        dat->loadlist[dat->compIndex].entry_func_name = (char *) malloc(sizeof(char) * 64);
		dat->loadlist[dat->compIndex].entry_func_name[0] = '\0';
    }
    if (!strcmp(name, "name") && dat->depth == 4)
    {
        dat->status = cpName;
    }
    if (!strcmp(name, "lib"))
    {
        dat->status = cpLib;
    }
    if (!strcmp(name, "entryname"))
    {
        dat->status = cpEntry;
    }
    if (!strcmp(name, "roles"))
    {
        dat->roleIndex = 0;
    }
    if (!strcmp(name, "name") && dat->depth == 5)
    {
        dat->status = cpRoleName;
        dat->loadlist[dat->compIndex].name_specific[dat->roleIndex] = (char *) malloc(sizeof(char) * 64);
		dat->loadlist[dat->compIndex].name_specific[dat->roleIndex][0] = '\0';
    }
    if (!strcmp(name, "role"))
    {
        dat->status = cpRole;
        dat->loadlist[dat->compIndex].role_specific[dat->roleIndex] = (char *) malloc(sizeof(char) * 64);
		dat->loadlist[dat->compIndex].role_specific[dat->roleIndex][0] = '\0';
    }
    if (!strcmp(name, "version"))
    {
        dat->status = cpVersion;
    }
    if (!strcmp(name, "omxversion"))
    {
        dat->status = cpOmxVersion;
    }
    return;
}

static void XMLCALL
endElement2(void *userData, const char *name)
{
    parser_info *dat = (parser_info *) userData;
    dat->depth--;
    if (!strcmp(name, "role"))
    {
        dat->roleIndex++;
    }
	dat->status = nothing;
}

static void XMLCALL
charhandle(void *userData, const char *s, int len)
{
    int a, b;
    parser_info *dat = (parser_info *) userData;
    ActComponentType *curCom = &(dat->loadlist[dat->compIndex]);

    switch (dat->status)
    {
        case cpName:
        strncat(curCom->name, s, len);
        break;
        case cpLib:
        strncat(curCom->libname, s, len);
        break;
        case cpEntry:
        strncat(curCom->entry_func_name, s, len);
        break;
        case cpRoleName:
        strncat(curCom->name_specific[dat->roleIndex], s, len);
        break;
        case cpRole:
        strncat(curCom->role_specific[dat->roleIndex], s, len);
        break;
        case cpVersion:
        sscanf(s, "%d.%d", &a, &b);
        curCom->componentVersion.s.nVersionMajor = (OMX_U8) a;
        curCom->componentVersion.s.nVersionMinor = (OMX_U8) b;
        break;
        case cpOmxVersion:
        sscanf(s, "%d.%d", &a, &b);
        curCom->omxVersion.s.nVersionMajor = (OMX_U8) a;
        curCom->omxVersion.s.nVersionMinor = (OMX_U8) b;
        break;
        default:
        break;	//do nothing;
    }
}

static int createComponentLoaders()
{
    FILE *fp = NULL;
    parser_info *parserInfo;
    char buf[BUFSIZ];
    XML_Parser parser = XML_ParserCreate(NULL );
    int done;
    int nCompNums = 0;
    fp = fopen("/system/etc/omx_codec.xml", "r");
    if (fp == NULL )
    {
        DEBUG(DEB_LEV_ERR, "open file error\n");
		XML_ParserFree(parser);
        return OMX_ErrorInsufficientResources;
    }

    XML_SetUserData(parser, &nCompNums);
    XML_SetElementHandler(parser, startElement, endElement);
    do
    {
        int len = (int) fread(buf, 1, sizeof(buf), fp);
        done = (unsigned) len < sizeof(buf);
        if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR)
        {
            DEBUG(DEB_LEV_ERR, "%s at line %u\n", XML_ErrorString(XML_GetErrorCode(parser)),
                    (unsigned int) XML_GetCurrentLineNumber(parser));
			fclose(fp);
			XML_ParserFree(parser);
            return OMX_ErrorInsufficientResources;
        }
    } while (!done);

    XML_ParserFree(parser);

    fseek(fp, 0, SEEK_SET);

    if (nCompNums == 0)
    {
        DEBUG(DEB_LEV_ERR, "error of %s,OMX_Init is %d\n", __func__, __LINE__);
        act_loaders = 0;
        fclose(fp);
        return OMX_ErrorInsufficientResources;
    }

    ActComponentType *act_component;

    act_loaders = nCompNums;
    act_component = (ActComponentType *) malloc(sizeof(ActComponentType) * nCompNums);
    parserInfo = (parser_info *) malloc(sizeof(parser_info));
    if(NULL == act_component)
    {
        DEBUG(DEB_LEV_ERR, "error of %s,OMX_Init is %d\n", __func__, __LINE__);
        fclose(fp);
        return OMX_ErrorInsufficientResources;
    }
    memset(parserInfo, 0, sizeof(parser_info));
    parserInfo->loadlist = act_component;
    parserInfo->roleIndex = -1;

    parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, parserInfo);
    XML_SetCharacterDataHandler(parser, charhandle);
    XML_SetElementHandler(parser, startElement2, endElement2);
    parserInfo->compIndex = -1;
    do
    {
        int len = (int) fread(buf, 1, sizeof(buf), fp);
        done = (unsigned) len < sizeof(buf);
        if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR)
        {
            DEBUG(DEB_LEV_ERR, "%s at line %u\n", XML_ErrorString(XML_GetErrorCode(parser)),
                    (unsigned int) XML_GetCurrentLineNumber(parser));
            free(parserInfo);
			fclose(fp);
			XML_ParserFree(parser);
            return OMX_ErrorInsufficientResources;
        }
    } while (!done);

    loadersList = parserInfo->loadlist;
	int i =0;

    XML_ParserFree(parser);

    fclose(fp);

    free(parserInfo);

    return 0;
}

static int destroyComponentLoaders()
{
    unsigned int i, j;
    for (i = 0; i < act_loaders; i++)
    {
        if (loadersList)
        {
            if (loadersList[i].libname)
            {
                free(loadersList[i].libname);
                loadersList[i].libname = NULL;
            }
			
            if (loadersList[i].name)
            {
                free(loadersList[i].name);
                loadersList[i].name = NULL;
            }

			if (loadersList[i].entry_func_name)
            {
                free(loadersList[i].entry_func_name);
                loadersList[i].entry_func_name = NULL;
            }

            for (j = 0; j < loadersList[i].name_specific_length; j++)
            {
                if (loadersList[i].name_specific[j])
                {
                    free(loadersList[i].name_specific[j]);
                    loadersList[i].name_specific[j] = NULL;
                }

                if (loadersList[i].role_specific[j])
                {
                    free(loadersList[i].role_specific[j]);
                    loadersList[i].role_specific[j] = NULL;
                }
            }
			if (loadersList[i].name_specific)
            {
                free(loadersList[i].name_specific);
                loadersList[i].name_specific = NULL;
            }

			if (loadersList[i].role_specific)
            {
                free(loadersList[i].role_specific);
                loadersList[i].role_specific = NULL;
            }
        }
    }

    if (loadersList)
    {
        free(loadersList);
        loadersList = NULL;
    }

    return 0;
}

/** @brief The OMX_Init standard function
 *
 * This function calls the init function of each component loader added. If there
 * is no component loaders present, the ST default component loader (static libraries)
 * is loaded as default component loader.
 *
 * @return OMX_ErrorNone
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_Init()
{
    if (pthread_mutex_lock(&mutex) != 0)
    {
        return OMX_ErrorUndefined;
    }
    if (initialized == 0)
    {
        if (createComponentLoaders())
        {
            DEBUG(DEB_LEV_ERR, "error of %s,OMX_Init is %d\n", __func__, __LINE__);
            xmlflg = 0;
        }
		else
		{
			xmlflg = 1;
		}
    }
	initialized++;
	
    if (pthread_mutex_unlock(&mutex) != 0)
    {
        return OMX_ErrorUndefined;
    }
    ALOGV("Out of %s, initialized = %d", __func__, initialized);
    return OMX_ErrorNone;
}

/** @brief The OMX_Deinit standard function
 *
 * In this function the Deinit function for each component loader is performed
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_Deinit()
{
    if (pthread_mutex_lock(&mutex) != 0)
    {
        return OMX_ErrorUndefined;
    }

    if (initialized <= 0)
    {
        DEBUG(DEB_LEV_ERR, "error of %s,initialized is %d\n", __func__, initialized);
        initialized = 0;
    }

    if (initialized > 0)
    {
        if(initialized == 1)
        {
    		if(xmlflg == 1)
    		{
    			destroyComponentLoaders();
    		}
            loadersList = 0;
        }
        initialized--;
    }
	
    if (pthread_mutex_unlock(&mutex) != 0)
    {
        return OMX_ErrorUndefined;
    }
    ALOGV("Out of %s initialized =%d", __func__, initialized);
    return OMX_ErrorNone;
}

/** @brief the OMX_GetHandle standard function
 *
 * This function will scan inside any component loader to search for
 * the requested component. If there are more components with the same name
 * the first component is returned. The existence of multiple components with
 * the same name is not contemplated in OpenMAX specification. The assumption is
 * that this behavior is NOT allowed.
 *
 * @return OMX_ErrorNone if a component has been found
 *         OMX_ErrorComponentNotFound if the requested component has not been found
 *                                    in any loader
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE* pHandle, OMX_STRING cComponentName, OMX_PTR pAppData,
        OMX_CALLBACKTYPE* pCallBacks)
{
    void *pModules = NULL;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    unsigned int i = 0;
    OMX_BOOL flghandle;	//

    static const char prefix[] = "lib";
    static const char postfix[] = ".so";
    char buf[sizeof(prefix) + MAXNAMESIZE + sizeof(postfix)];
    int (*pComponent_entry)(OMX_COMPONENTTYPE *, OMX_STRING);
    OMX_COMPONENTTYPE *componentType;

    if (pthread_mutex_lock(&mutex) != 0)
    {
        DEBUG(DEB_LEV_ERR, "In %s for %d\n", __func__, __LINE__);
        return OMX_ErrorUndefined;
    }

    if ((NULL == cComponentName) || (NULL == pHandle) || (NULL == pCallBacks))
    {
        DEBUG(DEB_LEV_ERR, "In %s for %d\n", __func__, __LINE__);
        err = OMX_ErrorBadParameter;
        goto UNLOCK_MUTEX;
    }
	
	if (initialized <= 0)
	{
	    initialized = 0;
		DEBUG(DEB_LEV_ERR, "call omx_init() first!");
		err = OMX_ErrorUndefined;
		goto UNLOCK_MUTEX;
	}

	flghandle = OMX_FALSE;
    do
    {
		if (xmlflg == 0)
		{
			strcpy(buf, prefix);
			strcat(buf, cComponentName);
			strcat(buf, postfix);
		}
		else
		{
			for (i = 0; i < act_loaders; i++)
			{
				if (!strcmp(loadersList[i].name, cComponentName))
				{
					break;
				}
			}
			
			if (i >= act_loaders)
			{
				DEBUG(DEB_LEV_ERR, "cannot find component in xml file");
				err = OMX_ErrorNoMore;
				break;
			}
			else
			{
				strcpy(buf, loadersList[i].libname);
			}
		}

        pModules = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
        if (pModules == NULL )
        {
            DEBUG(DEB_LEV_ERR, "In %s for %d, dlerror = %s\n", __func__, __LINE__, dlerror());
            err = OMX_ErrorComponentNotFound;
            break;
        }

        /* Get a function pointer to the "OMX_ComponentInit" function.  If
         * there is an error, we can't go on, so set the error code and exit */
        if (xmlflg == 1)
        {
            pComponent_entry = dlsym(pModules, loadersList[i].entry_func_name);
        }
        else
        {
            pComponent_entry = dlsym(pModules, "OMX_ComponentInit");
        }

        if (pComponent_entry == NULL)
        {
            DEBUG(DEB_LEV_ERR, "In %s for %d dlerror = %s\n", __func__, __LINE__, dlerror());
            err = OMX_ErrorInvalidComponent;
            break;
        }

        *pHandle = malloc(sizeof(ACT_OMX_COMPONENTTYPE));
        if (*pHandle == NULL )
        {
            DEBUG(DEB_LEV_ERR, "In %s for %d\n", __func__, __LINE__);
            err = OMX_ErrorInsufficientResources;
            break;
        }
        memset(*pHandle, 0, sizeof(ACT_OMX_COMPONENTTYPE));
        componentType = &((ACT_OMX_COMPONENTTYPE*) (*pHandle))->phandle;
        componentType->nSize = sizeof(OMX_COMPONENTTYPE);
        ((ACT_OMX_COMPONENTTYPE*) (*pHandle))->pModule = pModules;
        (*pComponent_entry)(componentType, cComponentName);	//È¡µÃhandle
        if (OMX_ErrorNone == err)
        {
            err = (componentType->SetCallbacks)(*pHandle, pCallBacks, pAppData);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s for %d\n", __func__, __LINE__);
                break;
            }
            flghandle = OMX_TRUE;
            // Component is found, and thus we are done
        }
        else if (err == OMX_ErrorInsufficientResources)
        {
            DEBUG(DEB_LEV_ERR, "In %s for %d\n", __func__, __LINE__);
            break;
        }
    } while (0);

    if (OMX_TRUE != flghandle)
    {
        if (*pHandle != NULL )
        {
            free(*pHandle);
            *pHandle = NULL;
        }
        if (pModules != NULL )
        {
            dlclose(pModules);
            pModules = NULL;
        }
    }

    UNLOCK_MUTEX:
    if (pthread_mutex_unlock(&mutex) != 0)
    {
        err = OMX_ErrorUndefined;
    }
    ALOGV("Out of %s", __func__);
    return (err);
}

/** @brief The OMX_FreeHandle standard function
 *
 * This function executes the BOSA_DestroyComponent of the component loaders
 *
 * @param hComponent the component handle to be freed
 *
 * @return The error of the BOSA_DestroyComponent function or OMX_ErrorNone
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE err;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) hComponent;

    if (pthread_mutex_lock(&mutex) != 0)
    {
        return OMX_ErrorUndefined;
    }

    err = pHandle->ComponentDeInit(pHandle);
    dlclose(((ACT_OMX_COMPONENTTYPE*) pHandle)->pModule);
    free(pHandle);
    if (err == OMX_ErrorNone)
    {
        // the component has been found and destroyed
        if (pthread_mutex_unlock(&mutex) != 0)
        {
            return OMX_ErrorUndefined;
        }
        return OMX_ErrorNone;
    }
    if (pthread_mutex_unlock(&mutex) != 0)
    {
        return OMX_ErrorUndefined;
    }
    ALOGV("Out of %s\n", __func__);
    return OMX_ErrorComponentNotFound;
}

/** @brief the OMX_ComponentNameEnum standard function
 *
 * This function build a complete list of names from all the loaders.
 * For each loader the index is from 0 to max, but this function must provide a single
 * list, with a common index. This implementation orders the loaders and the
 * related list of components.
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
	
	if(initialized > 0)
	{
		if(xmlflg != 0)
		{
			if (nIndex >= act_loaders)
			{
				err = OMX_ErrorNoMore;
			}
			else
			{
				strcpy(cComponentName, loadersList[nIndex].name);
			}
		}
		else
		{
			if (nIndex >=  tableCount)
			{
				err = OMX_ErrorNoMore;
			}
			else
			{
				strcpy(cComponentName, tComponentName[nIndex][0]);
			}
		}
	}
	else
	{
		err = OMX_ErrorBadParameter;
	}

    ALOGV("Out of %s\n", __func__);
    return err;
}

/** @brief the OMX_SetupTunnel standard function
 *
 * The implementation of this function is described in the OpenMAX spec
 *
 * @param hOutput component handler that controls the output port of the tunnel
 * @param nPortOutput index of the output port of the tunnel
 * @param hInput component handler that controls the input port of the tunnel
 * @param nPortInput index of the input port of the tunnel
 *
 * @return OMX_ErrorBadParameter, OMX_ErrorPortsNotCompatible, tunnel rejected by a component
 * or OMX_ErrorNone if the tunnel has been established
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_SetupTunnel(OMX_HANDLETYPE hOutput, OMX_U32 nPortOutput, OMX_HANDLETYPE hInput,
        OMX_U32 nPortInput)
{

    OMX_ERRORTYPE err;
    OMX_COMPONENTTYPE* component;
    OMX_TUNNELSETUPTYPE* tunnelSetup;
		if(initialized <= 0){
		    initialized = 0;
			return OMX_ErrorUndefined;
		}
    DEBUG(DEB_LEV_PARAMS, "In %s the output port is:%p/%i, the input port is %p/%i\n", __func__, hOutput,
            (int) nPortOutput, hInput, (int) nPortInput);
    tunnelSetup = malloc(sizeof(OMX_TUNNELSETUPTYPE));
    component = (OMX_COMPONENTTYPE*) hOutput;
    tunnelSetup->nTunnelFlags = 0;
    tunnelSetup->eSupplier = OMX_BufferSupplyUnspecified;

    if (hOutput == NULL && hInput == NULL )
        return OMX_ErrorBadParameter;
    if (hOutput)
    {
        err = (component->ComponentTunnelRequest)(hOutput, nPortOutput, hInput, nPortInput, tunnelSetup);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Tunneling failed: output port rejects it - err = %x\n", err);
            free(tunnelSetup);
            tunnelSetup = NULL;
            return err;
        }
    }

    component = (OMX_COMPONENTTYPE*) hInput;
    if (hInput)
    {
        err = (component->ComponentTunnelRequest)(hInput, nPortInput, hOutput, nPortOutput, tunnelSetup);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Tunneling failed: input port rejects it - err = %08x\n", err);
            // the second stage fails. the tunnel on poutput port has to be removed
            component = (OMX_COMPONENTTYPE*) hOutput;
            err = (component->ComponentTunnelRequest)(hOutput, nPortOutput, NULL, 0, tunnelSetup);
            if (err != OMX_ErrorNone)
            {
                // This error should never happen. It is critical, and not recoverable
                free(tunnelSetup);
                tunnelSetup = NULL;
                DEBUG(DEB_LEV_PARAMS, "Out of %s with OMX_ErrorUndefined\n", __func__);
                return OMX_ErrorUndefined;
            }
            free(tunnelSetup);
            tunnelSetup = NULL;
            DEBUG(DEB_LEV_PARAMS, "Out of %s with OMX_ErrorPortsNotCompatible\n", __func__);
            return OMX_ErrorPortsNotCompatible;
        }
    }

    free(tunnelSetup);
    tunnelSetup = NULL;
    DEBUG(DEB_LEV_PARAMS, "Out of %s\n", __func__);
    return OMX_ErrorNone;
}

/** @brief the OMX_GetRolesOfComponent standard function
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_STRING CompName, OMX_U32 *pNumRoles, OMX_U8 **roles)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    unsigned int i, j;
    //act_loaders
    OMX_BOOL bFound = OMX_FALSE;

    if (CompName == NULL || pNumRoles == NULL || initialized == 0)
    {
        DEBUG(DEB_LEV_ERR, "In %s for %d\n", __func__, __LINE__);
        err = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if(xmlflg == 0)
    {
	    i = 0;
	    while (i < tableCount)
	    {
	        if (strcmp(CompName, tComponentName[i][0]) == 0)
	        {
	            bFound = OMX_TRUE;
	            break;
	        }
	        i++;
	    }
	    
	    if (!bFound)
	    {
	        err = OMX_ErrorComponentNotFound;
			DEBUG(DEB_LEV_ERR, "component %s not found\n", CompName);
	        goto EXIT;
	    } 
	    
	    if (roles == NULL)
	    { 
	        *pNumRoles = 1;
	    }
	    else
	    {
        /* must be second of two calls,
           pNumRoles is input in this context.
           If pNumRoles is < actual number of roles
           than we return an error */
            if (*pNumRoles >= 1)
            {

               strcpy((OMX_STRING)roles[0], tComponentName[i][1]);
               *pNumRoles = 1;
            }
            else
            {
                err = OMX_ErrorBadParameter;
                DEBUG(DEB_LEV_ERR, "pNumRoles (%ld) is less than actual number (%s) of roles \
                       for this component %s\n", *pNumRoles, tComponentName[i][1], CompName);
            }
    	}
  	}
  	else
  	{
	    i = 0;
	    while (i < act_loaders)
	    {
	        if (strcmp(CompName, loadersList[i].name) == 0)
	        {
	            bFound = OMX_TRUE;
	            break;
	        }
	        i++;
	    }
	
	    if (!bFound)
	    {
	        err = OMX_ErrorComponentNotFound;
	        goto EXIT;
	    }
	
	    if (roles == NULL )
	    {
	        *pNumRoles = loadersList[i].name_specific_length;
	    }
	    else
	    {
	        if (*pNumRoles >= loadersList[i].name_specific_length)
	        {
	            for (j = 0; j < loadersList[i].name_specific_length; j++)
	            {
	                strcpy((OMX_STRING) roles[j], loadersList[i].role_specific[j]);
	            }
	            *pNumRoles = loadersList[i].name_specific_length;
	        }
	        else
	        {
	            err = OMX_ErrorBadParameter;
	        }
	    }
	}
    EXIT: ALOGV("Out of %s\n", __func__);
    return err;
}

/** @brief the OMX_GetComponentsOfRole standard function
 *
 * This function searches in all the component loaders any component
 * supporting the requested role
 *
 * @param role See spec
 * @param pNumComps See spec
 * @param compNames See spec
 *
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_STRING role, OMX_U32 *pNumComps, OMX_U8 **compNames)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 i = 0;
    OMX_U32 j = 0;
    OMX_U32 k = 0;
    OMX_U32 compOfRoleCount = 0;

    if (role == NULL || pNumComps == NULL || initialized == 0)
    {
        DEBUG(DEB_LEV_ERR, "In %s for %d\n", __func__, __LINE__);
        err = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if(xmlflg == 0)
    {
		/* This implies that the componentTable is not filled */
	    if (!tableCount)
	    {
	        err = OMX_ErrorUndefined;
	        DEBUG(DEB_LEV_ERR, "Component table is empty. Please reload OMX Core\n");
	        goto EXIT;
	    }

	    /* no matter, we always want to know number of matching components
	       so this will always run */ 
	    for (i = 0; i < tableCount; i++)
	    {
	        for (j = 0; j < 1; j++) 
	        { 
	            if (strcmp(tComponentName[i][1], role) == 0)
	            {
	                /* the first call to this function should only count the number
	                   of roles 
	                */
	                compOfRoleCount++;
	            }
	        }
	    }
	    
	    if (compOfRoleCount == 0)
	    {
	        err = OMX_ErrorComponentNotFound;
			DEBUG(DEB_LEV_ERR, "Component supporting role %s was not found\n", role);
	
	    }
	    
	    if (compNames == NULL)
	    {
	        /* must be the first of two calls */
	        *pNumComps = compOfRoleCount;
	    }
	    else
	    {
	        /* must be the second of two calls */
	        if (*pNumComps < compOfRoleCount)
	        {
	            /* pNumComps is input in this context,
	               it can not be less, this would indicate
	               the array is not large enough
	            */
	            err = OMX_ErrorBadParameter;
	            DEBUG(DEB_LEV_ERR, "pNumComps (%ld) is less than the actual number (%ld) of components supporting role %s\n", *pNumComps, compOfRoleCount, role);
	        }
	        else
	        {
	            k = 0;
	            for (i = 0; i < tableCount; i++)
	            {
	                for (j = 0; j < 1; j++) 
	                { 
	                    if (strcmp(tComponentName[i][1], role) == 0)
	                    {
	                        /*  the second call compNames can be allocated
	                            with the proper size for that number of roles.
	                        */
	                        compNames[k] = (OMX_U8*)tComponentName[i][0];
	                        k++;
	                        if (k == compOfRoleCount)
	                        {
	                            /* there are no more components of this role
	                               so we can exit here */
	                            *pNumComps = k;
	                            goto EXIT;
	                        } 
	                    }
	                }
	            }
	        }        
    	}
	}
	else
	{
        /* This implies that the componentTable is not filled */
        if (!act_loaders)
        {
            DEBUG(DEB_LEV_ERR, "In %s for %d\n", __func__, __LINE__);
            err = OMX_ErrorUndefined;
            goto EXIT;
        }

        /* no matter, we always want to know number of matching components
         so this will always run */
        for (i = 0; i < act_loaders; i++)
        {
            for (j = 0; j < loadersList[i].name_specific_length; j++)
            {
                if (strcmp(loadersList[i].role_specific[j], role) == 0)
                {
                    /* the first call to this function should only count the number
                     of roles
                     */
                    compOfRoleCount++;
                }
            }
        }
        if (compOfRoleCount == 0)
        {
            DEBUG(DEB_LEV_ERR, "In %s for %d\n", __func__, __LINE__);
            err = OMX_ErrorComponentNotFound;
        }

        if (compNames == NULL )
        {
            /* must be the first of two calls */
            *pNumComps = compOfRoleCount;
        }
        else
        {
            /* must be the second of two calls */
            if (*pNumComps < compOfRoleCount)
            {
                /* pNumComps is input in this context,
                 it can not be less, this would indicate
                 the array is not large enough
                 */
                err = OMX_ErrorBadParameter;
            }
            else
            {
                k = 0;
                for (i = 0; i < act_loaders; i++)
                {
                    for (j = 0; j < loadersList[i].name_specific_length; j++)
                    {
                        if (strcmp(loadersList[i].role_specific[j], role) == 0)
                        {
                            /*  the second call compNames can be allocated
                             with the proper size for that number of roles.
                             */
                            compNames[k] = (OMX_U8*) loadersList[i].name_specific[j];
                            k++;
                            if (k == compOfRoleCount)
                            {
                                /* there are no more components of this role
                                 so we can exit here */
                                *pNumComps = k;
                                goto EXIT;
                            }
                        }
                    }
                }
            }
        }
  	}

    EXIT: ALOGV("Out of %s\n", __func__);
    return err;
}

//OSCL_EXPORT_REF OMX_ERRORTYPE OMX_GetContentPipe(
//    OMX_HANDLETYPE *hPipe,
//    OMX_STRING szURI) {
//	  OMX_ERRORTYPE err = OMX_ErrorContentPipeCreationFailed;
//	  CPresult res;
//	  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
//
//	  if(strncmp(szURI, "file", 4) == 0) {
//	    res = file_pipe_Constructor((CP_PIPETYPE*) hPipe, szURI);
//	    if(res == 0x00000000)
//	      err = OMX_ErrorNone;
//	  }
//
//	  else if(strncmp(szURI, "inet", 4) == 0) {
//	    res = inet_pipe_Constructor((CP_PIPETYPE*) hPipe, szURI);
//	    if(res == 0x00000000)
//	      err = OMX_ErrorNone;
//	  }
//	  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
//	  return err;
//}
