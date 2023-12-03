#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

static struct kobject *test_kobj; 

static ssize_t xxx_ts1_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    int ret=0;
    return ret;
}

static ssize_t xxx_ts1_store(struct kobject* kobjs, struct kobj_attribute *attr, const  char *buf, size_t count)
{
    int ret=0;
    return ret;
}

static ssize_t xxx_ts2_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    int ret=0;
    return ret;
}

static ssize_t xxx_ts2_store(struct kobject* kobjs, struct kobj_attribute *attr, const  char *buf, size_t count)
{
    int ret=0;
    return ret;
}

static ssize_t xxx_ts3_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    int ret=0;
    return ret;
}

static ssize_t xxx_ts3_store(struct kobject* kobjs, struct kobj_attribute *attr, const  char *buf, size_t count)
{
    int ret=0;
    return ret;
}

static struct kobj_attribute xxx_ts1 = __ATTR(xxx_ts1, S_IWUSR | S_IRUGO, xxx_ts1_show, xxx_ts1_store);
static struct kobj_attribute xxx_ts2 = __ATTR(xxx_ts2, S_IWUSR | S_IRUGO, xxx_ts2_show, xxx_ts2_store);
static struct kobj_attribute xxx_ts3 = __ATTR(xxx_ts3, S_IWUSR | S_IRUGO, xxx_ts3_show, xxx_ts3_store);

static struct attribute *xxx_atttrs[] = {
    &xxx_ts1.attr,
    &xxx_ts2.attr,
    &xxx_ts3.attr,
    NULL
};

static const struct attribute_group xxx_gr = {
    .name = "xxx",
    .attrs = xxx_atttrs
};
static int __init sysfs_test_init(void)
{
    int res;
    test_kobj = kobject_create_and_add("sys_gr_ts",NULL);
    if(test_kobj == NULL)
    {
        printk(KERN_INFO"create test_kobj failed!\n");
        return -1;
    }
    res = sysfs_create_group(test_kobj, &xxx_gr);
    if (res) {
        printk(KERN_ERR"create sysfs file failed!");
        return res;
    }
    return 0;
}

static void __exit sysfs_test_exit(void)
{
    sysfs_remove_group(test_kobj, &xxx_gr);
    kobject_put(test_kobj);
}

module_init(sysfs_test_init);
module_exit(sysfs_test_exit);

MODULE_LICENSE("GPL");        
MODULE_AUTHOR("1477153217@qq.com");  
MODULE_VERSION("0.1");      
MODULE_DESCRIPTION("sysfs group test");