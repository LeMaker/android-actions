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
#include <termios.h>
#include <time.h>


void add_fail_string(char *buf, char *name)
{
	printf("failed uart name=%s\n",name);
	sprintf(buf, "FAIL(%s)",name + 1);
}

int parse_uart_name(char *devname, char name[3][50])
{
	int count = 0;
	char *head = devname;
	char *pos = devname;
	printf("devname=%s,strlen=%d\n",devname,strlen(devname));
	if (devname == NULL || devname[0] == '\0')
	{
		return -1;
	}
	while(1)
	{
		if(*pos == '\0')
		{
			strncpy(name[count], head, 50);
			count++;
			break;
		}
		if(*pos == ',')
		{
			*pos = '\0';
			strncpy(name[count], head, 50);
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

int uart_open(char *port)
{
	int fd;
	struct termios term;
	fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0)
	{
		perror("serial open error!");
		return -1;
	}
	
	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &term) < 0) 
	{
		perror("Can't get port settings");
		return -1;
	}
	cfsetispeed(&term, B9600);   
	cfsetospeed(&term, B9600); 
		
	term.c_cflag |= CLOCAL; 
	term.c_cflag |= CREAD;
	term.c_cflag &= ~CRTSCTS;
	
	//data bit 
	term.c_cflag &= ~CSIZE;
	term.c_cflag |= CS8;
	//stop bit
	term.c_cflag |= ~CSTOPB;
	//parity
	term.c_cflag &= ~PARENB;
	term.c_iflag &= ~INPCK;
	
	term.c_oflag &= ~OPOST;  
	term.c_lflag &= ~(ISIG | ICANON);  
   	//term.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	
    term.c_cc[VTIME] = 1; /* wait 1*(1/10)s between bytes*/    
    term.c_cc[VMIN] = 1; /* at least read 1 byte*/
	
	tcflush(fd, TCIOFLUSH);
	if (tcsetattr(fd, TCSANOW, &term) < 0) 
	{
		perror("Can't set port settings");
		return -1;
	}
	return fd;
}

int uart_recev(int fd, char *buf, int datalen)
{
	fd_set fds;
	int ret,len;
	struct timeval time;
	
	time.tv_sec = 10;
	time.tv_usec = 0;
	
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	ret = select(fd+1, &fds, NULL, NULL, &time);  //only wait for 10 second
	
	if (ret > 0)
	{
		printf("select success, read uart ready---[uart test]\n");
		len = read(fd, buf, datalen);
		printf("read len:%d buf:%s---[uart test]\n", len, buf);
		return len;
	}
	else
	{
		printf("select return %d, error:%s---[uart test]\n", ret, strerror(errno));
		return -1;
	}
}

int uart_send(int fd, char *buf, int len)
{
	int ret;
	char *line = "\n";
	ret = write(fd, buf, len);
	return ret;
}

bool test_uart(case_t *uart_case)
{
	int fd;
	int i,datalen,len;
	char buf[100];
	char path[100] = "/dev/";
	char uart_name[3][50];
	char *message = "www.actions-semi.com";
	int err_flag = 0;
	int uart_num;
	char fail_string[50];
	
	
	memset(fail_string, 0, 50);
	uart_num = parse_uart_name(uart_case->dev_name, uart_name);
	if (uart_num < 0)
	{
		printf("uart dev name config error!\n");
		err_flag = 1;
	}
	printf("uart_num = %d\n",uart_num);
	for (i = 0; i < uart_num; i++)
	{	
		strcat(path, uart_name[i]);
		printf("start test uart name %s\n",path);
		fd = uart_open(path);
		if (fd < 0)
		{
			perror("uart open error!"); 
			err_flag = 1;
			sprintf(fail_string, "%s,%s", fail_string, uart_name[i]);
			printf("add [%s] to fail string\n", uart_name[i]);
			continue;
		}
		printf("open %s success!\n",path);
		datalen = strlen(message);
		len = uart_send(fd, message, datalen);  
		if (len != datalen)  
		{  
			perror("uart write data error!"); 
			tcflush(fd,TCOFLUSH);  
			err_flag = 1; 
			sprintf(fail_string, "%s,%s", fail_string, uart_name[i]);
			printf("add [%s] to fail string\n", uart_name[i]);
			close(fd);
			continue;
		}
		tcdrain(fd); //wait for all data to be send
		printf("send buf to uart %s success\n",path);
		memset(buf, 0, 100);
		len = uart_recev(fd, buf, datalen);
		if (len != datalen)
		{
			printf("uart receive data error:len = %d, datalen = %d\n",len,datalen); 
			err_flag = 1;
			sprintf(fail_string, "%s,%s", fail_string, uart_name[i]);
			printf("add [%s] to fail string\n", uart_name[i]);
			close(fd);
			continue;
		}
		printf("uart recev success!\n");
		if (strcmp(buf, message))
		{
			printf("uart data strcmp error!\n");
			err_flag = 1;
			sprintf(fail_string, "%s,%s", fail_string, uart_name[i]);
			printf("add [%s] to fail string\n", uart_name[i]);
			close(fd);
			continue;
		}
		close(fd);
	
	}
	if (err_flag)
	{
		printf("uart test failed!\n");
		add_fail_string(uart_case->fail_string, fail_string);
		return false;
	}
	printf("uart test success!");
	return true;
}


