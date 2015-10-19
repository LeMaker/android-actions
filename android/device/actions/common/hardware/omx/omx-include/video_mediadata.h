#ifndef __VIDEO_MEDIADATA_H__
#define __VIDEO_MEDIADATA_H__

#define kMetadataBufferTypeCameraSource_act 0x0//same as camerasource from android
#define kMetadataBufferTypeImageBackground_act 0x100

typedef struct vce_private{
	int noffset;
	int nfilledlen;
}vce_private;

typedef struct {
    int metadataBufferType;
    void* handle;
    int off_x;
    int off_y;
    int crop_w;
    int crop_h;
    int nAlpha;
	vce_private vce_attribute;
}video_metadata_t;


typedef struct{
	unsigned int nativehanle[3];
	int ion_share_fd;//ION share Fd for MMAP VirAddr
	/*
		ptrVir = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, ion_share_fd, 0);
	*/
	int     magic;
	int     flags;
	int     size;
	int     offset;

	unsigned int revoffset[7];
	int phys_addr;
	int ion_handle_t;//unuse here
}video_handle_t;
#endif

