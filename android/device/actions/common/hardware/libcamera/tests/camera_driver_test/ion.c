/*
 *  ion.c
 *
 * Memory Allocator functions for ion
 *
 *   Copyright 2011 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#define LOG_TAG "ion"
//#include <cutils/log.h>
#define LOGE    printf

#include <linux/ion.h>
#include <linux/ion-owl.h>
#include <ion/ion.h>


ion_user_handle_t ion_handle_t;
int ion_fd  = -1;
int ion_map_fd;

#if 0
int ion_open()
{
        int fd = open("/dev/ion", O_RDWR);
        if (fd < 0)
                LOGE("open /dev/ion failed!\n");
        return fd;
}

int ion_close(int fd)
{
        return close(fd);
}

static int ion_ioctl(int fd, int req, void *arg)
{
        int ret = ioctl(fd, req, arg);
        if (ret < 0) {
                LOGE("ioctl %d failed with code %d: %s\n", req,
                       ret, strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_alloc(int fd, size_t len, size_t align, unsigned int flags,
              ion_user_handle_t *handle)
{
        int ret;
        struct ion_allocation_data data = {
                .len = len,
                .align = align,
                .flags = flags,
        };

        ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
        if (ret < 0)
                return ret;
        *handle = data.handle;
        printf("ion_alloc ok data.handle = %p  \n",data.handle);
        return ret;
}

int ion_free(int fd, ion_user_handle_t handle)
{
        struct ion_handle_data data = {
                .handle = handle,
        };
        return ion_ioctl(fd, ION_IOC_FREE, &data);
}

int ion_map(int fd, ion_user_handle_t handle, size_t length, int prot,
            int flags, long long offset, unsigned char **ptr, int *map_fd)
{
        struct ion_fd_data data = {
                .handle = handle,
        };
        int ret = ion_ioctl(fd, ION_IOC_MAP, &data);
        if (ret < 0)
                return ret;
        *map_fd = data.fd;
        if (*map_fd < 0) {
                LOGE("map ioctl returned negative fd\n");
                return -EINVAL;
        }
        *ptr = mmap(NULL, length, prot, flags, *map_fd, offset);
        if (*ptr == MAP_FAILED) {
                LOGE("mmap failed: %s\n", strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_share(int fd, ion_user_handle_t handle, int *share_fd)
{
        int map_fd;
        struct ion_fd_data data = {
                .handle = handle,
        };
        int ret = ion_ioctl(fd, ION_IOC_SHARE, &data);
        if (ret < 0)
                return ret;
        *share_fd = data.fd;
        if (*share_fd < 0) {
                LOGE("map ioctl returned negative fd\n");
                return -EINVAL;
        }
        return ret;
}

int ion_import(int fd, int share_fd, ion_user_handle_t *handle)
{
        struct ion_fd_data data = {
                .fd = share_fd,
        };
        int ret = ion_ioctl(fd, ION_IOC_IMPORT, &data);
        if (ret < 0)
                return ret;
        *handle = data.handle;
        return ret;
}
int ion_phys(int fd, ion_user_handle_t handle, unsigned long *phys)
{
          int ret;
        struct owl_ion_phys_data phys_data = {
                .handle = handle,
        };
        
        struct ion_custom_data data = {
                .cmd = OWL_ION_GET_PHY,
                .arg = (unsigned long)&phys_data,
        }; 
        
        ret = ion_ioctl(fd, ION_IOC_CUSTOM, &data);

        if (ret < 0)
            return ret;
        *phys = phys_data.phys_addr;
        return ret;

}
#endif
int ion_count = 0;
/*利用ion分配内存，成功返回0*/
int sys_mem_allocate(unsigned int size, void **vir_addr, ion_user_handle_t * p_ion_handle)
{
    int ret;

    if (!ion_count)
    {
        ion_fd = ion_open();
      if(ion_fd < 0){
          printf("ion_open failed\n");
          return -1;
      }
      printf("ion_open ok ion_fd = %d \n",ion_fd);
    }
    ret = ion_alloc(ion_fd, size, 0, 1,0, &ion_handle_t);
    if(ret)
    {
        printf("%s failed: %s\n", __func__, strerror(ret));
        return -1;
    }
    *p_ion_handle  = ion_handle_t;
    ret = ion_map(ion_fd, ion_handle_t, size, PROT_READ | PROT_WRITE, MAP_SHARED, 0, (unsigned char **)vir_addr, &ion_map_fd);
    if (ret){
    printf("ion_map error \n");
    return -1 ;
    }
    printf("ion_map ok \n");
    ion_count++;
    return 0;

}

int sys_get_phyaddr(unsigned long *phys, ion_user_handle_t p_ion_handle)
{
    int ret;
    ret = ion_phys(ion_fd,p_ion_handle, phys);
    if(ret){
        printf("ion get phys addr error \n");
        return -1;
    }
    return 0;
}


int sys_mem_free(ion_user_handle_t p_ion_handle)
{
  int ret;
    ret = ion_free(ion_fd, p_ion_handle);
    if (ret)
    {
        printf("ion mem free error \n");
        return ret;
    }
    ion_count--;
    if (!ion_count)
    {
        ion_close(ion_fd);
    }
    return 0;

}
