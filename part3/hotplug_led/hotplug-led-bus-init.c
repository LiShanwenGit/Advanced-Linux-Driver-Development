#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>            
#include <linux/device.h> 
#include <linux/device/driver.h>
#include <linux/kobject.h>   
#include <linux/sysfs.h>
#include <linux/gpio/consumer.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/gpio.h>

struct hotplug_led_dev 
{
    struct device dev;
    const char *match;
    struct gpio_desc *pin;
};

struct hotplug_led_drv 
{
    struct device_driver driver;
    const char *match;
    int (*probe) (struct hotplug_led_dev *dev);
	void (*remove) (struct hotplug_led_dev *dev);
};

static int led_bus_match(struct device *dev, struct device_driver *drv)
{
    struct hotplug_led_dev *led_dev =  container_of(dev, struct hotplug_led_dev, dev);
    struct hotplug_led_drv *led_drv =  container_of(drv, struct hotplug_led_drv, driver);
    //match by driver match and device match
    printk(KERN_INFO"In %s \n", __func__);
    return (strcmp(led_drv->match, led_dev->match) == 0);
}

static int led_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    printk(KERN_INFO "%s hotplug\n", dev_name(dev));
    return add_uevent_var(env, "MODALIAS=%s", dev_name(dev));
}

static int led_bus_probe(struct device *dev)
{
    struct hotplug_led_dev *led_dev =  container_of(dev, struct hotplug_led_dev, dev);
    struct hotplug_led_drv *led_drv = container_of(dev->driver, struct hotplug_led_drv, driver);
	int ret = led_drv->probe(led_dev);
    return ret;
}

static void led_bus_remove(struct device *dev)
{
    struct hotplug_led_dev *led_dev =  container_of(dev, struct hotplug_led_dev, dev);
    struct hotplug_led_drv *led_drv = container_of(dev->driver, struct hotplug_led_drv, driver);
	led_drv->remove(led_dev);
}

struct bus_type led_bus = 
{
    .name  = "led_bus",
    .match = led_bus_match,
    .uevent= led_bus_uevent,
    .probe = led_bus_probe,
    .remove= led_bus_remove,
};
EXPORT_SYMBOL(led_bus);

static void hotplug_led_release(struct device *dev)
{

}

static ssize_t hotplug_led_modalias_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "%s\n", dev_name(dev));
}
static DEVICE_ATTR(modalias, S_IRUSR, hotplug_led_modalias_show, NULL);

struct class  hotplug_led_class = {
	.name		= "hotplug_led",
	.owner		= THIS_MODULE,
};


static int __init led_bus_init(void)
{
    int ret = 0;
    struct device_node *root, *ctrl, *child;
    struct property *prop;
    const char *match = NULL;
    struct hotplug_led_dev *ctrl_dev, *led_dev;
    struct device *bus_dev;
    int dev_index = 0;
    char *bus_dev_name;
    printk(KERN_INFO "led bus init!\n");
    ret = class_register(&hotplug_led_class);
    if(ret)
    {
        printk(KERN_ERR "hotplug_led class register error!\n");
        return ret;
    }
    ret = bus_register(&led_bus);
    if (ret) {
        printk(KERN_ERR "led bus register error!\n");
        goto err;
    }
    root = of_find_node_by_path("/"); //找到设备树的根节点
    //这里我们去搜索根节点得子节点，如果存在matching属性，则进行注册控制器设备
    for_each_child_of_node(root, ctrl){
        prop = of_find_property(ctrl, "matching", NULL); //判断是否存在该属性的节点
        if(prop)
        {
            ret = of_property_read_string(ctrl, "matching", &match);
            if(ret)
            {
                printk(KERN_ERR "get led device property failed!\n");
                goto err0;
            }
            ctrl_dev = kzalloc(sizeof(struct hotplug_led_dev), GFP_KERNEL);
            if (!ctrl_dev) {
                printk(KERN_ERR "alloc ctrl_dev failed!\n");
                goto err0;
            }
            ctrl_dev->dev.init_name = ctrl->name;
            ctrl_dev->dev.of_node = ctrl;
            ctrl_dev->match = match;
            ctrl_dev->dev.bus = &led_bus; 
            ctrl_dev->dev.release = hotplug_led_release;
            ret = device_register(&ctrl_dev->dev);
            if(ret) {
                printk(KERN_ERR "%s controller register error!\n", ctrl->name);
                goto err1;
            }
            else {
                printk(KERN_INFO "%s controller register successful!\n", ctrl->name);
                ret = device_create_file(&ctrl_dev->dev, &dev_attr_modalias);
                if (unlikely(ret)) {
                    dev_err(&ctrl_dev->dev, "Failed to creating modalias attribute file\n");
                    goto err2;
                }
            }
            bus_dev = kzalloc(sizeof(struct device), GFP_KERNEL);
            bus_dev_name = kzalloc(32, GFP_KERNEL);
            snprintf(bus_dev_name, 32, "hotplug_led%d", dev_index++);
            bus_dev->init_name = bus_dev_name;
            bus_dev->release = hotplug_led_release;
            bus_dev->class = &hotplug_led_class;
            bus_dev->parent = &ctrl_dev->dev;
            ret = device_register(bus_dev);
            if(ret) {
                printk(KERN_ERR "%s bus_dev register error!\n", child->name);
                goto err3;
            }
            child = of_get_next_child(ctrl, NULL); //查找该节点的子节点，去注册子设备
            if(child) {
                //如果存在matching，则进行注册子设备
                ret = of_property_read_string(child, "matching", &match); 
                if(ret) {
                    printk(KERN_ERR "get led device property failed!\n");
                    goto err4;
                }
                led_dev = kzalloc(sizeof(struct hotplug_led_dev), GFP_KERNEL);
                if (!led_dev) {
                    return -ENOMEM;
                }
                led_dev->dev.init_name = child->name;
                led_dev->dev.of_node = child;
                led_dev->match = match;
                led_dev->dev.bus = &led_bus; 
                led_dev->dev.parent = bus_dev;
                led_dev->dev.release = hotplug_led_release;
                prop = of_find_property(child, "removable", NULL); //判断是否存在该属性
                if(prop) {
                    led_dev->dev.removable = DEVICE_REMOVABLE;
                }
                ret = device_register(&led_dev->dev);
                if(ret) {
                    printk(KERN_ERR "%s device register error!\n", child->name);
                    goto err5;
                }
                else {
                    printk(KERN_INFO "%s device register successful!\n", child->name);
                    ret = device_create_file(&led_dev->dev, &dev_attr_modalias);
                    if (unlikely(ret)) {
                        dev_err(&led_dev->dev, "Failed to creating modalias attribute file\n");
                        goto err6;
                    }
                }
            }
        }
    }
    return 0;
err6:
    device_unregister(&led_dev->dev);
err5:
    kfree(led_dev);
err4:
    device_unregister(bus_dev);
err3:
    kfree(bus_dev);
err2:
    device_unregister(&ctrl_dev->dev);
err1:
    kfree(ctrl_dev);
err0:
    bus_unregister(&led_bus);
err:
    class_unregister(&hotplug_led_class);
    return -1;
}

module_init(led_bus_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("hotplug led bus");
