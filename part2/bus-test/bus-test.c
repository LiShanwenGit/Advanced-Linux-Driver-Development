#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>            
#include <linux/device.h> 
#include <linux/kobject.h>   
#include <linux/sysfs.h>    


static int bus_match(struct device *dev, struct device_driver *drv)
{
    printk(KERN_INFO"In %s \n", __func__);
    //match by driver name and device name
    return (strcmp(dev_name(dev), drv->name) == 0);
}

static int bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    printk(KERN_INFO "%s hotplug\n",dev_name(dev));
    return add_uevent_var(env, "MODALIAS=%s", dev_name(dev));
}

struct bus_type bus_test_type = {
    .name  = "bus-test",
    .match = bus_match,
    .uevent= bus_uevent,
};
EXPORT_SYMBOL(bus_test_type);

static int init_bus_test(void)
{
    int ret = 0;
    printk(KERN_INFO "init bus module!\n");
    ret = bus_register(&bus_test_type);
    if (ret) {
        printk(KERN_ERR "bus_register error!\n");
        return ret;
    }
    return ret;
}

static void exit_bus_test(void)
{
    bus_unregister(&bus_test_type);
    printk(KERN_INFO "exit bus module!\n");
}
module_init(init_bus_test);
module_exit(exit_bus_test);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("bust test");
