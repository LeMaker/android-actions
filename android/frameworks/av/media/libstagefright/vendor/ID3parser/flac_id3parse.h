#ifndef _FLAC_ID3PARSE_H_
#define _FLAC_ID3PARSE_H_

#include "id3parse.h"
namespace android {
#ifdef __cplusplus
extern "C" {
#endif

enum
{
    FLAC_STREAMINFO = 0,
    FLAC_PADDING = 1,
    FLAC_APPLICATION = 2,
    FLAC_SEEKTABLE = 3,
    FLAC_VORBIS_COMMENT = 4,
    FLAC_CUESHEET = 5,
    FLAC_PICTURE = 6
};

#ifdef __cplusplus
}
#endif // __cplusplus
} // namespace android

#endif
