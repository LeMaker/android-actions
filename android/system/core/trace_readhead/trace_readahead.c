
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>

#include <linux/fiemap.h>

#include <sys/stat.h>
#include<fcntl.h>

#include <linux/fs.h>
#include <linux/kdev_t.h>

#include <malloc.h>
#include <string.h>

#include <cutils/properties.h>

#include "trace_readahead.h"
#include "values.h"
#include "sqlite3.h"


static int get_file_stat(STFILE_STAT *pstfilestat) {
	int ifd;
	struct stat statbuf;

	ifd = open(pstfilestat->acfilename, O_RDONLY);

	if (ifd < 0) {

		return -1;

	}

	if (fstat(ifd, &statbuf) < 0) {
		ERROR("fstat error\n");
		return -1;
	}

	pstfilestat->uimajor=MAJOR(statbuf.st_dev);
	pstfilestat->uiminor=MINOR(statbuf.st_dev);
	pstfilestat->ulinod=statbuf.st_ino;
	pstfilestat->ulsize=statbuf.st_size;

	close(ifd);
	return 1;
}

static int getfilecount() {

	FILE *fstream = NULL;
	char ucbuff[MAX_FILENAME_LENGTH];
	int ifilecount = -1;
	if (NULL== (fstream = popen(GET_PARTITION_FILE_COUNT_CMD, "r"))) {
		ERROR("getfilecount popen error\n");
		return -1;
	}

	fgets(ucbuff, sizeof(ucbuff), fstream);

	ifilecount = atoi(ucbuff);

	pclose(fstream);
	return ifilecount;
}





static int getallfilestat(STFILE_STAT *pstfilestat)
{

	FILE *pfstream = NULL;
	char ucbuff[MAX_FILENAME_LENGTH];
	int ifileindex=-1;

	if (NULL == (pfstream = popen(GET_PARTITION_FILE_NAME_CMD, "r"))) {
		ERROR(" popen error\n");
		return -1;
	}

	while (fgets(ucbuff, sizeof(ucbuff), pfstream)) {

		memcpy(pstfilestat[ifileindex].acfilename,ucbuff,strlen(ucbuff)-1);


		if(get_file_stat(&pstfilestat[ifileindex])<0)
		{
			//ERROR("error get %s stat\n",ucbuff);
		}

		ifileindex++;


	}

	pclose(pfstream);

	return ifileindex;
}

static void gettraceblockinfo(char* pbuffer,unsigned long* paltracebuffer)
{
	char *space=" ";
	char *result=NULL;



	paltracebuffer[0]=atoi(strtok(pbuffer,space));


	for(int iloop=1;iloop<BLOCK_LOG_ROWSIZE;iloop++)
	{
		 result = strtok( NULL, space );
		 if(result == NULL)
		 {
			 break;
		 }

		paltracebuffer[iloop]=atoi(result);


	 }




}



static void addblockinfo(STREADHEAD_FILE* pstreadhead,unsigned long uloffset,unsigned long ulsize)
{
	int iblockinfoindex=0;
	unsigned long ulpreblockend=0;
	STFILE_BLOCKINFO stfileblockinfo;
	memset(&stfileblockinfo,0,sizeof(STFILE_BLOCKINFO));
	stfileblockinfo.ulstart = uloffset;
	stfileblockinfo.ulend = uloffset + ulsize;



		if (pstreadhead->iblockinfosize > 0) {
			 iblockinfoindex = pstreadhead->iblockinfosize - 1;
			 ulpreblockend = pstreadhead->acfileblocks[iblockinfoindex].ulend;


		if (ulpreblockend >= stfileblockinfo.ulstart) {
			pstreadhead->acfileblocks[iblockinfoindex].ulend =MAX(ulpreblockend,stfileblockinfo.ulend);

			} else {

				iblockinfoindex = pstreadhead->iblockinfosize++;
				if(iblockinfoindex < MAX_BLOCK_INFOSIZE)
				{
					pstreadhead->acfileblocks[iblockinfoindex].ulstart =stfileblockinfo.ulstart;
					pstreadhead->acfileblocks[iblockinfoindex].ulend = stfileblockinfo.ulend;
				}

			}
		}else
		{

			pstreadhead->acfileblocks[pstreadhead->iblockinfosize].ulstart = stfileblockinfo.ulstart;
			pstreadhead->acfileblocks[pstreadhead->iblockinfosize].ulend = stfileblockinfo.ulend;
			pstreadhead->iblockinfosize++;
		}

}


static void fprintf_block(FILE *pfreadblock,STREADHEAD_FILE streadheadfile)
 {


	assert (pfreadblock != NULL);

	fprintf(pfreadblock, "%lu %s ", streadheadfile.ulreadheadindex,streadheadfile.acfilename);
	if (streadheadfile.iblockinfosize > 0) {

		for (int iloop = 0; iloop < streadheadfile.iblockinfosize; iloop++)

		{

			fprintf(pfreadblock, "%lu %lu ",
					streadheadfile.acfileblocks[iloop].ulstart,
					streadheadfile.acfileblocks[iloop].ulend);
		}

	}
	fprintf(pfreadblock, "\n");
}

static int syncblockinfofile() {
	int ifd = 0;
	ifd = open(READAHEAD_BLOCKS, O_RDONLY);
	if (ifd < 0) {
		ERROR("open block file error\n");
		return -1;
	}
	if (fsync(ifd) < 0) {
		ERROR("fsync block file error\n");
		close(ifd);
		return -1;
	}
	close(ifd);
	return 1;

}


static int removelogfile() {
	if (remove(TRACE_LOG) < 0) {
		return -1;
	}

	if (remove(DEALED_TRACE_LOG) < 0) {
		return -1;
	}

	if (remove(READAHEAD_BLOCKS) < 0) {
		return -1;
	}

	return 1;

}

static int matchblockfile(STFILE_STAT *pstfilestat,int ifilestatsize)
{

	assert (pstfilestat != NULL);

	char *piter = NULL, *pfile = NULL;
	int ifd = open(DEALED_TRACE_LOG, O_RDONLY);
	char ucbuffer[BLOCK_LOG_LINESIZE]={0};
	unsigned long  ulfilesize = 0;
	struct stat fileState;
	fstat(ifd, &fileState);
	ulfilesize = fileState.st_size;
	int imatchedfilesize=0;
	int ioptionfilesize=0;
	int ijudgesize=0;
	STFILE_STAT stcumatchfilestat;

	STREADHEAD_FILE streadheadfile;
	memset(&stcumatchfilestat,0,sizeof(STFILE_STAT));
	memset(&streadheadfile,0,sizeof(STREADHEAD_FILE));

	unsigned long ualtracebuffer[6]={0};
	int ilooper=0;

	FILE *pfreadblock=NULL;


	pfile = (char *) mmap(NULL, ulfilesize, PROT_READ, MAP_SHARED, ifd, 0);
	if(pfile == MAP_FAILED)
	{
		goto error;
	}
	piter = pfile;


	if((pfreadblock=fopen(READAHEAD_BLOCKS,"wt+")) == NULL)
	{
		ERROR("jsces open /data/readheadblockfile  error\n"  );
		goto error;
	}

	while (piter) {
		/* find next newline */
		char* pnext = memchr(piter, '\n', BLOCK_LOG_LINESIZE);
		if (pnext) {
			if (pnext - piter > 1) {
				memcpy(ucbuffer, piter, pnext - piter);
				int ihit=0;
				// replace newline with string terminator
				ucbuffer[pnext - piter] = '\0';

				/*ualtracebuffer[?]: 0:readaheadindex/ 1:major /2:minor /3:inode /4:offset /5:size */
				gettraceblockinfo(ucbuffer,ualtracebuffer);

				if((stcumatchfilestat.uiminor==ualtracebuffer[2])&&(stcumatchfilestat.ulinod==ualtracebuffer[3]))
				{
					ihit=1;
				}
				else {
					for (ilooper = 0; ilooper < ifilestatsize; ilooper++) {
						if ((ualtracebuffer[2] == pstfilestat[ilooper].uiminor)
								&& (ualtracebuffer[3]== pstfilestat[ilooper].ulinod)) {

							//INFO("math file name ==%s\n",pstfilestat[ilooper].acfilename);
							memcpy(&stcumatchfilestat, &pstfilestat[ilooper],sizeof(STFILE_STAT));
							imatchedfilesize++;
							ihit = 1;
							break;
						}

					}
				}

				if (ihit == 1) {

					if(imatchedfilesize>ioptionfilesize)
					{
						if(imatchedfilesize>1)
						{

						//INFO("dsp get block info : %s \n",streadheadfile.acfilename);

							fprintf_block(pfreadblock,streadheadfile);


						}

						memset(&streadheadfile,0,sizeof(streadheadfile));

						strcpy(streadheadfile.acfilename,stcumatchfilestat.acfilename);
						streadheadfile.ulreadheadindex=ualtracebuffer[0];
						ioptionfilesize=imatchedfilesize;

					}
					/*get current stfile readahead index:first number*/
					streadheadfile.ulreadheadindex=MIN(streadheadfile.ulreadheadindex,ualtracebuffer[0]);

					ijudgesize=ualtracebuffer[5];

					if(ualtracebuffer[5] > stcumatchfilestat.ulsize )
					{
						ijudgesize=stcumatchfilestat.ulsize-ualtracebuffer[4];

					}

					if(ijudgesize > 0)
					{

						addblockinfo(&streadheadfile,ualtracebuffer[4],ijudgesize);
					}


				}



			}
			piter = pnext + 1;
		} else {
			piter = NULL;
		}
	}

	/*fprintf the last one*/
	if(streadheadfile.iblockinfosize!=0)
	{
	fprintf_block(pfreadblock,streadheadfile);
	}

	fprintf(pfreadblock,"%s\n",WRITE_BLOCKS_COMPLETED);


	fclose(pfreadblock);
	munmap(pfile, ulfilesize);
	close(ifd);



	if(syncblockinfofile()<0)
	{
		ERROR("syncblockinfofile  error\n");
		goto error;
	}
	/*===============================*/
	char acfilesortcmd[1024] = { '\0' };

	/*sort depend 2(minor) 3(inode) 4(offset)*/
	sprintf(acfilesortcmd, SORT_READHEAD_BLOCK_FILE, READAHEAD_BLOCKS, FINAL_READAHEAD_BLOCKS,FINAL_READAHEAD_BLOCKS);


	if (system(acfilesortcmd) != 0) {
		ERROR("system file_cmd <%s> failed \n",acfilesortcmd);
		return -1;
	}

	if (removelogfile() < 0) {
		goto error;
	}

	INFO("trace readahead complete\n");
	return 1;


error:
	close(ifd);
	return -1;
}













static int digestblock()
{


	int ifilestatcount=-1;
	STFILE_STAT *pstfilestat=NULL;

	int ifilecount=0;

	/*===============getallfilestat===================*/


	ifilecount=getfilecount();


	if (ifilecount < 0) {
		return -1;
	}

	INFO(" popen filecount==%d \n", ifilecount);

	pstfilestat = (STFILE_STAT *) malloc(ifilecount);

	if (pstfilestat == NULL) {
		ERROR("pstfilestat malloc failed\n");
		return -1;
	}

	ifilestatcount = getallfilestat(pstfilestat);

	if (ifilestatcount < 0) {
		ERROR("getallfilestat  failed\n");
		goto error;
	}




/*===============parser===================*/





	if(matchblockfile(pstfilestat,ifilestatcount)<0)
	{
		ERROR("mathblockfile error\n");
		goto error;
	}


	free(pstfilestat);

	return 1;

error:
	free(pstfilestat);
	return -1;

}







static int read_trace(int idfd, const char *path, char* psystemdev,char* pdatadev) {
	int   ifd;
	FILE *pfile;
	char *pline;
	char *pspace=" ";


	unsigned long ullogindex=1;
	assert (path != NULL);

	char *pactraceclass[TRACE_CLASSSIZE]={DO_FILE_MAP_TAG,DO_FS_READ_TAG,DO_OPEN_EXEC_TAG};

	FILE *pfilename;
	unsigned int uilooper=0;

	ifd = openat (idfd, path, O_RDONLY);
	if (ifd < 0)
		return -1;

	pfile = fdopen (ifd, "r");
	if (! pfile) {
		close (ifd);
		return -1;
	}

	if((pfilename=fopen(TRACE_LOG,"wt+")) == NULL)
	{
		ERROR("jsces open /data/trace_log  error\n"  );
		goto error;
	}

	INFO(" begain fgets_alloc ========\n");

	while ((pline = fgets_line (NULL, pfile)) != NULL) {
		char *ptr =NULL;
		char *end;
		int ihit=0;


		for(int iloop=0;iloop<TRACE_CLASSSIZE;iloop++)
		{

			if(strstr(pline,pactraceclass[iloop])!=NULL)
			{
				ihit=1;
				break;
			}
		}

		if(ihit==0)
		{
			free(pline);
			continue;
		}

			ptr = strstr(pline, psystemdev);
		if (ptr == NULL) {
			ptr = strstr(pline, pdatadev);
		}

		if (ptr == NULL) {
			free(pline);
			continue;
		}



		fprintf(pfilename, "%lu %s\n",ullogindex,ptr);

		ullogindex++;

		free (pline);
	}




	char acfilesortcmd[1024] = {'\0'};

	/*sort depend 2(minor) 3(inode) 4(offset)*/
	sprintf(acfilesortcmd,SORT_BLOCKS_FILE_CMD,TRACE_LOG,DEALED_TRACE_LOG);


	if(system(acfilesortcmd)!=0)
	{
		ERROR("system file_cmd <%s> failed \n",acfilesortcmd);
		goto error;
	}




	close(ifd);
	fclose(pfile);
	fclose(pfilename);
	return 0;

	error:
	close(ifd);
	fclose (pfile);
	fclose(pfilename);
	return -1;

}







static int wait_for_property(const char *name, const char *desired_value, int maxwait)
{
    char value[PROPERTY_VALUE_MAX] = {'\0'};
    int maxnaps = maxwait / SLEEP_ONE_TIME;

    if (maxnaps < 1) {
        maxnaps = 1;
    }

    while (maxnaps-- > 0) {
        sleep(SLEEP_ONE_TIME);
        if (property_get(name, value, NULL)) {
            if (desired_value == NULL || strcmp(value, desired_value) == 0) {
                return 0;
            }
        }
    }
    return -1; /* failure */
}


int trace(char* psystemdev,char* pdatadev) {
	int idfd;

	int icpucount=0;
	int ifreefd;
	int iold_do_file_map_enable = 0;
	int iold_do_fs_read_enable = 0;
	int iold_do_open_exec_enable = 0;



	/*  debugfs  already mounted */
	idfd = open (PATH_DEBUGFS "/tracing", O_RDONLY | O_NOATIME);
	if (idfd < 0) {

		ERROR("error:dfd = open %s \n",PATH_DEBUGFS);
		return -1;
	}

	ifreefd = open(TRACE_BUFFER_FREE, O_RDONLY);

	if (ifreefd < 0) {
		goto error;

	}




	if (set_value(idfd, DO_FILE_MAP_ENABLE, TRUE,
			&iold_do_file_map_enable) < 0) {
		ERROR("error do_file_map/enable\n");
		goto error;
	}
	if (set_value(idfd, DO_FS_READ_ENABLE, TRUE,
			&iold_do_fs_read_enable) < 0) {
		ERROR("error do_fs_read/enable\n");
		goto error;
	}
	if (set_value(idfd, DO_OPEN_EXEC_ENABLE, TRUE,
			&iold_do_open_exec_enable) < 0) {
		ERROR("error do_open_exec/enable\n");
		goto error;
	}



		icpucount = get_value(idfd, TRACE_CPUMASK, NULL);

		if (icpucount <= 0) {
			ERROR("error tracing_cpumask\n");
			goto error;
		}


     /*set one cpu buffer_size_kb*/
	if (set_value(idfd, TRACE_BUFFER, TRACE_NEED_BUFFER_SIZE/icpucount, NULL) < 0)
		{
		ERROR("error buffer_size_kb\n");
		goto error;

		}


	INFO("sleep to tracing\n");

	if(wait_for_property(SYS_COMLETED_PROPERTY_TAG,"1",TRACE_SLEEP_MAX_TIME)<0)
	{

		ERROR("wait_for_property do not wait_for <sys.boot_completed> give up\n");
		goto error;

	}


	if (set_value(idfd, DO_FILE_MAP_ENABLE,
			iold_do_file_map_enable,NULL) < 0) {
		ERROR("error do_file_map/enable\n");
		goto error;
	}
	if (set_value(idfd, DO_FS_READ_ENABLE,
			iold_do_fs_read_enable,NULL) < 0) {
		ERROR("error do_fs_read/enable\n");
		goto error;
	}
	if (set_value(idfd, DO_OPEN_EXEC_ENABLE,
			iold_do_open_exec_enable,NULL) < 0) {
		ERROR("error do_open_exec/enable\n");
		goto error;
	}


	/* Be nicer */
	if (nice(TREACE_PROCESS_NICE)!=TREACE_PROCESS_NICE)
	{
		ERROR("error nice\n");
		goto error;
	}

	INFO("begain read_trace\n");
	/* Read trace log */
	if (read_trace(idfd, "trace",psystemdev,pdatadev) < 0)
		goto error;

	if (close (idfd)) {
		goto error;
	}

	/*release £º free_buffer*/
	close(ifreefd);
	return 0;
error:
	close (idfd);
	close(ifreefd);
	return -1;
}









static void getpartitiondevstring(char* psystemdev,char* pdatadev)
{
	STFILE_STAT stsystempatition;
	STFILE_STAT stdatapatition;
	memset(&stsystempatition,0,sizeof(STFILE_STAT));
	memset(&stdatapatition,0,sizeof(STFILE_STAT));
	strcpy(stsystempatition.acfilename,SYSTEM_PARTITION);
	strcpy(stdatapatition.acfilename,DATA_PARTITION);
	get_file_stat(&stsystempatition);
	get_file_stat(&stdatapatition);

	sprintf(psystemdev, "%d %d", stsystempatition.uimajor, stsystempatition.uiminor);
	sprintf(pdatadev, "%d %d", stdatapatition.uimajor, stdatapatition.uiminor);

	INFO("getpartitiondev system== %s ; data== %s \n",psystemdev,pdatadev);
}



static int checkblocksfile() {

	FILE *pfbloksfile = NULL;
	int ioffset = strlen(WRITE_BLOCKS_COMPLETED);
	char acbuffer[MAX_FILENAME_LENGTH] = { "\0" };

	if(access(READAHEAD_BLOCKS,0)==0)
	{
		INFO("blockinfofile.bak exist;  exception at last time\n");
		return -1;
	}

	if ((pfbloksfile = fopen(FINAL_READAHEAD_BLOCKS, "rt")) == NULL) {
		INFO("error:fopen /data/final_readahead_file_blocks \n");
		return -1;
	}


	if ((fread(acbuffer, ioffset, 1, pfbloksfile)) != 1) {
		INFO("error:fread\n");
		goto error;
	}

	if (strcmp(WRITE_BLOCKS_COMPLETED, acbuffer) != 0) {
		INFO("error:strcmp acbuffer=%s\n", acbuffer);
		goto error;
	}

	fclose(pfbloksfile);
	return 1;

	error: fclose(pfbloksfile);
	return -1;

}


/*settings.db-global table-format: ID|NAME|VALUE*/
/*CREATE TABLE global (_id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT UNIQUE ON CONFLICT REPLACE,value TEXT);*/
/*now get value*/
int loaddbinfo( void * pbuffer, int icolumn, char ** pvalue, char ** pname )
{

    for( int iloop = 0 ; iloop < icolumn; iloop++ )
    {

    	if((iloop==icolumn-1)&&(pbuffer != NULL))
          {

        	strcpy((char *)pbuffer,pvalue[iloop]);
          }
    }
    return 0;
}


static int checkdeviceprovisioned() {

	char acvalue[10] = {'\0'};
	sqlite3 * sdb = 0;

	if (sqlite3_open(PROVISIONED_DB_NAME, &sdb) != SQLITE_OK) {
		ERROR("open error! : %s\n", sqlite3_errmsg(sdb));
		goto error;
	}


	if (sqlite3_exec(sdb, QUERY_DEVICE_PROVISIONED_CMD, loaddbinfo, &acvalue,
			NULL) != SQLITE_OK) {
		ERROR("select provisioned error\n");
		goto error;
	}

	if (strcmp(acvalue, "1") != 0) {
		ERROR("device_provisioned =0 :the device is not ready\n");
		goto error;
	}

	sqlite3_close(sdb);
	sdb = 0;

	return 1;

	error: sqlite3_close(sdb);
	sdb = 0;
	return -1;

}

int main(int argc, char *argv[]) {


	char systemdev[48]={0};
	char datadev[48]={0};


	if(checkblocksfile()>0)
	{
		INFO("go to process_files\n");
		process_files(FINAL_READAHEAD_BLOCKS);

	}else
	{

		INFO("go to open trace\n");
		if(checkdeviceprovisioned()<0)
		{
			INFO("device do not provisioned:do not trace - wait next time \n");
			return -1;
		}

		getpartitiondevstring(systemdev, datadev);

		if (trace(systemdev, datadev) < 0) {
			ERROR("error:trace \n");

			return -1;
		}

		if (digestblock() < 0) {
			ERROR("error:digest \n");
			return -1;
		}

	}



	return 1;
}
