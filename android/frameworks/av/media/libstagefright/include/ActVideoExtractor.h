/*
 * Nothing here 2...
 */

#ifndef ACTVIDEO_EXTRACTOR_H_

#define ACTVIDEO_EXTRACTOR_H_
#include <media/MediaPlayerInterface.h>
#include <media/stagefright/MediaExtractor.h>
#include <utils/Vector.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <media/stagefright/MediaBuffer.h>
#include <actal_posix_dev.h>
#include <ALdec_plugin.h>
#define TURN_ON_ADD_VIDEO_DECODE_FLAG		1 // Mainly include vp6/vp8/rvg2
#define TURN_ON_STREAM_BUFFER_MANAGE_FLAG	0 // can print stream_buffer manage log
namespace android {

//struct AMessage;
//class String8;

//struct ActVideoSource;
//
//typedef packet_header_t _key;
//
//List<short> m_strfifo; /* stream fifo */

enum EVENT_T {
	EXTRACTOR_SUBMIT_SUB_BUF = 0,
};

typedef struct {
	size_t a;
	size_t v;
	size_t s;
} track_index_t;

typedef struct {
    size_t a;
    size_t v;
    size_t s;
} packet_max_size_t;

typedef struct {
	uint8_t* a;
	uint8_t* b;
} region_t;

typedef struct {
	uint8_t* start;
	uint8_t* start_phyaddr;
  size_t size;
	region_t empty;
} stream_buf_t;

typedef enum {
	CODEC_CODA,
	CODEC_ACTIONS,
	CODEC_SOFTWARE,
	CODEC_UNKNOWN
} CODEC_TYPE;

typedef struct ExtToMime {
	const char *ext;
	const char *mime;
	CODEC_TYPE ct;
} ExtToMime_t;

typedef struct aExtToMime{
	const char * ext;
	const char * mime;
	uint32_t len;
}aExtToMime_t;


typedef struct av_buf_ext_t{
	av_buf_t av_buf;
	uint32_t align_len;
}av_buf_ext_t;

#define MMM_SUBTITLE_NUM 30
#define MMM_SUBTITLE_DATA_LEN 15360

struct ActVideoExtractor : public MediaExtractor {
	ActVideoExtractor(const sp<DataSource> &source, const char* mime, void* cookie = NULL);

	virtual size_t countTracks();

	virtual sp<MediaSource> getTrack(size_t index);

	virtual sp<MetaData> getTrackMetaData(
	        size_t index, uint32_t flags);

	virtual sp<MetaData> getMetaData();
	virtual status_t setAudioTrack(int index, int64_t cur_playing_time);
	virtual status_t unSelectTrack(void);
	virtual uint8_t* SelectTrack(int index, int64_t cur_playing_time);

	#ifdef MMM_ENABLE_SUBTITLE  
	   virtual int64_t getsubtitle();
	   virtual int putsubtitle();
		virtual int waitsubtitle();
		virtual int dropsubtitle();
	#endif

    virtual uint32_t flags() const;
	
	bool    mthreadrun;
	bool    mactseek;
	bool    mthreadpause;
	Mutex   mActExtraLock;
	bool    isloopstart();
	bool    isseekstatus();
	void    loopstart();
	void    loopstop();
	void    loopstopseek();
	void    loopstopsettrack();
	void    loopresume();
	void    init_act_stream();
	status_t init_settrack_pram();
	virtual void stopextractor();	
	virtual uint32_t getreadsize();
	virtual void setListener(void* Listerner);
	//status_t end_stop_thread();
	status_t shutdown();
	status_t shutdown_l();
protected:
    virtual ~ActVideoExtractor();
///////////////////////////////////////////// 
//add by bruceding 2013-08
			Mutex mActDemuxerLock;  
			//Mutex mProcessthreadLock;
			Mutex init_lock_;
			//Condition mProcessPause;
			//Mutex mVbufferLock;
			Condition mAsyncVbuffer;
			
			//Mutex mVpacketLock;
			
			//Mutex mAbufferLock;
			Condition mAsyncAbuffer;
			
			//Mutex mApacketLock;
    // protected by the thread_lock_
#if 0    
    		typedef List<MediaBuffer*> MBQueue;
    		MBQueue in_queue_;

    		DISALLOW_EVIL_CONSTRUCTORS(AAH_DecoderPump);	
#endif	

    		List<av_buf_ext_t> mtimetextQueue;



    		bool mIsTimeTextSelected;
    		void flush_timedtext_queue(void);

			//List<av_buf_t> mvideoQueue;
			List<av_buf_ext_t> mvideoQueue;
			//Mutex mvqueLock;
			Condition mVBufferFilled;
            Condition mSeekCompleted;
		
			//List<av_buf_t> maudioQueue;
			List<av_buf_ext_t> maudioQueue;
			//Mutex maqueLock;
			Condition mABufferFilled;
			
			//av_buf_t vpacktbuf;
			av_buf_t vpacktarray[5];
			uint32_t vpacktarraynums;
			
			//av_buf_t apacktbuf;
			av_buf_t apacktarray[5];
			uint32_t apacktarraynums;
			
			uint8_t *vposstart;
			uint8_t *vposend, *vptempend;
			uint8_t *videopool, *videopoolend;
			
			uint8_t *aposstart;
			uint8_t *aposend, *aptempend;
			uint8_t *audiopool, *audiopoolend;	
			
//from AAH_DecoderPump
			class ThreadWrapper : public Thread {
			  public:
			  	ThreadWrapper(ActVideoExtractor* owner);
				friend class ActVideoExtractor;				

			  private:
				virtual bool threadLoop();
				ActVideoExtractor* owner_;
				~ThreadWrapper();
				//DISALLOW_EVIL_CONSTRUCTORS(ThreadWrapper);
			};
			void* workThread();
			status_t init_thread_pram();

	void audiopusheos();
	void timeoutpusheos();

	//Mutex               mthreadLock;    // Thread::mLock is private
	//Condition           mthreadCond;    // Thread::mThreadExitedCondition is private
	bool 			    thread_init;
	
	friend struct ActVideoSource;

	struct TrackInfo {
	    unsigned long mTrackNum;
	    sp<MetaData> mMeta;
	};
	Vector<TrackInfo> mTracks;

	//sp<ActpacketThread>    mActpacketThread;

	sp<ThreadWrapper>    thread_;
    Condition           thread_cond_;
    Mutex               thread_lock_;
    status_t            thread_status_;

	
	ActVideoExtractor(const ActVideoExtractor &);
	ActVideoExtractor &operator=(const ActVideoExtractor &);

	/* demuxer handle, demuxer function */
	void* m_dh;
	demux_plugin_t *m_dp;
	media_info_t m_mi;
	void* mLib_handle;

	/* main functions */
	bool test();
	bool input_mi_init();
	bool open_plugin(const char *ext);
	void addTracks();

    status_t readPacket(av_buf_t* av_buf, packet_type_t t);
    bool seekable();
    bool seekcompleted(packet_type_t t);
	status_t seek(int64_t seekTimeUs);
    uint32_t getFirstAudioTime();
	packet_type_t matchPacketType(size_t track_idx);
	bool isTextTrack(size_t track_idx);

	/* stream input */
	stream_input_t *m_input;
	char m_ext[24];
	bool m_exist_location;
	uint32_t mSeekMask;
	uint32_t mMask;


	/* now playing track index */
	track_index_t m_ti_playing;

    /* for seek */
    time_stuct_t m_ts;
    /* Normally, demuxer's packet size should NOT excced this size */
    packet_max_size_t m_pkt_maxs;
	/* stream buffer assosiated */
	stream_buf_t m_ap;
	stream_buf_t m_vp;
	bool mIsSetTrack;
	//stream_buf_t m_sp;g
	bool stream_buf_destroy();
  void stream_buf_reset();
  bool stream_buf_init();
  void stream_buf_status_dump(stream_buf_t* t);
  mutable Mutex mLock;
  Mutex mMiscStateLock;

	/* stream fifo assosiated */
	List<av_buf_t> mQueue;


	av_buf_t m_ao_buf;
	av_buf_t m_vo_buf;

 

	/* notify event to Owner */
	void* m_cookie;


	/* demuxer plugin status */
	plugin_err_no_t m_pstatus;
	bool m_fstatus;


	uint32_t mExtractorFlags;
	const ExtToMime_t *mEtm;
	const aExtToMime_t *mAEtm;
	/* when video seek failed, set this flag == 1, and audio seek won't work. */
	int mSeekFailed;
	int mAlwaysDropAudio;


	#ifdef MMM_ENABLE_SUBTITLE  
	unsigned char subtitle_data[MMM_SUBTITLE_NUM][MMM_SUBTITLE_DATA_LEN];	
	int subtitle_buf_No_0;
	int subtitle_buf_No_1;
	int subtitle_get_flag;
	int pkt_prv_ts;
	packet_header_t *packet_header_tmp;
	packet_header_t *packet_header_org;
	int addr_oft;	
	int reserved_count;
	int put_subtitle_packet_flag;
	int need_subtitle;
	#endif
	int64_t mADuration;
	int64_t mVDuration;
	int64_t mVLastPktTime;
	int64_t mALastPktTime;

	bool mNotPlayVideo;
	bool mNotPlayAudio;
	uint32_t mUnsupportAudioTrackNum;
	bool mUsingPhyContBuffer;
////////////////////////////////
    bool       mActive;                // protected by mLock
    uint32_t   streamcounterfornet;
	bool       mhttpflag; 
	uint8_t *  mSubInitBuf;
};

}  // namespace android

#endif  // ACTVIDEO_EXTRACTOR_H_
