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




#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "ActCameraParameters.h"

#include "MemoryManager.h"

#include <ion/ion.h>



namespace android
{

///@todo Move these constants to a common header file, preferably in tiler.h
#define STRIDE_8BIT (4 * 1024)
#define STRIDE_16BIT (4 * 1024)

#define ALLOCATION_2D 2

#define ION_HEAP_MASK (1<<0)

///Utility Macro Declarations

/*--------------------MemoryManager Class STARTS here-----------------------------*/
/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
void* MemoryManager::allocateBuffer(int width, int height, const char* format, int &bytes, int numBufs)
{
    LOG_FUNCTION_NAME;

    if(mIonFd < 0)
    {
        mIonFd = ion_open();
        if(mIonFd < 0)
        {
            CAMHAL_LOGEA("ion_open failed!!!");
            return NULL;
        }
    }

    ///We allocate numBufs+1 because the last entry will be marked NULL to indicate end of array, which is used when freeing
    ///the buffers
    const uint numArrayEntriesC = (uint)(numBufs+1);

    ///Allocate a buffer array
    uint32_t *bufsArr = new uint32_t [numArrayEntriesC];
    if(!bufsArr)
    {
        CAMHAL_LOGEB("Allocation failed when creating buffers array of %d uint32_t elements", numArrayEntriesC);
        goto error;
    }

    ///Initialize the array with zeros - this will help us while freeing the array in case of error
    ///If a value of an array element is NULL, it means we didnt allocate it
    memset(bufsArr, 0, sizeof(*bufsArr) * numArrayEntriesC);

    //2D Allocations are not supported currently
    if(bytes != 0)
    {
        ion_user_handle_t handle;
        int mmap_fd;
        unsigned int phy;

        ///1D buffers
        for (int i = 0; i < numBufs; i++)
        {
            int aligned = (bytes+0xfff)&(~0xfff);

            //alloc uncache ion buffer
            int ret = ion_alloc(mIonFd, aligned, 0, ION_HEAP_MASK, 0, &handle);
            //int ret = ion_alloc(mIonFd, aligned, 0, ION_HEAP_MASK,  &handle);
            if(ret < 0)
            {
                CAMHAL_LOGEB("ion_alloc resulted in error %d", ret);
                goto error;
            }

            CAMHAL_LOGDB("Before mapping, handle = %p, nSize = %d", handle, bytes);
            if ((ret = ion_map(mIonFd, handle, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, 0,
                               (unsigned char**)&bufsArr[i], &mmap_fd)) < 0)
            {
                CAMHAL_LOGEB("Userspace mapping of ION buffers returned error %d", ret);
                ion_free(mIonFd, handle);
                goto error;
            }

            ret = ion_phys(mIonFd, handle, (unsigned long *)&phy);
            if(ret < 0)
            {
                //ActionsCode(author:liuyiguang, change_code)
                //munmap((void *)bufsArr[i], bytes);
                munmap((void *)(long)bufsArr[i], bytes);
                ion_free(mIonFd, handle);
                goto error;
            }

            mIonHandleMap.add(bufsArr[i], (unsigned int)handle);
            mIonFdMap.add(bufsArr[i], (unsigned int) mmap_fd);
            mIonBufLength.add(bufsArr[i], (unsigned int) bytes);
            mIonBufPhyMap.add(bufsArr[i], (unsigned int) phy);
        }

    }
    else // If bytes is not zero, then it is a 2-D tiler buffer request
    {
    }

    LOG_FUNCTION_NAME_EXIT;

    return (void*)bufsArr;

error:
    ALOGE("Freeing buffers already allocated after error occurred");
    if(bufsArr)
        freeBuffer(bufsArr);

    if ( NULL != mErrorNotifier.get() )
    {
        mErrorNotifier->errorNotify(-ENOMEM);
    }

    if (mIonFd >= 0)
    {
        ion_close(mIonFd);
        mIonFd = -1;
    }

    LOG_FUNCTION_NAME_EXIT;
    return NULL;
}

//TODO: Get needed data to map tiler buffers
//Return dummy data for now
uint32_t * MemoryManager::getOffsets()
{
    LOG_FUNCTION_NAME;


    LOG_FUNCTION_NAME_EXIT;

    return NULL;
}

int MemoryManager::getFd()
{
    LOG_FUNCTION_NAME;


    LOG_FUNCTION_NAME_EXIT;

    return -1;
}
void * MemoryManager::getVaddrs()
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return NULL;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
void * MemoryManager::getPhys(void* buf)
{
    LOG_FUNCTION_NAME;

    ssize_t index;
    void *phy;

    //ActionsCode(author:liuyiguang, change_code)
    //index = mIonBufPhyMap.indexOfKey((unsigned int)buf);
    index = mIonBufPhyMap.indexOfKey((long)buf);

    if(index < 0)
    {
        return NULL;
    }
    //ActionsCode(author:liuyiguang, change_code)
    //phy = (void *) mIonBufPhyMap.valueAt(index);
    phy = (void *)(long) mIonBufPhyMap.valueAt(index);

    LOG_FUNCTION_NAME_EXIT;

    return phy;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
int MemoryManager::freeBuffer(void* buf)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    uint32_t *bufEntry = (uint32_t*)buf;

    if(!bufEntry)
    {
        CAMHAL_LOGEA("NULL pointer passed to freebuffer");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    while(*bufEntry)
    {
        unsigned int ptr = (unsigned int) *bufEntry++;
        if(mIonBufLength.valueFor(ptr))
        {
            //ActionsCode(author:liuyiguang, change_code)
            //munmap((void *)ptr, mIonBufLength.valueFor(ptr));
            munmap((void *)(long)ptr, mIonBufLength.valueFor(ptr));
            close(mIonFdMap.valueFor(ptr));
            ion_free(mIonFd, (ion_user_handle_t)mIonHandleMap.valueFor(ptr));
            mIonHandleMap.removeItem(ptr);
            mIonBufLength.removeItem(ptr);
            mIonFdMap.removeItem(ptr);
        }
        else
        {
            CAMHAL_LOGEA("Not a valid Memory Manager buffer");
        }
    }

    ///@todo Check if this way of deleting array is correct, else use malloc/free
    uint32_t * bufArr = (uint32_t*)buf;
    delete [] bufArr;

    if(mIonBufLength.size() == 0)
    {
        if(mIonFd >= 0)
        {
            ion_close(mIonFd);
            mIonFd = -1;
        }
    }
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t MemoryManager::setErrorHandler(ErrorNotifier *errorNotifier)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL == errorNotifier )
    {
        CAMHAL_LOGEA("Invalid Error Notifier reference");
        ret = -EINVAL;
    }

    if ( NO_ERROR == ret )
    {
        mErrorNotifier = errorNotifier;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

};


/*--------------------MemoryManager Class ENDS here-----------------------------*/
