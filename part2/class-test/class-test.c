#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>            
#include <linux/device.h> 
#include <linux/device/driver.h>
#include <linux/kobject.h>   
#include <linux/sysfs.h>    


struct class  class_test = {
	.name		= "class_test",
	.owner		= THIS_MODULE,
};

static void dev_test_release(struct device *dev)
{
    
}

static struct device class_dev = {
    .init_name  = "class_dev",
    .class        = &class_test,
    .release    = dev_test_release,
};

static int __init init_class_test(void)
{
    int ret = 0;
    printk(KERN_INFO "init module!\n");
    ret = class_register(&class_test);
    if(ret)
    {
        return ret;
    }
    ret = device_register(&class_dev);
    if(ret)
    {
        printk(KERN_ERR "device register error!\n");
        return ret;        
    }
    return ret;
}


static void __exit exit_class_test(void)
{
    device_unregister(&class_dev);
    class_unregister(&class_test);
    printk(KERN_INFO "exit module!\n");
}

module_init(init_class_test);
module_exit(exit_class_test);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("class test");
