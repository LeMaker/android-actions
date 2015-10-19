#include <stdio.h>
#include <string.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <errno.h> 
#include <unistd.h> 
#include <linux/limits.h>

#undef LOG_NDEBUG
#define LOG_NDEBUG 0

#undef LOG_TAG
#define LOG_TAG "CameraMediaProfileGenerator"

#include <utils/Log.h>

#include "MediaProfileGenerator.h"
#include "CameraHal.h"

namespace android
{
#define  TAG_START "\"\"\""
#define  TAG_END TAG_START
#define  TAG_HEADER "HEADER"
#define  TAG_PROFILE "CAMERAPROFILES_TEMPLATE"
#define  TAG_VIDEOSIZE "VIDEOSIZE_TEMPLATE"
#define  TAG_FOOTER "FOOTER"

#define MAX_VIDEOSIZE_INFO_SIZE (4*1024)
#define MAX_PROFILE_SIZE (16*1024)
#define MAX_QUALITY_NAME_SIZE (64)

#define WRITE_DATA(fd, data, size) do{  \
    int write_len = size;               \
    int bytes_write = 0;                \
    int ret = 0;                        \
    while(bytes_write < write_len)      \
    {                                   \
        ret = write(fd, (data) + bytes_write, write_len- bytes_write);    \
        if(ret > 0)                     \
        {                               \
            bytes_write+=ret;           \
        }                               \
    }                                   \
}while(0)

#define READ_DATA(fd, data, size) do{  \
    int read_len = size;               \
    int bytes_read= 0;                \
    int ret = 0;                        \
    while(bytes_read < read_len)      \
    {                                   \
        ret = read(fd, (data) + bytes_read, read_len- bytes_read);    \
        if(ret > 0)                     \
        {                               \
            bytes_read+=ret;           \
        }                               \
    }                                   \
}while(0)

#define DONOT_MODIFY_WARNING "<!--Do not modify this, this is auto genarated by camerahal at %s-->\r\n"

#define PROFILE_MAGIC "<!--MAGIC_OF_AUTO_GENARATED_MEDIAPROFILES;%s;%s:%s-->\n"

#define XML_HEADER "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"

typedef struct 
{
    const char *name;
    int width;
    int height;
}QualityMapper;

QualityMapper gQualityMapper[]={
    {"qvga", 320, 240},
    {"cif", 352, 288},
    {"480p", 640, 480},
    {"480p", 704, 480},
    {"480p", 720, 480},
    {"720p", 1280, 720},
    {"1080p", 1920, 1080},
};

static const char * getQualityName(int w, int h)
{
    unsigned int i = 0;
    for (i = 0; i < sizeof(gQualityMapper)/sizeof(gQualityMapper[0]);i++)
    {
        if(gQualityMapper[i].width == w
            && gQualityMapper[i].height == h)
        {
            return gQualityMapper[i].name;
        }
    }
    return NULL;
}

static int isInQualityMapper(int w, int h)
{
    unsigned int i = 0;
    for (i = 0; i < sizeof(gQualityMapper)/sizeof(gQualityMapper[0]);i++)
    {
        if(gQualityMapper[i].width == w
            && gQualityMapper[i].height == h)
        {
            return 1;
        }
    }
    return 0;
}
static int create_dir(const char *path) 
{ 
    char dir_name[PATH_MAX]; 
    strlcpy(dir_name, path, PATH_MAX); 
    int i = 0;
    int len = strlen(dir_name); 
    if(dir_name[len-1]!='/') 
    {
        strcat(dir_name, "/"); 
    }

    len = strlen(dir_name); 

    for(i=1; i<len; i++) 
    { 
        if(dir_name[i]=='/') 
        { 
            dir_name[i] = 0; 
            if( access(dir_name, 0)!=0 ) 
            { 
                if(mkdir(dir_name, 0755)==-1) 
                { 
                    ALOGE("mkdir(%s) error(%d, %s)", dir_name, errno, strerror(errno)); 
                    return -1; 
                } 
            } 
            dir_name[i] = '/'; 
        } 
    } 
    return 0; 
}

MediaProfileGenerator::MediaProfileGenerator()
{
    int i = 0;
    mCameraNum = 0;
    mTemplate = NULL;
    mHeader = NULL;
    mFooter = NULL;
    mProfileTemplate = NULL;
    mVideoSizeTemplate = NULL;
    mHeaderLen = 0;
    mFooterLen = 0;
    for( i = 0; i < _MAX_CAMERA_NUM; i++)
    {
        memset((char *)&mSensorNames[i], 0, _MAX_SENSOR_NAME_SIZE);
    }
};
MediaProfileGenerator::~MediaProfileGenerator() 
{
    unsigned int i = 0;
    if(mTemplate != NULL)
    {
        free(mTemplate);
    }
    mHeader = NULL;
    mFooter = NULL;
    mProfileTemplate = NULL;
    mVideoSizeTemplate = NULL;
    for(i = 0; i < sizeof(mFrameSizes)/sizeof(mFrameSizes[0]); i++)
    {
        mFrameSizes[i].clear();
    }
};

status_t MediaProfileGenerator::getTagValue(char *str, const char *tag, char **start, char **end )
{
    char *tagPtr = NULL;
    char *tagStartPtr = NULL;
    char *tagEndPtr = NULL;

    tagPtr = strstr(str, tag);
    if(tagPtr == NULL)
    {
        ALOGE("strstr (%s) error!", tag);
        return -1;
    }

    tagStartPtr = tagPtr + strlen(tag);
    tagStartPtr = strstr(tagStartPtr, TAG_START);
    if(tagStartPtr == NULL)
    {
        ALOGE("strstr (%s, %s) error!", tag, TAG_START);
        return -1;
    }
    tagStartPtr = tagStartPtr + strlen(TAG_START);

    tagEndPtr = strstr(tagStartPtr, TAG_END);
    if(tagEndPtr == NULL)
    {
        ALOGE("strstr (%s, %s) error!", tag, TAG_END);
        return -1;
    }
    *start = tagStartPtr;
    *end = tagEndPtr;

    return 0;
}

status_t MediaProfileGenerator::loadTemplate(const char * temppath)
{
    int ret = 0;
    int fd = -1;
    unsigned long filelen = 0;

    char *header_start;
    char *header_end;
    char *profile_start;
    char *profile_end;
    char *videosize_start;
    char *videosize_end;
    char *footer_start;
    char *footer_end;

    fd = open(temppath, O_RDONLY );
    if(fd < 0)
    {
        ALOGE("open template file (%s) error!\n", temppath);
        return -1;
    }
    filelen = lseek(fd, 0, SEEK_END);  
    if (filelen <= 0)  
    {  
        ALOGE("lseek failed (%s) !\n", strerror(errno));  
        close(fd);  
        return -1;  
    }  
    // seek to the start of the file
    lseek(fd, 0, SEEK_SET);  

    //(filelen+1) for '\0'
    mTemplate = (char *)malloc(filelen + 1);
    if(mTemplate == NULL)
    {
        ALOGE("malloc failed for (%ld) !\n", (filelen+1));  
        close(fd);
        return -1;
    }

    //read all contents
    READ_DATA(fd, mTemplate, filelen);
    close(fd);

    //get header
    ret = getTagValue(mTemplate, TAG_HEADER, &header_start, &header_end);
    ret |= getTagValue(mTemplate, TAG_PROFILE, &profile_start, &profile_end);
    ret |= getTagValue(mTemplate, TAG_VIDEOSIZE, &videosize_start, &videosize_end);
    ret |= getTagValue(mTemplate, TAG_FOOTER, &footer_start, &footer_end);

    if(ret != 0)
    {
        ALOGE("getTagValue error!");
        free(mTemplate);
        mTemplate = NULL;
        return -1;
    }
    mHeader = header_start;
    *header_end = '\0';
    mHeaderLen = header_end - header_start;

    mProfileTemplate = profile_start;
    *profile_end = '\0';

    mVideoSizeTemplate = videosize_start;
    *videosize_end = '\0';

    mFooter = footer_start;
    *footer_end = '\0';
    mFooterLen = footer_end - footer_start;
    return 0;
}

status_t MediaProfileGenerator::setCameraNum(int num){
    mCameraNum = num;
    return 0;
};

status_t MediaProfileGenerator::addVideoSize(int id, int width, int height)
{
    unsigned int i = 0;

    if(id >= mCameraNum)
    {
        return -1;
    }
    if(!isInQualityMapper(width, height))
    {
        return -1;
    }
    //has added?
    for(i =0; i < mFrameSizes[id].size(); i++)
    {
        CameraFrameSize size = mFrameSizes[id].itemAt(i);
        if(size.width == width && size.height == height)
        {
            return 0;
        }
    }

    CameraFrameSize size;
    size.width = width;
    size.height = height;

    mFrameSizes[id].add(size);

    return 0;
}
status_t MediaProfileGenerator::setSensorName(int id, char *name)
{
    strlcpy((char *)&mSensorNames[id], name, _MAX_SENSOR_NAME_SIZE);
    return 0;
}

int MediaProfileGenerator::getMaxSize(int id)
{
    unsigned int i = 0;
    int max = 0;
    int maxIndex = -1;

    for(i =0; i < mFrameSizes[id].size(); i++)
    {
        CameraFrameSize size = mFrameSizes[id].itemAt(i);
        if( size.width * size.height >= max)
        {
            max = size.width*size.height;
            maxIndex = i;
        }
    }
    return maxIndex;

}
int MediaProfileGenerator::getMinSize(int id)
{
    unsigned int i = 0;
    int min = 10000*10000;
    int minIndex = -1;

    for(i =0; i < mFrameSizes[id].size(); i++)
    {
        CameraFrameSize size = mFrameSizes[id].itemAt(i);
        if( size.width * size.height <= min)
        {
            min = size.width*size.height;
            minIndex = i;
        }
    }
    return minIndex;
}

/**
 *
 * MERGEFIX:  Sync the bitrate value with VCE.
 *
 ************************************
 *
 * ActionsCode(author:liuyiguang, change_code)
 */
void MediaProfileGenerator::genVideoSizeInfo(char *buf, int bufsize, const char *quality, int w, int h)
{
    int duration = 60;
    int bitrate = 4096000;
    if(strstr(quality, "timelapse") != NULL)
    {
        duration = 30;

        if( w == 1920 && h == 1080 )
        {
            //ActionsCode(author:liuyiguang, change_code)
            bitrate = 8192000;
        }
        else if( w == 1280 && h == 720 )
        {
            bitrate = 4096000;
        }
        else if ( w == 720 && h == 480 )
        {
            bitrate = 512000;
        }
        else if ( w == 352 && h == 288)
        {
            bitrate = 256000;
        }		
        else if ( w == 320 && h == 240)
        {
            bitrate = 256000;
        }
    }
    else if(strstr(quality, "high") != NULL)
    {
        duration = 60;

        if( w == 1920 && h == 1080 )
        {
            //ActionsCode(author:liuyiguang, change_code)
            bitrate = 16384000;
        }
        else if( w == 1280 && h == 720 )
        {
            bitrate = 8192000;
        }
        else if ( w == 720 && h == 480 )
        {
            bitrate = 4096000;
        }
        else if ( w == 352 && h == 288)
        {
            bitrate = 1024000;
        }
        else if ( w == 320 && h == 240)
        {
            bitrate = 1024000;
        }
    }
    else
    {
        duration = 60;

        if( w == 1920 && h == 1080 )
        {
            //ActionsCode(author:liuyiguang, change_code)
            bitrate = 12288000;
        }
        else if( w == 1280 && h == 720 )
        {
            bitrate = 8192000;
        }
        else if ( w == 720 && h == 480 )
        {
            bitrate = 4096000;
        }
        else if ( w == 352 && h == 288)
        {
            bitrate = 1024000;
        }		
        else if ( w == 320 && h == 240)
        {
            bitrate = 1024000;
        }
        
    }
    snprintf(buf, bufsize-1, mVideoSizeTemplate, quality, duration, bitrate, w, h);
}

void MediaProfileGenerator::genSizes(int id, char *buf, int bufsize)
{
    unsigned int i = 0;
    char *sizeBuf = NULL;
    int min, max;
    CameraFrameSize maxSize, minSize;

    char qualityName[MAX_QUALITY_NAME_SIZE];

    *buf = '\0';

    min = getMinSize(id);
    max = getMaxSize(id);
    if(max < 0 || min < 0)
    {
        return;
    }
    maxSize = mFrameSizes[id].itemAt(max);
    minSize = mFrameSizes[id].itemAt(min);

    sizeBuf = (char *)malloc(MAX_VIDEOSIZE_INFO_SIZE);
    if(sizeBuf == NULL)
    {
        return;
    }

    genVideoSizeInfo(sizeBuf, MAX_VIDEOSIZE_INFO_SIZE,  "low", minSize.width, minSize.height);
    strlcat(buf, sizeBuf, bufsize - strlen(buf));
    genVideoSizeInfo(sizeBuf, MAX_VIDEOSIZE_INFO_SIZE,  "timelapselow", minSize.width, minSize.height);
    strlcat(buf, sizeBuf, bufsize - strlen(buf));

    for(i =0; i < mFrameSizes[id].size(); i++)
    {
        CameraFrameSize size = mFrameSizes[id].itemAt(i);
        const char *name = getQualityName(size.width, size.height);
        if(name != NULL)
        {
            genVideoSizeInfo(sizeBuf, MAX_VIDEOSIZE_INFO_SIZE,  
                name, size.width, size.height);
            strlcat(buf, sizeBuf, bufsize - strlen(buf));

            memset((char *)&qualityName, 0, MAX_QUALITY_NAME_SIZE);
            snprintf((char *)&qualityName, MAX_QUALITY_NAME_SIZE, "timelapse%s", name);

            genVideoSizeInfo(sizeBuf, MAX_VIDEOSIZE_INFO_SIZE,  
                (char *)&qualityName, size.width, size.height);
            strlcat(buf, sizeBuf, bufsize - strlen(buf));
        }
    }

    genVideoSizeInfo(sizeBuf, MAX_VIDEOSIZE_INFO_SIZE,  "high", maxSize.width, maxSize.height);
    strlcat(buf, sizeBuf, bufsize - strlen(buf));
    genVideoSizeInfo(sizeBuf, MAX_VIDEOSIZE_INFO_SIZE,  "timelapsehigh", maxSize.width, maxSize.height);
    strlcat(buf, sizeBuf, bufsize - strlen(buf));

    if(sizeBuf != NULL)
    {
        free(sizeBuf);
    }
}

status_t MediaProfileGenerator::genMediaProfile(const char *savepath)
{
    int fd = -1;
    int ret = 0;
    int id = 0;

    char *buf0 = NULL;
    char *buf1 = NULL;
    char *tmp_buf0 = NULL;
    char *tmp_buf1 = NULL;
    char *magic_buf = NULL;
    int magic_buf_size = 512;
    int len = 0;
    struct tm *tm_ptr;
    time_t cur_time;

    mode_t oldmode;
#if 0
    if (access(savepath, R_OK|W_OK) != 0)
    {
        char dir[PATH_MAX];
        char *p = NULL;

        ALOGD("File(%s) access failed, try to mkdir!", savepath);

        //try to mkdir 
        strlcpy(dir, savepath, PATH_MAX); 
        p=strrchr(dir, '/');
        if(p == NULL)
        {
            return -1;
        }
        *p = '\0';
        oldmode = umask(0);
        create_dir(dir);
        umask(oldmode);
    }
#endif
    buf0 = (char *)malloc(MAX_PROFILE_SIZE);
    if(buf0 == NULL)
    {
        ret = -1;
        goto exit;
    }
    buf1 = (char *)malloc(MAX_PROFILE_SIZE);
    if(buf1 == NULL)
    {
        ret = -1;
        goto exit;
    }
    oldmode = umask(0);
    fd = open(savepath, O_TRUNC|O_RDWR|O_CREAT, 0666);
    umask(oldmode);
    if(fd < 0)
    {
        ALOGE("open media_profiles.xml file (%s) error, (%s)!\n", savepath, strerror(errno));
        ret = -1;
        goto exit;
    }

    WRITE_DATA(fd, XML_HEADER, strlen(XML_HEADER));

    len = sizeof(DONOT_MODIFY_WARNING)+128;
    tmp_buf0 = (char *)malloc(len);
    tmp_buf1 = (char *)malloc(128);
    if(tmp_buf0 == NULL || tmp_buf1 == NULL)
    {
        ret = -1;
        goto exit;
    }

    cur_time =time(0);
    tm_ptr = localtime(&cur_time);
    strftime(tmp_buf1,128-1,"%F %T", tm_ptr);
    snprintf(tmp_buf0, len-1, DONOT_MODIFY_WARNING, tmp_buf1);

    WRITE_DATA(fd, tmp_buf0, strlen(tmp_buf0));

    //header
    WRITE_DATA(fd, mHeader, mHeaderLen);
    
    *buf0 = *buf1 = '\0';
    for(id = 0; id < mCameraNum; id++)
    {
        if(mFrameSizes[id].size() <=0)
        {
            CameraFrameSize size(640,480);
            mFrameSizes[id].add(size);
        }
        genSizes(id, buf1, MAX_PROFILE_SIZE);
        snprintf(buf0, MAX_PROFILE_SIZE-1, mProfileTemplate, id, buf1);
        WRITE_DATA(fd, buf0, strlen(buf0));
    }

    WRITE_DATA(fd, mFooter, mFooterLen);

    magic_buf = (char *)malloc(magic_buf_size);
    if(magic_buf == NULL)
    {
        ret = -1;
        goto exit;
    }
    snprintf(magic_buf, magic_buf_size-1, PROFILE_MAGIC, CAMERAHAL_VERSION, (char *)&mSensorNames[0], (char *)&mSensorNames[1]);

    WRITE_DATA(fd, magic_buf, strlen(magic_buf));

exit:

    if(magic_buf)
    {
        free(magic_buf);
    }
    if(tmp_buf0)
    {
        free(tmp_buf0);
    }
    if(tmp_buf1)
    {
        free(tmp_buf1);
    }
    if(fd >= 0)
    {
        close(fd);
    }
    if(buf0)
    {
        free(buf0);
    }
    if(buf1)
    {
        free(buf1);
    }
    return ret;
}

bool MediaProfileGenerator::checkMediaProfile(const char *path, const char *sensor0, const char *sensor1)
{
    int fd = -1;
    unsigned long filelen = 0;
    bool ret = false;
    char *magicFromFile = NULL;
    char *magicBuf = NULL;
    int magicBufSize = 512;

    magicBuf = (char *)malloc(magicBufSize);
    if(magicBuf == NULL)
    {
        ret = false;
        goto exit;
    }

    snprintf(magicBuf, magicBufSize-1, PROFILE_MAGIC, CAMERAHAL_VERSION, sensor0, sensor1);

    if (access(path, R_OK) != 0)
    {
        ALOGD("File(%s) access failed!", path);
        ret = false;
        goto exit;
    }

    fd = open(path, O_RDONLY );
    if(fd < 0)
    {
        ALOGE("Open template file (%s) error!\n", path);
        ret = false;
        goto exit;
    }

    filelen = lseek(fd, 0, SEEK_END);  
    if (filelen <= 0 )  
    {  
        ALOGE("lseek failed (%s) !\n", strerror(errno));  
        ret = false;
        goto exit;
    }  

    if (filelen < strlen(magicBuf))
    {
        ALOGD("file length is shorter than strlen(PROFILE_MAGIC)");
        ret = false;
        goto exit;
    }

    // seek to the start of the file
    lseek(fd, filelen-strlen(magicBuf), SEEK_SET);  

    //(filelen+1) for '\0'
    magicFromFile = (char *)malloc(strlen(magicBuf) + 1);
    if(magicFromFile == NULL)
    {
        ALOGE("malloc failed for (%d) !\n", (strlen(magicBuf)+1));  
        ret = false;
        goto exit;
    }

    //read all contents
    READ_DATA(fd, magicFromFile, strlen(magicBuf));

    magicFromFile[strlen(magicBuf)]='\0';

    ALOGD("magicFromFile = %s", magicFromFile);
    if(strncmp(magicFromFile, magicBuf, strlen(magicBuf)) == 0)
    {
        ret = true;
        goto exit;
    }

    exit:
    if(magicBuf != NULL)
    {
        free(magicBuf);
    }
    if(magicFromFile != NULL)
    {
        free(magicFromFile);
    }
    if(fd >= 0)
    {
        close(fd);
    }
    return ret;
}

};

