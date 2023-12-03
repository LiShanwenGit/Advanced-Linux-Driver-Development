#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>            
#include <linux/device.h> 
#include <linux/device/driver.h>
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
    int ret=1;
    printk(KERN_INFO "%s hotplug\n", dev_name(dev));
    ret = add_uevent_var(env, "THIS_TEST=%s", dev_name(dev));
    if(ret)
    {
        return ret;
    }
    return add_uevent_var(env, "EVENT=%s", "change");
}

static int bus_probe(struct device *dev)
{
    int ret = dev->driver->probe(dev);
    return ret;
}

static void bus_remove(struct device *dev)
{
    dev->driver->remove(dev);
}

struct bus_type bus_test = 
{
    .name  = "bus-test",
    .match = bus_match,
    .uevent= bus_uevent,
    .probe = bus_probe,
    .remove= bus_remove,
};
EXPORT_SYMBOL(bus_test);

static int driver_probe(struct device *dev)
{
    printk(KERN_INFO "driver probe!\n");
    return 0;
}

static int driver_remove(struct device *dev)
{
    printk(KERN_INFO "driver remove!\n");
    return 0;
}

static struct device_driver driver_test=
{
    .name = "driver_test",
    .bus  = &bus_test,
    .probe = driver_probe,
    .remove = driver_remove,
};

static void dev_test_release(struct device *dev)
{
    printk(KERN_INFO "device release!\n");
}

static struct device dev_test = {
    .init_name  = "driver_test",
    .bus        = &bus_test, 
    .release    = dev_test_release,
};

static int __init init_driver_test(void)
{
    int ret = 0;
    printk(KERN_INFO "init module!\n");
    ret = bus_register(&bus_test);
    if (ret) {
        printk(KERN_ERR "bus register error!\n");
        return ret;
    }
    dev_test.devt=MKDEV(100, 1);
    ret = device_register(&dev_test);
    if(ret)
    {
        printk(KERN_ERR "device register error!\n");
        return ret;        
    }
    ret = driver_register(&driver_test);
    if (ret) {
        printk(KERN_ERR "driver register error!\n");
        return ret;
    }
    return ret;
}

static void __exit exit_driver_test(void)
{
    driver_unregister(&driver_test);
    device_unregister(&dev_test);
    bus_unregister(&bus_test);
    printk(KERN_INFO "exit module!\n");
}
module_init(init_driver_test);
module_exit(exit_driver_test);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("hotplug env test");
