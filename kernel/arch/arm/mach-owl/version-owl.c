#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/syscalls.h>

#include <mach/hardware.h>

extern unsigned char iversion_read(void);
extern unsigned int mktno_read(void);

static unsigned char verchar;
static unsigned int mno;
static DEFINE_SPINLOCK(cid_op_lock);

unsigned char icversion_get(void)
{
	return verchar;
}
EXPORT_SYMBOL(icversion_get);

unsigned int mno_get(void)
{
	return mno;
}
EXPORT_SYMBOL(mno_get);

static void __init cinfo_read(void)
{
	unsigned long flag;

	spin_lock_irqsave(&cid_op_lock, flag);
	verchar = iversion_read();
	spin_unlock_irqrestore(&cid_op_lock, flag);
	mno = mktno_read();
	printk(KERN_INFO "%s: %c, %d\n", __func__, verchar, mno);
}

static ssize_t cver_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	unsigned char cver;
	cver = icversion_get();
	return sprintf(buf, "%c\n", cver);
}
static struct kobj_attribute cver_attribute =
	__ATTR_RO(cver);

static ssize_t mno_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	unsigned int mno;
	mno = mno_get();
	return sprintf(buf, "%d\n", mno);
}
static struct kobj_attribute mno_attribute =
	__ATTR_RO(mno);

static struct attribute *cinfo_attrs[] = {
	&cver_attribute.attr,
	&mno_attribute.attr,
	NULL,
};

struct attribute_group cinfo_attr_group = {
	.name = "cinfo",
	.attrs = cinfo_attrs,
};

static struct kobject *board_info_kobj;
static int __init broad_info_init(void)
{
	int retval;

	cinfo_read();

	board_info_kobj = kobject_create_and_add("boardinfo", NULL);
	if (!board_info_kobj) {
		printk(KERN_INFO "unable to create board info kobject\n");
		return -ENOMEM;
	}
	retval = sysfs_create_group(board_info_kobj, &cinfo_attr_group);
	if (retval)
		kobject_put(board_info_kobj);

	return 0;
}

static void __exit broad_info_exit(void)
{
	kobject_put(board_info_kobj);
}

module_init(broad_info_init);
module_exit(broad_info_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Actions");
