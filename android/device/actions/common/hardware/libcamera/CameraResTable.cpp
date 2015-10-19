
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>
#include <dlfcn.h>

#include "CameraHalDebug.h"

#include "CameraResTable.h"


#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

namespace android
{

const CameraResItem CameraResTable::sCameraResNameTable[] = {
    {"QCIF",    176,    144},
    {"QVGA",    320,    240},
    {"CIF",     352,    288},
    {"D1",      752,    480},
    {"VGA",     640,    480},
    {"SVGA",    800,    600},
    {"WVGA",    800,    480},
    {"HD",      1280,   720},
    {"960P",    1280,   960},    //ActionsCode(author:liuyiguang, add_code)
    {"UXGA",    1600,   1200},
    {"FULLHD",  1920,   1080},
    {"1M",      1024,   768},
    {"1.3M",    1280,   960},
    {"2M",      1600,   1200},
    {"2.1M",    1920,   1080},    //ActionsCode(author:liuyiguang, add_code)
    {"3M",      2048,   1536},
    {"5M",      2592,   1944},
    {"8M",      3264,   2448},
    {"12M",     4288,   2848},
};

const CameraResItem *CameraResTable::getResByName(const char *name)
{
    int i = 0;
    const CameraResItem *item;

    for(i=0; i< (int)ARRAY_SIZE(sCameraResNameTable);i++)
    {

        item = &sCameraResNameTable[i];

        if(strcmp(item->name, name)== 0)
        {
            return item;
        }

    }

    return NULL;
}

const CameraResItem *CameraResTable::getResBySize(int w, int h)
{
    int i = 0;
    const CameraResItem *item;

    for(i=0; i< (int)ARRAY_SIZE(sCameraResNameTable);i++)
    {

        item = &sCameraResNameTable[i];

        if(item->width == w && item->height == h)
        {
            return item;
        }
    }

    return NULL;
}
};
