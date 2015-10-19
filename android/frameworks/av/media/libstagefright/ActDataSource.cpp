/*
 * Copyright (C) 2009 The Android Open Source Project
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
//#define LOG_NDEBUG 0
#define LOG_TAG "ActDataSource"
#include <utils/Log.h>
#include <media/stagefright/foundation/ADebug.h>

#include "ActDataSource.h"
namespace android {
#define DATA_LOGV(x, ...)    //ALOGV(x, ##__VA_ARGS__) 

typedef struct {
    storage_io_t io;

    off64_t mOffset;
    off64_t mFileSize;

    sp<DataSource> mSource;
} storage_internal_t;

typedef struct {
    stream_input_t input;
    storage_io_t *storage_io;

} stream_manager_t;

int storage_read(void *buf, int size, int count, storage_io_t *io) {
	ALOGV("storage_read: size: %d count: %d io: %p", size, count, (void *)io);
    storage_internal_t * const storage = (storage_internal_t *)io;
	size_t need_size = (size_t)(size * count);
	if (need_size == 0) {
		return 0;
	}
	CHECK(need_size > 0);
    ssize_t n = storage->mSource->readAt(storage->mOffset, (void*)buf, need_size);
	
	#if 1
	if ((n == need_size)||((n>0) && (n < need_size))) {
		storage->mOffset += n;
	}else if (n == 0){
		DATA_LOGV("storage_read() read upto the end of stream !!! storage->mOffset: %lld need_size: %d", storage->mOffset, need_size);
	}else {
		ALOGE("storage_read() read error !!! storage->mOffset: %lld need_size: %d", storage->mOffset, need_size);
	}
	#else
    if (n > 0)
    {
        storage->mOffset += n;
    }
	#endif
	
	ALOGV("storage_read: end");
    return n / size;
}

static int storage_seek(storage_io_t *io, off64_t offset, int whence) {
    storage_internal_t * const storage = (storage_internal_t *)io;

    if (whence == SEEK_CUR) {
        storage->mOffset += offset;
    } else if (whence == SEEK_SET) {
        storage->mOffset = offset;
    } else {
        storage->mOffset = storage->mFileSize + offset;
    }

    return 0;
}

static off64_t storage_tell(storage_io_t *io) {
    storage_internal_t * const storage = (storage_internal_t *)io;

    return storage->mOffset;
}

static int stream_read(stream_input_t *input, unsigned char *buf, unsigned int len)
{
    int nread = 0;
    stream_manager_t *stream_manager = (stream_manager_t *)input;

    if((input == NULL) || (buf == NULL)) {
        ALOGE("stream_read: error input: %p buf: %p\n", (void *)input, (void *)buf);
        return 0;
    }

    nread = stream_manager->storage_io->read(buf, 1, len, stream_manager->storage_io);

    return nread;
}

static int stream_seek(stream_input_t *input, off64_t offset, int original)
{
    stream_manager_t *stream_manager = (stream_manager_t *)input;
	DATA_LOGV("stream_seek: input(0x%x) offset(%lld) original(%d) then call storage_seek", input, offset, original);
    if(input == NULL){
        ALOGE("stream_seek()->(input == NULL) error!!!\n");
        return -1;
    }
    if (original == DSEEK_SET){
        original = SEEK_SET;
    }else if (original == DSEEK_CUR) {
        original = SEEK_CUR;
    } else if (original == DSEEK_END){
        original = SEEK_END;
    } else {
        ALOGE("stream_seek(): offset(%lld)  original(%d) error!!! \n", offset, original);
        return -1;
    }

    return stream_manager->storage_io->seek(stream_manager->storage_io, offset, original);
}

static off64_t stream_tell(stream_input_t *input)
{
    stream_manager_t *stream_manager = (stream_manager_t *)input;

    if(input == NULL) {
		ALOGE("create_storage_io()->(input==NULL) error!!!!");
        return 0;
    }

    return stream_manager->storage_io->tell(stream_manager->storage_io);
}
//---------------------------------------------------------
off64_t init_storage_io(storage_io_t *io, const sp<DataSource> &source)
{
    storage_internal_t * const storage = (storage_internal_t *)io;
    off64_t  size = 0;

    storage->mSource = source;
    storage->mOffset = 0;
    if (storage->mSource->getSize(&size) < 0) {
        storage->mFileSize = 0;
    } else { 
        storage->mFileSize = size;
    }

    return storage->mFileSize;
}

void dispose_storage_io(storage_io_t *io)
{
    storage_internal_t *storage = (storage_internal_t *)io;

    if (storage){
        delete storage;
        storage = NULL;
    }
}

storage_io_t *create_storage_io(void)
{
    storage_internal_t *storage;
    status_t status = 0;
    storage = new storage_internal_t;
    if (storage == NULL){
    	ALOGE("create_storage_io()-> return NULL error!!!!");
        return NULL;
    }

    storage->io.read = storage_read;
    storage->io.write = NULL;
    storage->io.seek = storage_seek;
    storage->io.tell = storage_tell;

    return &storage->io;
}
//---------------------------------------------------------
off64_t stream_input_init(stream_input_t *input, const sp<DataSource> &source)
{
    stream_manager_t *stream_manager = (stream_manager_t *)input;
    int ret = 0;

    return init_storage_io(stream_manager->storage_io, source);
}

void stream_input_dispose(stream_input_t *input)
{
    stream_manager_t *stream_manager = (stream_manager_t *)input;

    if (stream_manager){
        dispose_storage_io(stream_manager->storage_io);
        actal_free(stream_manager);
        stream_manager = NULL;
    }
}

stream_input_t *stream_input_open(void)
{
    stream_manager_t *stream_manager = NULL;

    stream_manager = (stream_manager_t *)actal_malloc(sizeof(stream_manager_t));
    if (stream_manager == NULL){
        ALOGE("%s: disk manager open error\n", __FILE__);
        return NULL;
    }

    actal_memset(stream_manager, 0, sizeof(stream_manager_t));

    stream_manager->storage_io = create_storage_io();
    if (stream_manager->storage_io == NULL){
        goto exit0;
    }

    stream_manager->input.read = stream_read;
    stream_manager->input.write = NULL;
    stream_manager->input.seek = stream_seek;
    stream_manager->input.tell = stream_tell;

    return &stream_manager->input;

exit0:
    actal_free(stream_manager);
    return NULL;
}
} // namespace android
