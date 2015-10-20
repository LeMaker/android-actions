/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "SkImageDecoder.h"
#include "SkBitmap.h"
#include "SkImagePriv.h"
#include "SkPixelRef.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkCanvas.h"
#ifndef ANDROID_DEFAULT_CODE
#include "SkImageDecoder_Act.h"

#include <cutils/properties.h>
static int	sdram_capacity = -1;
static int	decode_standard = -1;

static int _max_width = -1;
static int _max_height = -1;

static int get_sdram_capacity(void);
static int get_decode_standard(void);
static void get_hw_codec_max_wh(int *width, int *height );
static int get_enhance_system(void);
#endif /*ANDROID_DEFAULT_CODE*/
SkImageDecoder::SkImageDecoder()
    : fPeeker(NULL)
#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    , fChooser(NULL)
#endif
    , fAllocator(NULL)
    , fSampleSize(1)
    , fDefaultPref(kUnknown_SkColorType)
    , fDitherImage(true)
#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
    , fUsePrefTable(false)
#endif
    , fSkipWritingZeroes(false)
    , fPreferQualityOverSpeed(false)
    , fRequireUnpremultipliedColors(false) {
    
#ifndef ANDROID_DEFAULT_CODE
    int hwMaxWidth, hwMaxHeight;

    ACT_setSdramCapacity(get_sdram_capacity());
    ACT_setDecodeStandard(get_decode_standard());
	
    get_hw_codec_max_wh(&hwMaxWidth, &hwMaxHeight);
    ACT_setHWCodecMaxWidth(hwMaxWidth);
    ACT_setHWCodecMaxHeight(hwMaxHeight);
	ACT_setDoEnhance(get_enhance_system());
#endif /*ANDROID_DEFAULT_CODE*/
}

SkImageDecoder::~SkImageDecoder() {
    SkSafeUnref(fPeeker);
#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    SkSafeUnref(fChooser);
#endif
    SkSafeUnref(fAllocator);
}

void SkImageDecoder::copyFieldsToOther(SkImageDecoder* other) {
    if (NULL == other) {
        return;
    }
    other->setPeeker(fPeeker);
#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    other->setChooser(fChooser);
#endif
    other->setAllocator(fAllocator);
    other->setSampleSize(fSampleSize);
#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
    if (fUsePrefTable) {
        other->setPrefConfigTable(fPrefTable);
    } else {
        other->fDefaultPref = fDefaultPref;
    }
#endif
    other->setDitherImage(fDitherImage);
    other->setSkipWritingZeroes(fSkipWritingZeroes);
    other->setPreferQualityOverSpeed(fPreferQualityOverSpeed);
    other->setRequireUnpremultipliedColors(fRequireUnpremultipliedColors);
}

SkImageDecoder::Format SkImageDecoder::getFormat() const {
    return kUnknown_Format;
}

const char* SkImageDecoder::getFormatName() const {
    return GetFormatName(this->getFormat());
}

const char* SkImageDecoder::GetFormatName(Format format) {
    switch (format) {
        case kUnknown_Format:
            return "Unknown Format";
        case kBMP_Format:
            return "BMP";
        case kGIF_Format:
            return "GIF";
        case kICO_Format:
            return "ICO";
        case kPKM_Format:
            return "PKM";
        case kKTX_Format:
            return "KTX";
        case kJPEG_Format:
            return "JPEG";
        case kPNG_Format:
            return "PNG";
        case kWBMP_Format:
            return "WBMP";
        case kWEBP_Format:
            return "WEBP";
        default:
            SkDEBUGFAIL("Invalid format type!");
    }
    return "Unknown Format";
}

SkImageDecoder::Peeker* SkImageDecoder::setPeeker(Peeker* peeker) {
    SkRefCnt_SafeAssign(fPeeker, peeker);
    return peeker;
}

#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
SkImageDecoder::Chooser* SkImageDecoder::setChooser(Chooser* chooser) {
    SkRefCnt_SafeAssign(fChooser, chooser);
    return chooser;
}
#endif

SkBitmap::Allocator* SkImageDecoder::setAllocator(SkBitmap::Allocator* alloc) {
    SkRefCnt_SafeAssign(fAllocator, alloc);
    return alloc;
}

void SkImageDecoder::setSampleSize(int size) {
    if (size < 1) {
        size = 1;
    }
    fSampleSize = size;
}


#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
// TODO: change Chooser virtual to take colorType, so we can stop calling SkColorTypeToBitmapConfig
//
bool SkImageDecoder::chooseFromOneChoice(SkColorType colorType, int width, int height) const {
    Chooser* chooser = fChooser;
    
    if (NULL == chooser) {    // no chooser, we just say YES to decoding :)
        return true;
    }
    chooser->begin(1);
    chooser->inspect(0, SkColorTypeToBitmapConfig(colorType), width, height);
    return chooser->choose() == 0;
}
#endif

bool SkImageDecoder::allocPixelRef(SkBitmap* bitmap,
                                   SkColorTable* ctable) const {
    return bitmap->allocPixels(fAllocator, ctable);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
void SkImageDecoder::setPrefConfigTable(const PrefConfigTable& prefTable) {
    fUsePrefTable = true;
    fPrefTable = prefTable;
}
#endif

// TODO: use colortype in fPrefTable, fDefaultPref so we can stop using SkBitmapConfigToColorType()
//
SkColorType SkImageDecoder::getPrefColorType(SrcDepth srcDepth, bool srcHasAlpha) const {
    SkColorType ct = fDefaultPref;
#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG

    if (fUsePrefTable) {
        // Until we kill or change the PrefTable, we have to go into Config land for a moment.
        SkBitmap::Config config = SkBitmap::kNo_Config;
        switch (srcDepth) {
            case kIndex_SrcDepth:
                config = srcHasAlpha ? fPrefTable.fPrefFor_8Index_YesAlpha_src
                                     : fPrefTable.fPrefFor_8Index_NoAlpha_src;
                break;
            case k8BitGray_SrcDepth:
                config = fPrefTable.fPrefFor_8Gray_src;
                break;
            case k32Bit_SrcDepth:
                config = srcHasAlpha ? fPrefTable.fPrefFor_8bpc_YesAlpha_src
                                     : fPrefTable.fPrefFor_8bpc_NoAlpha_src;
                break;
        }
        // now return to SkColorType land
        ct = SkBitmapConfigToColorType(config);
    }
#endif
    return ct;
}

#ifndef ANDROID_DEFAULT_CODE

static int get_enhance_system(void)
{
    char value[PROPERTY_VALUE_MAX]; 
    property_get("sys.image_enhanced_system", value, "0");
    return atoi(value);
}

static int get_sdram_capacity(void)
{
    if (sdram_capacity != -1)
    {       
        return sdram_capacity;
    }
    char value[PROPERTY_VALUE_MAX]; 
    property_get("system.ram.total", value, "1024");
    sdram_capacity=atoi(value);

	SkDebugf("sdram_cap = %d %s",sdram_capacity, __FUNCTION__); 
	return sdram_capacity;
}

static int get_decode_standard(void)
{
    if (decode_standard != -1)
    {       
        return decode_standard;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("ro.skia.img.decode.standard",value,"NATIVE");
	
    decode_standard = !strcmp(value,"ACTIONS")? 0: 1;
	
	SkDebugf("decode_standard = %d, %s",decode_standard, __FUNCTION__);

	return decode_standard;

}

static void get_hw_codec_max_wh(int *width, int *height )
{
    if(_max_width != -1 && _max_height != -1)
    {
        *width = _max_width;
        *height = _max_height;
        return;
    }
    char ic_type[PROPERTY_VALUE_MAX];

    property_get("ro.dvfslevel", ic_type, "NORMAL");

    SkDebugf(" ic type = %s \n", ic_type); 

    if(!strcmp(ic_type, "0x7021") || !strcmp(ic_type, "0x7023"))
    {
        _max_width = 8176;
        _max_height = 8176;
    }else if(!strcmp(ic_type, "0x7029")){
        _max_width = 30000;
        _max_height  = 30000;
    }else if(!strcmp(ic_type, "0x705x") || !strcmp(ic_type, "0x705xTC")){
        _max_width = 30000;
        _max_height = 15000;
    }else{
        _max_width = 8176;
        _max_height = 8176;
    }

    SkDebugf(" hw codec max_width = %d, max_height =%d\n",
		_max_width, _max_height);

    *width = _max_width;
    *height = _max_height;
    return;
}

#endif /*ANDROID_DEFAULT_CODE*/

SkImageDecoder::Result SkImageDecoder::decode(SkStream* stream, SkBitmap* bm, SkColorType pref,
                                              Mode mode) {
#ifndef ANDROID_DEFAULT_CODE   
    int width,height;
    unsigned char *enhance_src = NULL;
    int sdram_cap;
    int native_standard = -1;

	sdram_cap = ACT_getSdramCapacity();;
	native_standard = ACT_getDecodeStandard();

#endif	/*ANDROID_DEFAULT_CODE*/
    // we reset this to false before calling onDecode
    fShouldCancelDecode = false;
    // assign this, for use by getPrefColorType(), in case fUsePrefTable is false
    fDefaultPref = pref;
	
#ifndef ANDROID_DEFAULT_CODE
	int img_enhance;

    img_enhance = ACT_getDoEnhance();    
    SkColorType config;
#endif	/*ANDROID_DEFAULT_CODE*/

    // pass a temporary bitmap, so that if we return false, we are assured of
    // leaving the caller's bitmap untouched.
    SkBitmap tmp;
    const Result result = this->onDecode(stream, &tmp, mode);
    if (kFailure != result) {
        bm->swap(tmp);
    }
#ifndef ANDROID_DEFAULT_CODE  	
    config = tmp.colorType();
    
    if((tmp.width() *tmp.height() <= SOFTDEC_MAX_SIZE)&&(native_standard == 1))
    {
        native_standard =  1;
    }
    else
    {
        native_standard =  0;
    }
    
    if(img_enhance == 1 && sdram_cap >= SDRAM_SUPPORT && native_standard == 0 \
        && mode == SkImageDecoder::kDecodePixels_Mode   \
        && (config == kRGB_565_SkColorType || config == kN32_SkColorType))
    {
       SkAutoLockPixels alp(tmp);       
       enhance_src = (unsigned char *)tmp.getPixels();
       width = tmp.width();
       height = tmp.height();
       //SkDebugf("image_enhance  at %d...   %d    %d",__LINE__,width,height);  
       if(image_enhance_func(width, height, config, enhance_src))
       {
            SkDebugf("image_enhance failed at %d...",__LINE__);     
       } 
    }
#endif	/*ANDROID_DEFAULT_CODE*/
    return result;
}

bool SkImageDecoder::decodeSubset(SkBitmap* bm, const SkIRect& rect, SkColorType pref) {
    // we reset this to false before calling onDecodeSubset
    fShouldCancelDecode = false;
    // assign this, for use by getPrefColorType(), in case fUsePrefTable is false
    fDefaultPref = pref;

    return this->onDecodeSubset(bm, rect);
}

bool SkImageDecoder::buildTileIndex(SkStreamRewindable* stream, int *width, int *height) {
    // we reset this to false before calling onBuildTileIndex
    fShouldCancelDecode = false;
    return this->onBuildTileIndex(stream, width, height);
}

bool SkImageDecoder::cropBitmap(SkBitmap *dst, SkBitmap *src, int sampleSize,
                                int dstX, int dstY, int width, int height,
                                int srcX, int srcY) {
    int w = width / sampleSize;
    int h = height / sampleSize;
    if (src->colorType() == kIndex_8_SkColorType) {
        // kIndex8 does not allow drawing via an SkCanvas, as is done below.
        // Instead, use extractSubset. Note that this shares the SkPixelRef and
        // SkColorTable.
        // FIXME: Since src is discarded in practice, this holds on to more
        // pixels than is strictly necessary. Switch to a copy if memory
        // savings are more important than speed here. This also means
        // that the pixels in dst can not be reused (though there is no
        // allocation, which was already done on src).
        int x = (dstX - srcX) / sampleSize;
        int y = (dstY - srcY) / sampleSize;
        SkIRect subset = SkIRect::MakeXYWH(x, y, w, h);
        return src->extractSubset(dst, subset);
    }
    // if the destination has no pixels then we must allocate them.
    if (dst->isNull()) {
        dst->setInfo(src->info().makeWH(w, h));

        if (!this->allocPixelRef(dst, NULL)) {
            SkDEBUGF(("failed to allocate pixels needed to crop the bitmap"));
            return false;
        }
    }
    // check to see if the destination is large enough to decode the desired
    // region. If this assert fails we will just draw as much of the source
    // into the destination that we can.
    if (dst->width() < w || dst->height() < h) {
        SkDEBUGF(("SkImageDecoder::cropBitmap does not have a large enough bitmap.\n"));
    }

    // Set the Src_Mode for the paint to prevent transparency issue in the
    // dest in the event that the dest was being re-used.
    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kSrc_Mode);

    SkCanvas canvas(*dst);
    canvas.drawSprite(*src, (srcX - dstX) / sampleSize,
                            (srcY - dstY) / sampleSize,
                            &paint);
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool SkImageDecoder::DecodeFile(const char file[], SkBitmap* bm, SkColorType pref,  Mode mode,
                                Format* format) {
    SkASSERT(file);
    SkASSERT(bm);

    SkAutoTUnref<SkStreamRewindable> stream(SkStream::NewFromFile(file));
    if (stream.get()) {
        if (SkImageDecoder::DecodeStream(stream, bm, pref, mode, format)) {
            bm->pixelRef()->setURI(file);
            return true;
        }
    }
    return false;
}

bool SkImageDecoder::DecodeMemory(const void* buffer, size_t size, SkBitmap* bm, SkColorType pref,
                                  Mode mode, Format* format) {
    if (0 == size) {
        return false;
    }
    SkASSERT(buffer);

    SkMemoryStream  stream(buffer, size);
    return SkImageDecoder::DecodeStream(&stream, bm, pref, mode, format);
}

bool SkImageDecoder::DecodeStream(SkStreamRewindable* stream, SkBitmap* bm, SkColorType pref,
                                  Mode mode, Format* format) {
    SkASSERT(stream);
    SkASSERT(bm);

    bool success = false;
    SkImageDecoder* codec = SkImageDecoder::Factory(stream);

    if (NULL != codec) {
        success = codec->decode(stream, bm, pref, mode);
        if (success && format) {
            *format = codec->getFormat();
            if (kUnknown_Format == *format) {
                if (stream->rewind()) {
                    *format = GetStreamFormat(stream);
                }
            }
        }
        delete codec;
    }
    return success;
}
