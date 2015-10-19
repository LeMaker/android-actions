#ifndef TRACE_READAHEAD_H
#define TRACE_READAHEAD_H

#include <stdio.h>
#include <cutils/klog.h>

#define _DEBUG

#ifdef _DEBUG
#define DEBUG(format,...) printf("FILE: , LINE: %d: "format"\n", __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format,...)
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define MAX(a,b) ((a>b)?(a):(b))
#define MIN(a,b) ((a>b)?(b):(a))





#define INFO(x...)    KLOG_INFO("treadahead", x)
#define ERROR(x...)   KLOG_ERROR("treadahead", x)

/*==================path========================*/
#define PATH_DEBUGFS    "/sys/kernel/debug"

#define TRACE_LOG   "/data/trace_log"

#define DEALED_TRACE_LOG  "/data/dealed_trace_log"

#define READAHEAD_BLOCKS  "/data/readahead_blocks.bak"

#define FINAL_READAHEAD_BLOCKS "/data/final_readahead_file_blocks"

#define SYSTEM_PARTITION "/system"
#define DATA_PARTITION  "/data"

/*====================trace realated===========================*/

#define DO_FILE_MAP_TAG "do_file_map:"
#define DO_FS_READ_TAG "do_fs_read:"
#define DO_OPEN_EXEC_TAG "do_open_exec:"


#define DO_FILE_MAP_ENABLE "events/readahead/do_file_map/enable"
#define DO_FS_READ_ENABLE "events/readahead/do_fs_read/enable"
#define DO_OPEN_EXEC_ENABLE "events/readahead/do_open_exec/enable"

#define TRACE_BUFFER "buffer_size_kb"
#define TRACE_CPUMASK "tracing_cpumask"

#define TRACE_BUFFER_FREE "/sys/kernel/debug/tracing/free_buffer"
#define SYS_COMLETED_PROPERTY_TAG  "sys.boot_completed"

#define WRITE_BLOCKS_COMPLETED "blocks_info_completed"

#define TRACE_NEED_BUFFER_SIZE 10000   //10m

#define TRACE_SLEEP_MAX_TIME 300   /* time out 300s */

#define SLEEP_ONE_TIME 3 /* wait for 3s at a time */

#define TREACE_PROCESS_NICE 10
/*=====================count=====================*/
#define  TRACE_CLASSSIZE  3

#define MAX_FILENAME_LENGTH 512

#define BLOCK_LOG_LINESIZE 50
#define BLOCK_LOG_ROWSIZE 6

#define MAX_BLOCK_INFOSIZE 4096

/*====================db==cmd============================*/

#define PROVISIONED_DB_NAME "/data/data/com.android.providers.settings/databases/settings.db"

#define QUERY_DEVICE_PROVISIONED_CMD "select * from global where name='device_provisioned'"

/*[k1:readaheadindex] [k2:major] [k3:minor] [k4:inode] [k5:ofset] [k6:size]*/
#define SORT_BLOCKS_FILE_CMD "busybox sort -n -k3 -k4 -k5 %s | busybox uniq -f 1 > %s"

#define GET_PARTITION_FILE_NAME_CMD "busybox find /system /data -type f"

#define GET_PARTITION_FILE_COUNT_CMD "busybox find /system /data -type f | busybox wc -l"

#define SORT_READHEAD_BLOCK_FILE "busybox sort -n %s > %s;busybox sed -i 's/^[0-9]* *//' %s"


/*=======================struct=========================*/

typedef struct {

	char acfilename[MAX_FILENAME_LENGTH];

	unsigned int uimajor;
	unsigned int uiminor;

	unsigned long ulinod;
	unsigned long ulsize;
} STFILE_STAT;

typedef struct {

	unsigned long ulstart;
	unsigned long ulend;

} STFILE_BLOCKINFO;

typedef struct {

	char acfilename[MAX_FILENAME_LENGTH];

	unsigned long ulreadheadindex;
	int iblockinfosize;

	STFILE_BLOCKINFO acfileblocks[MAX_BLOCK_INFOSIZE];

} STREADHEAD_FILE;


int trace(char* psystemdev,char* pdatadev);
extern void process_files(char* filename);
#endif
