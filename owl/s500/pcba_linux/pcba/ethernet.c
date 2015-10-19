/*
*************author: xiehaocheng
*************date: 2015-4-21
*************version: v1.0
*/

#include"case.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <time.h>

typedef struct ethernet {
	char interface[20];
	char ip[20];
	char netmask[20];
	char ping[20];
}ethernet_t;
	
//static struct ethernet eth;

int parse_to_array(char *buf, char argv[3][20])
{
	int count = 0;
	char *head = buf;
	char *pos = buf;
	printf("net config buf = [%s]\n",buf);
	if (buf == NULL || buf[0] == '\0')
	{
		return -1;
	}
	while(*pos)
	{
		if(*pos == ':')
		{
			*pos = '\0';
			strncpy(argv[count], head, 20);
			count++;
			if(count == 3)
			{
				break;
			}
			head = pos + 1;
		}
		pos++;
	}
	return count;
}



int parse_ethernet_config(case_t *eth_case, struct ethernet *eth)
{
	int argc, i;
	char argv[3][20];
	char *index = eth_case->nod_path;
	
	strncpy(eth->interface, eth_case->dev_name, 20);
	argc = parse_to_array(eth_case->nod_path, argv);
	if (argc < 0)
	{
		return -1;
	}
	for (i = 0; i < argc; i++)
	{
		char flag = argv[i][0];
		switch (flag)
		{
			case 'i':
				strncpy(eth->ip, &argv[i][1], 20);
				break;
			case 'm':
				strncpy(eth->netmask, &argv[i][1], 20);
				break;
			case 'p':
				strncpy(eth->ping, &argv[i][1], 20);
				break;
			default:
				break;
		}
	}
	if (strcmp(eth->ping, "") == 0)
	{
		strcpy(eth->ping, eth->ip);
	}
	return 0;
}



bool test_ethernet(case_t *eth_case)
{
	int ret, pid, i;
	struct ethernet *eth;
	char command[100];
	
	eth	= malloc(sizeof(struct ethernet));
	memset(eth, 0, sizeof(struct ethernet));
	
	ret = parse_ethernet_config(eth_case, eth);
	if (ret < 0)
	{
		printf("Network test config error!\n");
		return false;
	}
	printf(" interface=%s\n ip=%s\n netmask=%s\n ping=%s\n",
			eth->interface, eth->ip, eth->netmask, eth->ping);
			
	//lo enable, for ping test
	sprintf(command, "ifconfig lo up");
	ret = system(command);
	if (ret != 0)
	{
		printf("[%s] error! Network test failed\n", command);
		return false;
	}
	printf("[%s] success!---ethernet---\n", command);
	
	//interface enable
	sprintf(command, "ifconfig %s up", eth->interface);
	ret = system(command);
	if (ret != 0)
	{
		printf("[%s] error! Network test failed\n", command);
		return false;
	}
	printf("[%s] success!---ethernet---\n", command);
	
	//interface config
	sprintf(command, "ifconfig %s %s netmask %s", eth->interface, eth->ip, eth->netmask);
	ret = system(command);
	if (ret != 0)
	{
		printf("[%s] error! Network test failed\n", command);
		return false;
	}
	printf("[%s] success!---ethernet---\n", command);
	
	//ping ip test
	sprintf(command, "ping %s -c 1", eth->ping);
	for (i = 0; i < 5; i++)
	{
		ret = system(command);
		if (ret != 0)
		{
			printf("[%s] error! Network test failed\n", command);
			return false;
		}
	}
	printf("[%s] success!---ethernet---\n", command);
	free(eth);
	return true;
}


