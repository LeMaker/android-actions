#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <mach/bootdev.h>


extern unsigned int card_total_size; 

extern int tSD_single_blk_test(unsigned start, unsigned blocks);
extern int __do_adfu_read(unsigned start, unsigned nsector, void *buf);
extern int __do_adfu_write(unsigned start, unsigned nsector, void * buf);

extern void NAND_ShowMiscInfoAll(void);

extern int NAND_WriteMiscInfo(int type, char * buf, int size);	
extern int NAND_GetMiscInfo(int type, char * buf, int size);

extern unsigned int card_total_size;
unsigned int tSD_get_logic_cat(void);
static ssize_t card_proc_storage_type_read (struct file *file, 
									char __user *buffer, size_t count, loff_t * ppos);

static ssize_t card_proc_phy_cap_read (struct file *file,
								char __user *buffer, size_t count, loff_t * ppos);

static ssize_t card_proc_logic_cap_read (struct file *file,
								char __user *buffer, size_t count, loff_t * ppos)	;  

static ssize_t card_proc_info_write(struct file *file, const char __user *buffer,
		size_t count, loff_t * ppos);


#define CARD_DIR_NAME	          "mmc"
#define STORAGE_TYPE_NAME	      "storage_type"
#define CARD_LOGIC_FILE_NAME	  "logic_cap"
#define CARD_PHY_FILE_NAME        "phy_cap" 
#define CARD_INFO        		  "mmc_info" 

static struct proc_dir_entry *proc_card = NULL;
static struct proc_dir_entry *proc_storage_type = NULL;
static struct proc_dir_entry *proc_card_logic_info = NULL;
static struct proc_dir_entry *proc_card_cap = NULL;
static struct proc_dir_entry *proc_card_info = NULL;

#define isxdigit(c)     (('0' <= (c) && (c) <= '9') \
		|| ('a' <= (c) && (c) <= 'f') \
		|| ('A' <= (c) && (c) <= 'F'))

#define isdigit(c)      ('0' <= (c) && (c) <= '9')
#define islower(c)      ('a' <= (c) && (c) <= 'z')
#define toupper(c)      (islower(c) ? ((c) - 'a' + 'A') : (c))

#define my_isspace(c)      (c == ' ' || c == '\t' || c == 10 || c == 13 || c == 0)
#define isspace(c)      (c == ' ' || c == '\t' || c == 10 || c == 13 || c == 0)
#define TOLOWER(x)         ((x) | 0x20)


static const struct file_operations proc_ops[] = {
	{
		.owner		= THIS_MODULE,

		.read			= card_proc_logic_cap_read,
	},
	{
		.owner		= THIS_MODULE,
		.read			= card_proc_phy_cap_read,
	},
	{
		.owner		= THIS_MODULE,
		.read		= card_proc_storage_type_read,
	},
	
	{
		.owner		= THIS_MODULE,
		.write		= card_proc_info_write,
	},
	
};





static int my_atoi(char *psz_buf)
{
	char *pch = psz_buf;
	int base = 0;
	unsigned long long result = 0;
	unsigned int value;

	while (my_isspace(*pch))
		pch++;

	if (*pch == '-' || *pch == '+') {
		base = 10;
		pch++;
	} else if (*pch && TOLOWER(pch[strlen(pch) - 1]) == 'h') {
		base = 16;
	}

	if (pch[0] == '0') {
		if (TOLOWER(pch[1]) == 'x' && isxdigit(pch[2]))
			base = 16;
		else
			base = 8;
	} else {
		base = 10;
	}

	if (base == 16 && pch[0] == '0' && TOLOWER(pch[1]) == 'x')
		pch += 2;

	while (isxdigit(*pch)) {

		value = isdigit(*pch) ? *pch - '0' : TOLOWER(*pch) - 'a' + 10;
		if (value >= base)
			break;
		result = result * base + value;
		pch++;
	}

	return result;
}

/******************************************************************************/
/*!
* \par  Description:
*     dump memory
* \param[in]    startaddr, start address
* \param[in]    size, size of the memory
* \param[in]    showaddr, offset of the memory that wwe want to show
* \param[in]    show_bytes, show type: 1£ºbyte£¬2£ºshort£¬4£ºint
* \ingroup      card_proc.c
* \par          exmaple code
* \code
*               dump_mem(ret_v0_uncached, 512, 0, 1);
* \endcode
*******************************************************************************/
void dump_mem(void *startaddr, unsigned int size, unsigned int showaddr, unsigned int show_bytes)
{
	unsigned int i, count, count_per_line;
	void *addr = startaddr;

	if ((show_bytes!=1) && (show_bytes!=2) && (show_bytes!=4)){
		printk("dump_mem: not support mode\n");
		return;
	}

	if (((unsigned int)startaddr & (show_bytes -1)) ||\
		((unsigned int)size & (show_bytes -1))){
		printk("dump_mem: startaddr must be align by %d bytes!\n", show_bytes);
		return;
	}

	count = size / show_bytes;
	count_per_line = 16 / show_bytes;   // 16 bytes per line

	printk("\nstartaddr %p, size %d, count %d, count_per_line %d\n",
	startaddr, size, count, count_per_line);

	i = 0;
	while(i < count){
		
		if (!(i % count_per_line)){
			printk("\n%08x: ", showaddr + (i / count_per_line) * 16);
		}

		switch (show_bytes){
		case 1:
			printk("%02x ", *((unsigned char *)addr + i));
			break;
		case 2:
			printk("%04x ", *((unsigned short *)addr + i));
			break;
		case 4:
			printk("%08x ", *((unsigned int *)addr + i));
			break;
		default:
			printk("dump_mem: not support mode\n");
			
			return;
		}

			i++;
	}
	printk("\n");
}


void init_mem(unsigned char * buf, int seq, int size)
{
	int i;
	for(i=0;i<size;i++){
		buf[i] = seq++;    
	}
    
}


static ssize_t card_proc_storage_type_read (struct file *file, 
									char __user *buffer, size_t count, loff_t * ppos)
{

	int len;
	int ret;
	unsigned int boot_dev = 0;
	char * emmc_buf = "emmc";
	char * tsd_buf = "tsd";
	char * nand_buf = "nand";

	boot_dev=owl_get_boot_dev();

	if(*ppos > 0)      
		return 0;
	
	if(boot_dev == OWL_BOOTDEV_SD2){

		ret = copy_to_user(buffer,emmc_buf,strlen(emmc_buf));
		len = strlen(emmc_buf); 
	}else if(boot_dev == OWL_BOOTDEV_SD0){
		ret = copy_to_user(buffer,tsd_buf,strlen(tsd_buf));
		len = strlen(tsd_buf); 
	}else{
		ret = copy_to_user(buffer,nand_buf,strlen(nand_buf));
		len = strlen(nand_buf);
	}

	if(ret){
		return -EFAULT;
	}

	*ppos += len;

	return len;

}


static ssize_t card_proc_phy_cap_read (struct file *file,
								char __user *buffer, size_t count, loff_t * ppos)
{

	int len;	
	char buf[32]={0};	
	
	unsigned int total_size; //Mb	

	if(*ppos > 0)      
		return 0;
	total_size = card_total_size / 2048 ; 

	len = sprintf(buf, "%d\n", total_size);		
	if(copy_to_user(buffer, buf, len))        
		return -EFAULT;	  
	
	*ppos += len;	    
	return len;
}

static ssize_t card_proc_logic_cap_read (struct file *file,
								char __user *buffer, size_t count, loff_t * ppos)	  
{
	int len;	
	char buf[32]={0};	
	unsigned int logic_size; //Mb	

	if(*ppos > 0)      
		return 0;
	
	logic_size = tSD_get_logic_cat()/2048 ; 

	len = sprintf(buf, "%d\n", logic_size);		
	if(copy_to_user(buffer, buf, len))        
		return -EFAULT;	  
	
	*ppos += len;	    
	return len;
}


static ssize_t card_proc_info_write(struct file *file, const char __user *buffer,
		size_t count, loff_t * ppos)
{

	loff_t pos = *ppos;
	char *buf = NULL;
	char *card_read_buf = NULL;
	char *card_write_buf = NULL;
	char * b = NULL;
	
	char op;
	int ret = 0;
	int i,j;
	
	int argc,blk_num, card_addr;
	int param[4]= {-1};
	char **argv = NULL;

	printk("%s, count: %u\n", __FUNCTION__, count);
	buf = kzalloc(count+1, GFP_KERNEL);
	if(buf == NULL){
		printk("buf alloc error \n");
		count = 0;
		goto out4;

	}
	card_read_buf = kzalloc(512 * 1024 ,GFP_KERNEL);
	if(card_read_buf == NULL){
		printk("card_read_buf alloc error \n");
		count = 0;
		goto out3;

	}
	
	card_write_buf = kzalloc(512 * 1024 ,GFP_KERNEL);
	if(card_write_buf == NULL){
		printk("card_write_buf alloc error \n");
		count = 0;
		goto out2;

	}
	
	b = kmalloc(512, GFP_KERNEL);
	if(NULL == b){
		printk("%s %d:fail malloc buf\n",__FUNCTION__,__LINE__);
		count = 0;
		goto out1;
	}

	ret = copy_from_user(buf, buffer, count);
	if(ret){
		printk("%s err!\n", __FUNCTION__);
	}
	
	buf[count-1] = 0;

	printk("%s\n", buf);


	
	argv = argv_split(GFP_KERNEL, buf, &argc);
	
	for(i=0,j=0;i<argc;i++){
		//printk("%d %s %d %d!\n", i,  argv[i], my_atoi(argv[i]), argc);
		char tmp = argv[i][0];
		if(isdigit(tmp)){
			param[j++]= my_atoi(argv[i]);
		}
	}



	op=buf[0];

	switch(op){
		case 'o':
				printk("%s %d\n", __FUNCTION__, __LINE__);
				break;	
		case 'r':
				blk_num = (param[0] != -1) ? param[0]:0;
				card_addr = (param[1] != -1) ? param[1]:0;
				printk("card read, blk num: %d, card address: %d\n",blk_num, card_addr);

				__do_adfu_read(card_addr, blk_num, card_read_buf);
				//void dump_mem(void *startaddr, unsigned int size, unsigned int showaddr, unsigned int show_bytes)
				dump_mem(card_read_buf, blk_num * 512, 0, 1);
				break;
		case 'w':	
				blk_num = (param[0] != -1) ? param[0]:0;
				card_addr = (param[1] != -1) ? param[1]:0;
				printk("card read, blk num: %d, card address: %d\n",blk_num, card_addr);
				init_mem(card_write_buf, 0x0, 512 * 1024);
				__do_adfu_write(card_addr, blk_num, card_write_buf);
				break;
		case 'u':	
	
				init_mem(b, 0x0, 308);
				NAND_WriteMiscInfo(2,b,308);
				kfree(b);
				break;
		case 's':	

				init_mem(b, 0x0, 16);
				NAND_WriteMiscInfo(0,b,16);
				kfree(b);
				break;
		case 'h': //read the hdcp

				memset(b,0xff,512);	
				NAND_GetMiscInfo(2,b,308);
				dump_mem(b, 308, 0, 1);
				kfree(b);
				break;
		case 't': 
				memset(b,0xff,512);	
				NAND_GetMiscInfo(0,b,16);
				dump_mem(b, 16, 0, 1);
				kfree(b);
				break;
		case 'e':
				printk("card_total_size:0x%x\n", card_total_size);
				
				__do_adfu_read(card_total_size - 2048 + 1, 1, b);
				dump_mem(b, 308, 0, 1);
				
				memset(b,0xff,512);	
				__do_adfu_write(card_total_size - 2048 + 1, 1, b);
				printk("after memset b(0xff)...\n");

				__do_adfu_read(card_total_size - 2048 + 1, 1, b);
				dump_mem(b, 308, 0, 1);
				kfree(b);
				break;
		default:
				printk("=== CARD PROC DEBUG KIT ====\n");
				break;

				
	}

out1:	
	kfree(card_write_buf);
out2:	
	kfree(card_read_buf);	
out3:	
	kfree(buf);	

out4:
	argv_free(argv);
	argv = NULL;

	*ppos = pos + count;
	
	return count;
}


int init_card_proc(void)
{
	proc_card = proc_mkdir(CARD_DIR_NAME, NULL);
	if (!proc_card){
		return -1;
	}
	/* card driver r/w infomation */
	proc_card_logic_info= proc_create(CARD_LOGIC_FILE_NAME, 0, proc_card, &proc_ops[0]);
	if (!proc_card_logic_info) {
		return -1;
	}

	
	/* card cap information */
	proc_card_cap= proc_create(CARD_PHY_FILE_NAME, 0, proc_card, &proc_ops[1]);
	if (!proc_card_cap) {
		return -1;
	}


	proc_storage_type= proc_create(STORAGE_TYPE_NAME, 0, proc_card, &proc_ops[2]);
	if (!proc_storage_type) {
		return -1;
	}	


	proc_card_info= proc_create(CARD_INFO, 0, proc_card, &proc_ops[3]);
	if (!proc_card_info) {
		return -1;
	}

	return 0;
}

int cleanup_card_proc(void)
{

	if(proc_card_info){
		remove_proc_entry(CARD_INFO, proc_card);
	}

	if(proc_storage_type){
		remove_proc_entry(STORAGE_TYPE_NAME, proc_card);
	}

	if(proc_card_cap){
		remove_proc_entry(CARD_PHY_FILE_NAME, proc_card);
	}

	if(proc_card_logic_info){
		remove_proc_entry(CARD_LOGIC_FILE_NAME, proc_card);
	}
	

	if (proc_card){
		remove_proc_entry(CARD_DIR_NAME, proc_card);
	}

	return 0;
}






