#include "case.h"

bool wait_dev_plugin(char * nod_path)
{
	struct stat st;
	while (-1 == stat(nod_path, &st)) //获取文件信息
	{
		// printf("wait %s plugin\n", nod_path);
		direct_thread_sleep( 1000000 );
	}
	return true;
}

bool is_nod_exists(char * nod_path)
{
	struct stat st;
	if(-1 == stat(nod_path, &st)) //获取文件信息
	{
		return false;
	}
	return true;
}

bool is_file_empty(char *filename)
{
	FILE *fp;
	fp = fopen(filename, "r");
	if(!fp)
	{
		printf("can't open %s\n", filename);
		return true;
	}
	
	if(feof(fp))
	{
		return true;
	}
	else
	{
		return false;
	}
}

//read integer from file, return -1 if failed.
int cat_file(char *filename)
{
	FILE *fp;
	int ret;
	
	fp = fopen(filename, "r");
	if(!fp)
	{
		printf("can't open %s\n", filename);
		return -1;
	}
	if(fscanf(fp, "%d", &ret))
	{
		fclose(fp);
		return ret;
	}
	fclose(fp);
	return -1;
}

//read string from file, return 0 if failed.
int cat_file_s(char *filename, char *result)
{
	FILE *fp;
	
	//clear result, avoid bug when file is empty
	result[0] = '\0';

	fp = fopen(filename, "r");
	if(!fp)
	{
		printf("can't open %s\n", filename);
		return 0;
	}

	if(fscanf(fp, "%s", result))
	{
		fclose(fp);
		return strlen(result);
	}
	fclose(fp);
	return 0;
}

int parse_dev_name(char *config, char dev[3][60])
{
	int i, count = 0;
	char dev_name[100];   
	strcpy(dev_name, config);  //make a copy of config devname
	char *pos = dev_name;
	char *token = NULL;

	printf("userconfig:devname=%s\n",dev_name);
	if (dev_name == NULL || dev_name[0] == '\0')
	{
		return -1;
	}
	for (i = 0; i < 3; i++)
	{
		token = strchr(pos, ',');
		if (token == NULL)
		{
			printf("config dev[%d]=%s\n", count, pos);
			strncpy(dev[count], pos, 60);
			count++;
			break;
		}
		else
		{
			*token = '\0';
			printf("config dev[%d]=%s\n",count, pos);
			strncpy(dev[count], pos, 60);
			count++;
			pos = token + 1; 
		}
	}
	return count;
}

//get input device index from device name
int get_input_event_name(char *dev_name, char *path)
{
	int i = 0, j = 0;
	int fd = -1;
	int ret = 0;
	char name_path[100];
	char buf[100];
	char dev[3][60];
	
	memset(dev, 0, 3*60);
	ret = parse_dev_name(dev_name, dev);
	if (ret < 0)
	{
		printf("tp dev name config error!\n");
		return -1;
	}
	for(i = 0; i < 5; i++)
	{
		sprintf(name_path, "/sys/class/input/event%d/device/name", i);
		// printf("name_path = %s\n", name_path);
		fd = open(name_path, O_RDONLY);
		if(fd < 0)
			continue;
		if ((ret = read(fd, buf, sizeof(buf))) < 0) {
		    close(fd);
		    continue;
		}
		
		buf[ret - 1] = '\0';
		// printf("buf = %s\n", buf);
		for (j = 0; j < 3; j++)
		{
			if (!strcmp(buf, dev[j])) 
			{
			    sprintf(path, "/dev/input/event%d", i);
				close(fd);
				// printf("dev %s event path is %s\n", dev_name, path);
				return i;
			}
		}
		close(fd);
	}
	printf("can't find dev %s\n", dev_name);
	return -1;
}

void draw_result(case_t *caseP, bool passflg)
{
	int width, height;
	IDirectFBFont *font;
	int font_height, ret_width, ret_num;
	const char *tmp_line, *next_line;
	int line_height = 0;
	int string_num;
	
	DFBCHECK(caseP->surface->GetSize(caseP->surface, &width, &height));
	DFBCHECK(caseP->surface->SetColor(caseP->surface, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(caseP->surface->FillRectangle(caseP->surface, 0, 0, width, height));

	if(passflg)
	{
		DFBCHECK(caseP->surface->SetColor(caseP->surface, 0, 0, 0xff, 0xff));
		DFBCHECK(caseP->surface->GetFont(caseP->surface, &font));
		DFBCHECK(font->GetHeight(font, &font_height));

		tmp_line = caseP->pass_string;
		do
		{
			string_num = -1;
			font->GetStringBreak(font, tmp_line, -1, width, &ret_width, &ret_num, &next_line);
			if(next_line != NULL)
			{
				string_num = next_line - tmp_line;
			}
			DFBCHECK(caseP->surface->DrawString(caseP->surface, tmp_line, string_num, 0, line_height, DSTF_TOPLEFT));
			
			tmp_line = next_line;
			line_height += font_height;
		}while(next_line != NULL);
	}
	else
	{
		DFBCHECK(caseP->surface->SetColor(caseP->surface, 0xff, 0, 0, 0xff));	
		DFBCHECK(caseP->surface->DrawString(caseP->surface, caseP->fail_string, -1, 0, 0, DSTF_TOPLEFT));
	}
	DFBCHECK(caseP->surface->Flip(caseP->surface, NULL, 0));
}

void wait_nod_state(case_t *caseP, bool *stateflg)
{
	int ret;
	bool test_done = false;
	while(true)
	{
		ret = cat_file(caseP->nod_path);
		if(ret == -1)
			break;
		if(ret)
		{
			*stateflg = true;
			if(!test_done)
			{
				draw_result(caseP, true);
				test_done = true;
			}
		}
		else
		{
			*stateflg = false;
		}
		direct_thread_sleep( 1000000 );
	}
}

bool pcba_system(char *cmd)
{
	int ret = 0;
	// printf("cmd = %s\n", cmd);
	ret = system(cmd);
	if(ret == -1)
	{
		printf("run system error, %s\n", cmd);
	}
	else
	{
		if(WIFEXITED(ret))
		{
			printf("shell exitstatus = %d,%s\n", WEXITSTATUS(ret), cmd);
			return true;
		}
		else
		{
			printf("run cmd failed %s ret = %d, %d %d\n", cmd, ret, WIFEXITED(ret), WEXITSTATUS(ret));
		}
	}
	
	return false;
}