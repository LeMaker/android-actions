#include "id3parse.h"

namespace android {
int ID3_fread(void *buffer, size_t size, size_t count, void* stream )
{   
    ID3file_t* source = (ID3file_t*)stream;
    mmm_off_t offset = (mmm_off_t)source->mOffset;
    size_t n = source->mSource->readAt64(offset, (void*)buffer, (size_t)size * count);
    if (n > 0)
    {
        source->mOffset += n;
    }
    return n;
    
}

int ID3_fseek(void *stream, long offset, int whence)
{
    ID3file_t* source = (ID3file_t*)stream;
    if (whence == SEEK_CUR) {
        source->mOffset += offset;
    } else if (whence == SEEK_SET) {
        source->mOffset = offset;
    } else {
        source->mOffset = source->mFileSize + offset;
    }
    return 0;
}

int ID3_getfilelength(void *stream)
{
    ID3file_t* source = (ID3file_t*)stream;
    return (int)source->mFileSize;
}
}
