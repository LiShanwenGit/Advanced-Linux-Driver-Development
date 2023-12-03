#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

extern struct bus_type bus_test_type;

static void dev_test_release(struct device *dev)
{

}

static struct device dev_test = {
    .init_name  = "dev_test",
    .bus        = &bus_test_type,  //总线必须初始化，用来与驱动进行匹配
    .release    = dev_test_release,
};

static int __init init_dev_test(void)
{
    int ret = 0;
    ret = device_register(&dev_test);
    if(ret < 0)
    {
        printk(KERN_ERR "device register error!\n");
        return ret;        
    }
    return ret;
}

static void __exit exit_dev_test(void)
{
    device_unregister(&dev_test);
}

module_init(init_dev_test);
module_exit(exit_dev_test);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("device test");