#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utils/threads.h>
#include "binder/MemoryBase.h"
#include "binder/MemoryHeapBase.h"
#include <utils/threads.h>

namespace android
{

class  BufferProvider;  
/**
  * Class used for allocating memory for JPEG bit stream buffers, output buffers of camera in no overlay case
  */
class MemoryManager : public BufferProvider, public virtual RefBase
{
public:
    MemoryManager():mIonFd(-1) { }

    ///Initializes the memory manager creates any resources required
    status_t initialize()
    {
        return NO_ERROR;
    }

    int setErrorHandler(ErrorNotifier *errorNotifier);
    virtual void* allocateBuffer(int width, int height, const char* format, int &bytes, int numBufs);
    virtual uint32_t * getOffsets();
    virtual void* getVaddrs();
    virtual int getFd() ;
    virtual int freeBuffer(void* buf);
    virtual void * getPhys(void* buf) ;

private:

    sp<ErrorNotifier> mErrorNotifier;
    int mIonFd;
    KeyedVector<unsigned int, unsigned int> mIonHandleMap;
    KeyedVector<unsigned int, unsigned int> mIonFdMap;
    KeyedVector<unsigned int, unsigned int> mIonBufLength;
    KeyedVector<unsigned int, unsigned int> mIonBufPhyMap;
};

}
#endif
