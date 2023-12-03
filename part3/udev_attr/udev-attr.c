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
    //printk(KERN_INFO "%s hotplug\n", dev_name(dev));
    //return add_uevent_var(env, "MODALIAS=%s", dev_name(dev));
    return 0;
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
    .name = "udev_hotplug",
    .bus  = &bus_test,
    .probe = driver_probe,
    .remove = driver_remove,
};

static void dev_test_release(struct device *dev)
{
    printk(KERN_INFO "device release!\n");
}

static struct device dev_test = {
    .init_name  = "udev_hotplug",
    .bus        = &bus_test, 
    .release    = dev_test_release,
};

static ssize_t modalias_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "%s\n", dev_name(dev));
}
static DEVICE_ATTR(modalias, S_IRUSR, modalias_show, NULL);

static ssize_t trigger_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    if(strncmp(buf,"add",3) == 0) {
        kobject_uevent(&dev->kobj, KOBJ_ADD);
    }
    else if (strncmp(buf,"remove",6) == 0) {
        kobject_uevent(&dev->kobj, KOBJ_REMOVE);
    }
	return count;
}
static DEVICE_ATTR(trigger, S_IWUSR, NULL, trigger_store);

static int __init init_driver_test(void)
{
    int ret = 0;
    printk(KERN_INFO "init module!\n");
    ret = bus_register(&bus_test);
    if (ret) {
        printk(KERN_ERR "bus register error!\n");
        return ret;
    }
    dev_test.devt = MKDEV(103, 1);
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
    ret = device_create_file(&dev_test, &dev_attr_trigger);
	if (unlikely(ret)) {
		dev_err(&dev_test, "Failed creating attrs\n");
		return ret;
	}
    ret = device_create_file(&dev_test, &dev_attr_modalias);
	if (unlikely(ret)) {
		dev_err(&dev_test, "Failed creating attrs\n");
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
MODULE_DESCRIPTION("udev hotplug test");
