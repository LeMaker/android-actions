
#ifdef OS_LINUX
#include <linux/debugfs.h>
#include "misc_info.h"
#endif


int debug_enable = 0;

#ifdef OS_LINUX
struct dentry *dirent;
struct dentry *file;

typedef void (*debug_func_t)(char *param);
typedef struct
{
    char            *func_name;
    char            *param;
    debug_func_t     func;
} struct_debug_func;


extern int read_storage(void *buf, int start, int size);
extern int write_storage(void *buf, int start, int size);
extern int format_misc_info(void);
extern int read_mi_head(misc_info_head_t * head);
extern void print_mi_items(misc_info_head_t * head);
extern int get_item_size(misc_info_head_t * head, char *name);
extern int read_mi_item(char *name, void *buf, unsigned int count);
extern int write_mi_item(char *name, void *buf, unsigned int count);

static void debug_help(char *param);
static void debug_print(char *param);
static void debug_dump(char *param);
static void debug_format(char *param);
static void debug_print_items(char *param);
static void debug_get_item(char *param);
static void debug_set_item(char *param);

struct_debug_func debug_func_table[] =
{
    {
        "help",
        "",
        debug_help,
    },
	{
        "print",
        "enable",
        debug_print,
    },
	{
        "format",
        "",
        debug_format,
    },
	{
		"dump_mem",
		"start size",
		debug_dump,
	},
	{
        "print_items",
        "",
        debug_print_items,
    },
    {
        "read_item",
        "name",
        debug_get_item,
    },
	{
        "write_item",
        "name type value  type:C/c(char) or H/h(hex), value:length < 200",
        debug_set_item,
    },
};

static void debug_help(char *param)
{
    int i;
    PRINT_DBG("%s, line %d\n",__FUNCTION__, __LINE__);
	
    for(i = 0; i < sizeof(debug_func_table)/sizeof(struct_debug_func); i++)
    {
        PRINT("[%d] %s %s\n", i, debug_func_table[i].func_name, debug_func_table[i].param);
    }
}

static void debug_print(char *param)
{
	int enable = 0;
	
	PRINT_DBG("%s, line %d\n",__FUNCTION__, __LINE__);
	sscanf(param, "%d", &enable);

	if(enable == 0 || enable == 1){
		debug_enable = enable;
	}else{
		PRINT_ERR("invalid param\n");
		return;
	}
}

static void debug_dump(char *param)
{
	int start = 0, size = 0;
	unsigned char *buf;
	
	PRINT_DBG("%s, line %d\n",__FUNCTION__, __LINE__);
	sscanf(param, "%d %d", &start, &size);
	PRINT_DBG("dump memory start %d, size %d\n", start, size);
	if(start < 0 || size < 0 || (start + size) > MISC_INFO_MAX_SIZE){
		PRINT_ERR("invalid param\n");
		return;
	}

	buf = MALLOC(size);
	if(!buf){
		PRINT_ERR("MALLOC FAILED\n");
		return;
	}
	if(read_storage(buf, start, size) < 0){
		PRINT_ERR("read_storage failed\n");
		goto OUT;
	}
	dump_mem(buf, 0, size);
OUT:
	if(buf){
		FREE(buf);
		buf = NULL;
	}
}

static void debug_format(char *param)
{
	PRINT_DBG("%s, line %d\n",__FUNCTION__, __LINE__);
	format_misc_info();
}

static void debug_print_items(char *param)
{
	misc_info_head_t *head;
	int head_length;
	
	PRINT_DBG("%s, line %d\n",__FUNCTION__, __LINE__);
	head_length = sizeof(misc_info_head_t);
	head = MALLOC(head_length);
	memset(head, 0, head_length);
	
	if(read_mi_head(head) < 0){
		PRINT_ERR("read Head failed\n");
		return;
	}
	print_mi_items(head);
	
	if(head){
		FREE(head);
		head = NULL;
	}
}

static void debug_get_item(char *param)
{
    char name[8] = {0};
	int head_length, size, ret = -1;
	void *buf;
	misc_info_head_t *head;
	
    sscanf(param, "%s", name);
	if(name[7] != 0)
		name[7] = 0;
	PRINT_DBG("%s, line %d, name %s\n",__FUNCTION__, __LINE__, name);
	
	head_length = sizeof(misc_info_head_t);
	head = MALLOC(head_length);
	memset(head, 0, head_length);
	
	if(read_mi_head(head) < 0){
		PRINT_ERR("read Head failed\n");
		goto OUT_FAILED;
	}
	size = get_item_size(head, name);
	if(size <= 0){
		PRINT_ERR("no item named %s\n",name);
		goto OUT_FAILED;
	}
	buf = MALLOC(size);
	memset(buf, 0, size);
	ret = read_mi_item(name, buf, size);
	if(ret < 0){
		PRINT_ERR("read data failed\n");
	}else{
		dump_mem(buf, 0, ret);
	}
	
	if(buf){
		FREE(buf);
		buf = NULL;
	}
	
OUT_FAILED:
	if(head){
		FREE(head);
		head = NULL;
	}
}

int check_hex(char *buf)
{
	int i;
	for(i = 0; i < strlen(buf); i++){
		if((buf[i] >= '0' && buf[i] <= '9')
			|| (buf[i] >= 'a' && buf[i] <= 'f')
			|| (buf[i] >= 'A' && buf[i] <= 'F')){
				continue;
		}else{
			return -1;
		}
	}
	return 0;
}

void char_to_hex(char *in, char *out)
{
	int i;
	char tmp = 0, value = 0;
	char buf[128] = {0};
	
	//printk("%s, strlen(in)=%d\n",__FUNCTION__, (int)strlen(in));
	if(strlen(in) % 2 == 1){
		PRINT_ERR("must be divide 2\n");
		return;
	} 
	for(i = 0; i < strlen(in); i++){
		if(in[i] >= '0' && in[i] <= '9')
			tmp = in[i] - '0';
		else if(in[i] >= 'a' && in[i] <= 'f')
			tmp = in[i] - 'a' + 10;
		else if(in[i] >= 'A' && in[i] <= 'F')
			tmp = in[i] - 'A' + 10;
		
		if(i % 2 == 0){
			value = tmp * 16;
		}else{
			value += tmp;
			buf[i/2] = value;
		}
	}
	memcpy(out, buf, strlen(in)/2);
}

static void debug_set_item(char *param)
{
    char name[8] = {0};
	int ret = -1;
	char type;
	char value[256] = {0}, buf[256] = {0};
	
    sscanf(param, "%s %c %s", name, &type, value);
	PRINT_DBG("%s, line %d\n",__FUNCTION__, __LINE__);
    PRINT_DBG("name %s, type %c, value %s\n", name, type, value);
	
	if(type == 'H' || type == 'h'){
		if(check_hex(value) < 0){
			PRINT_ERR("err, has invalid hex\n");
			return;
		}
		char_to_hex(value, buf);
		ret = strlen(value) / 2;//!!!not strlen(buf), because aa00 can be turn to aa.
	}else if(type == 'C' || type == 'c'){
		memcpy(buf, value, strlen(value));
		ret = strlen(value);
	}else{
		PRINT_ERR("invalid type %c\n", type);
		return;
	}
	//dump_mem(buf, 0, ret);
	
	ret = write_mi_item(name, buf, ret);
	if(ret < 0){
		PRINT_ERR("read data failed\n");
	}
}

static ssize_t mi_debug_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
    char cmd[512] = {0};
    char func_name[32] = {0};
    char param[256] = {0};
	
    char *cmd_ptr, *param_ptr;
    int i, len, flag;
    
//	printk("%s, line %d\n",__FUNCTION__, __LINE__);
	
    if(copy_from_user(cmd, buf, size))
    {
        PRINT_ERR("%s %d, copy_from_user has wrong\n", __FUNCTION__, __LINE__);
    }
//    printk("cmd %s\n", cmd);
	
    for(i = 0; i < size; i++)
    {
        if(cmd[i] == '\n')
        {
            cmd[i] = 0;
            break;
        }
    }
    
    len = strlen(cmd);
    flag = 0;
    for(i = 0; i < len; i++)
    {
		if(cmd[i] != ' '){
			flag = 1;
		}
        else if(flag == 1)
        {
            memcpy(func_name, cmd, i);
            func_name[i] = 0;
			break;
        }
    }
    if(i == len)
        strcpy(func_name, cmd);
        
    PRINT_DBG("func_name=%s\n", func_name);
    
    if(i != len)
    {
        cmd_ptr = cmd + i + 1;
        param_ptr = param;
        len = strlen(cmd_ptr);
        for(i = 0; i < len; i++)
        {
			if(cmd_ptr[i] != ' '){
				*param_ptr++ = cmd_ptr[i];
			}
			else if(param_ptr > param && *(param_ptr-1) != ' ')
			{
				*param_ptr++ = ' ';
			}
        }
        *param_ptr = 0;
        PRINT_DBG("param=%s\n", param);
    }

    for(i = 0; i < sizeof(debug_func_table)/sizeof(struct_debug_func); i++)
    {
        if(strcmp(func_name, debug_func_table[i].func_name) == 0 )
        {
            (debug_func_table[i].func)(param);
            break;
        }
    }
    
    if(i == sizeof(debug_func_table)/sizeof(struct_debug_func))
    {
        PRINT_ERR("can not find function %s\n", func_name);
    }
    return size;
}

static const struct file_operations mi_op = {
    .write = mi_debug_write,
};

int mi_debug_init(void)
{
	PRINT("%s, line %d\n",__FUNCTION__, __LINE__);
	dirent = debugfs_create_dir("misc_info", NULL);
    file = debugfs_create_file("debug", S_IFREG | S_IRUGO, dirent, NULL, &mi_op);
    
    return 0;
}

int mi_debug_exit(void)
{   
	PRINT("%s, line %d\n",__FUNCTION__, __LINE__);
	debugfs_remove(file);
    debugfs_remove(dirent);
    return 0;     
}

#endif

