
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#define  LOG_TAG    "inject_event_parser"
#include "uinput.h"

#define NAME_TAG        "Name="
#define HANDLER_TAG     "Handlers="
#define KBD_TAG         "kbd"
#define EVENT_TAG       "event"
#define MAX_INPUT_NUM    10

typedef struct {
    int isKBD;
    char name[32];
}input_dev_t;

static int s_parsed = 0;
static input_dev_t s_input_devs[MAX_INPUT_NUM];

static int eliminate_line_tag(const char *buff)
{
    int ret = 0;
    while(*buff != 0 && (*buff == 0x0d || *buff == 0x0a)) {
        buff ++;
        ret ++;
    }
    return ret;
}

static int read_line(const char *buff, char *linebuf)
{
    if(buff == NULL || linebuf == NULL) {
        LOGE("null parameter!");
        return 0;
    }

    int ret = 0;
    int tmp = eliminate_line_tag(buff);
    buff += tmp;
    ret += tmp;
    while(*buff != 0 && *buff != 0x0d && *buff != 0x0a) {
        *linebuf = *buff;
        linebuf ++;
        buff ++;
        ret ++;
    }
    tmp = eliminate_line_tag(buff);
    buff += tmp;
    ret += tmp;
    *linebuf = 0;
    return ret;
}


static int parse_input()
{
    s_parsed = 0;
    memset(s_input_devs, 0, sizeof(s_input_devs));

    int fd = open("/proc/bus/input/devices", O_RDONLY);
    if(fd < 0) {
        LOGE("open input devices fail!");
        return -1;
    }

    char *filebuf = (char *)malloc(512);
    if(filebuf == NULL) {
        LOGE("malloc fail!");
        close(fd);
        return -2;
    }

    int size = 0;
    int tmp;
    do {
       tmp = read(fd, filebuf, 512);
       if(tmp <= 0) {
           break;
       }
       size += tmp;
    } while(1);
    LOGI("input/devices file size: %d\n", size);

    free(filebuf);
    filebuf = (char *)malloc(size+1);
    memset(filebuf, 0, size+1);
    if(filebuf == NULL) {
        LOGE("malloc fail!");
        close(fd);
        return -2;
    }
    lseek(fd, 0, SEEK_SET);
    read(fd, filebuf, size);
    filebuf[size] = 0;
    //printf("%s\n", filebuf);

    // parse
    char *tmpFileBuf = filebuf;
    char linebuf[256];
    char curName[32] = {0};
    while(tmpFileBuf - filebuf < size) {
        tmp = read_line(tmpFileBuf, linebuf);
        if(tmp == 0) {
            LOGE("no more characters to be reads!");
            break;
        }
        tmpFileBuf += tmp;
        //printf("line:%s, ====chars: %d\n", linebuf, tmp);
        char *tmpstr = strstr(linebuf, NAME_TAG);
        if(tmpstr != NULL) {
            memset(curName, 0, sizeof(curName));
            strcpy(curName, tmpstr+strlen(NAME_TAG)+1); // ignore double quote
            tmp = sizeof(curName) - 1;
            while(tmp > 0) {
                if(curName[tmp] == ' ') {
                    curName[tmp] = 0;
                } else if(curName[tmp] == '\"') {
                    curName[tmp] = 0;
                    break;
                }
                tmp --;
            }
            LOGI("name: %s", curName);
            continue;
        }

        tmpstr = strstr(linebuf, HANDLER_TAG);
        if(tmpstr != NULL) {
            char *tmpstr1 = strstr(tmpstr, KBD_TAG);
            tmpstr = strstr(tmpstr, EVENT_TAG);
            if(tmpstr != NULL) {
                char handlers[4] = {0};
                memset(handlers, 0, sizeof(handlers));
                memcpy(handlers, tmpstr+strlen(EVENT_TAG), 2);
                int id = atoi(handlers);
                LOGI("handler:event%d, kbd:%d\n", id, (tmpstr1 != NULL));
                if(id >= 0 && id < MAX_INPUT_NUM) {
                    strcpy(s_input_devs[id].name, curName);
                    //s_input_devs[id].isKBD = (tmpstr1 != NULL);
                    s_input_devs[id].isKBD = (id < 3);
                } else {
                    LOGE("input dev id overflow:%d, maxmium: %d\n", id, MAX_INPUT);
                }
                continue;
            }
        }
    }

    close(fd);
    free(filebuf);
    s_parsed = 1;
    return 0;
}

int get_input_device_id(const char *name)
{
    if(!s_parsed) {
        parse_input();
    }

    int i = 0;
    for(i = 0; i < MAX_INPUT; i++) {
        if(s_input_devs[i].name[0] != 0 && !strcasecmp(s_input_devs[i].name, name)) {
            return i;
        }
    }
    return -1;
}

int is_kbd_input(int id)
{
    if(!s_parsed) {
        parse_input();
    }

    if(id >= MAX_INPUT) {
        return 0;
    }
    return 1;
    //return s_input_devs[id].isKBD;
}

char *get_input_name(int id)
{
    if(!s_parsed) {
        parse_input();
    }

    if(id >= MAX_INPUT || s_input_devs[id].name[0] == 0) {
        return 0;
    }
    return s_input_devs[id].name;
}

