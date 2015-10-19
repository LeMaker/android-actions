/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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



#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#ifdef CAMERA_ANDROID_LOG

#undef LOG_NDEBUG
#define LOG_NDEBUG 0

#undef LOG_TAG
#define LOG_TAG "CameraHal"


#include <utils/Log.h>

#if 0
#define Filename do{char *path=__FILE__;\
	char *ptr=path;\
	int size=strlen(path);\
	int i=size;\
	while(strcmp(*(ptr+i),'/'))\
	{\
		i--;\
		ptr=path+i;\
	}\
	ptr;
}while(0)
#endif 

///Defines for debug statements - Macro LOG_TAG needs to be defined in the respective files
#define DBGUTILS_LOGIA(str)         ALOGI("%s:%d %s - " str,__FILE__, __LINE__,__FUNCTION__);
#define DBGUTILS_LOGIB(str, ...)    ALOGI("%s:%d %s - " str,__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define DBGUTILS_LOGVA(str)         ALOGV("%s:%d %s - " str,__FILE__, __LINE__,__FUNCTION__);
#define DBGUTILS_LOGVB(str,...)     ALOGV("%s:%d %s - " str,__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define DBGUTILS_LOGDA(str)         ALOGD("%s:%d %s - " str,__FILE__, __LINE__,__FUNCTION__);
#define DBGUTILS_LOGDB(str, ...)    ALOGD("%s:%d %s - " str,__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define DBGUTILS_LOGEA(str)         ALOGE("%s:%d %s - " str,__FILE__, __LINE__, __FUNCTION__);
#define DBGUTILS_LOGEB(str, ...)    ALOGE("%s:%d %s - " str,__FILE__, __LINE__,__FUNCTION__, __VA_ARGS__);
#define DBGUTILS_LOG_FUNCTION_NAME           ALOGV("%d: %s() ENTER", __LINE__, __FUNCTION__);
#define DBGUTILS_LOG_FUNCTION_NAME_EXIT      ALOGV("%d: %s() EXIT", __LINE__, __FUNCTION__);
#define DBGUTILS_LOG_FUNCTION_NAME_EXIT_RET(ret)      ALOGV("%d: %s() EXIT,ret=%d", __LINE__, __FUNCTION__,ret);

#else

#include <stdio.h>

#undef LOG_TAG
#define LOG_TAG "CameraHal"

#undef LOG_NDEBUG

#define DBGUTILS_LOGI(str)         fprintf(stdout, "%s:%d %s - " str "\n",__FILE__, __LINE__,__FUNCTION__);
#define DBGUTILS_LOGVA(str)         fprintf(stdout, "%s:%d %s - " str "\n",__FILE__, __LINE__,__FUNCTION__);
#define DBGUTILS_LOGVB(str,...)     fprintf(stdout, "%s:%d %s - " str "\n",__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define DBGUTILS_LOGDA(str)         fprintf(stdout, "%s:%d %s - " str "\n",__FILE__, __LINE__,__FUNCTION__);
#define DBGUTILS_LOGDB(str, ...)    fprintf(stdout, "%s:%d %s - " str "\n",__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define DBGUTILS_LOGEA(str)         fprintf(stdout, "%s:%d %s - " str "\n",__FILE__, __LINE__, __FUNCTION__);
#define DBGUTILS_LOGEB(str, ...)    fprintf(stdout, "%s:%d %s - " str "\n",__FILE__, __LINE__,__FUNCTION__, __VA_ARGS__);
#define DBGUTILS_LOG_FUNCTION_NAME           fprintf(stdout, "%d: %s() ENTER\n", __LINE__, __FUNCTION__);
#define DBGUTILS_LOG_FUNCTION_NAME_EXIT      fprintf(stdout, "%d: %s() EXIT\n", __LINE__, __FUNCTION__);
#define DBGUTILS_LOG_FUNCTION_NAME_EXIT_RET(ret)      fprintf(stdout, "%d: %s() EXIT,ret=%d\n", __LINE__, __FUNCTION__,ret);
#endif


#endif //DEBUG_UTILS_H

