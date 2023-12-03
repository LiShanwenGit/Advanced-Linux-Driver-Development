#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>            
#include <linux/device.h> 
#include <linux/device/driver.h>
#include <linux/kobject.h>   
#include <linux/sysfs.h>    

struct class  dummy_class = {
	.name		= "dummy_master",
	.owner		= THIS_MODULE,
};

static int bus_match(struct device *dev, struct device_driver *drv)
{
    printk(KERN_INFO"In %s \n", __func__);
    //match by driver name and device name
    return (strcmp(dev_name(dev), drv->name) == 0);
}

static int bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    int ret;
    printk(KERN_INFO "%s hotplug\n", dev_name(dev));
    ret = add_uevent_var(env, "THIS_TEST=%s", dev_name(dev));
    if(ret)
    {
        return ret;
    }
    return add_uevent_var(env, "DEMO=%s", "bus-class-test");
}

static int bus_probe(struct device *dev)
{
    return dev->driver->probe(dev);
    return 0;
}

static void bus_remove(struct device *dev)
{
    dev->driver->remove(dev);
}

struct bus_type dummy_bus_type = 
{
    .name  = "dummy_bus",
    .match = bus_match,
    .uevent= bus_uevent,
    .probe = bus_probe,
    .remove= bus_remove,
};
EXPORT_SYMBOL(dummy_bus_type);

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

static struct device_driver dummy_controller_drv=
{
    .name = "dummy_controller",
    .bus  = &dummy_bus_type,
    .probe = driver_probe,
    .remove = driver_remove,
};

static void dev_release(struct device *dev)
{
    printk(KERN_INFO "device release!\n");
}

static struct device dummy_controller_dev = {
    .init_name  = "dummy_controller",
    .bus        = &dummy_bus_type,
    .release    = dev_release,
};

static struct device dummy_0 = {
    .init_name  = "dummy_0",
    .release    = dev_release,
};


static struct device_driver dummy_device_drv=
{
    .name = "dummy_device",
    .bus  = &dummy_bus_type,
    .probe = driver_probe,
    .remove = driver_remove,
};

static struct device dummy_device_dev = {
    .init_name  = "dummy_device",
    .release    = dev_release,
};

static int __init init_test(void)
{
    int ret = 0;
    printk(KERN_INFO "init module!\n");
    ret = class_register(&dummy_class);
    if(ret)
    {
        printk(KERN_ERR "dummy_class register error!\n");
        return ret;
    }
    ret = bus_register(&dummy_bus_type);
    if (ret) {
        printk(KERN_ERR "dummy_bus_type register error!\n");
        return ret;
    }
    ret = driver_register(&dummy_controller_drv);
    if (ret) {
        printk(KERN_ERR "dummy_controller_drv register error!\n");
        return ret;
    }
    ret = driver_register(&dummy_device_drv);
    if (ret) {
        printk(KERN_ERR "dummy_device_drv register error!\n");
        return ret;
    }
    ret = device_register(&dummy_controller_dev);
    if(ret)
    {
        printk(KERN_ERR "dummy_controller_dev register error!\n");
        return ret;        
    }
    dummy_0.class = &dummy_class;
    dummy_0.parent=&dummy_controller_dev;
    ret = device_register(&dummy_0);
    if(ret)
    {
        printk(KERN_ERR "dummy_0 register error!\n");
        return ret;
    }
    dummy_device_dev.parent=&dummy_0;
    dummy_device_dev.bus = &dummy_bus_type;
    dummy_device_dev.devt = MKDEV(103, 1);
    ret = device_register(&dummy_device_dev);
    if(ret)
    {
        printk(KERN_ERR "dummy_device_dev register error!\n");
        return ret;
    }
    return ret;
}

static void __exit exit_test(void)
{
    device_unregister(&dummy_device_dev);
    device_unregister(&dummy_0);
    device_unregister(&dummy_controller_dev);
    driver_unregister(&dummy_device_drv);
    driver_unregister(&dummy_controller_drv);
    bus_unregister(&dummy_bus_type);
    class_unregister(&dummy_class);
    printk(KERN_INFO "exit module!\n");
}

module_init(init_test);
module_exit(exit_test);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("bus class test");
