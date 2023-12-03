#include "dummy.h"

static int dummy_bus_match(struct device *dev, struct device_driver *drv)
{
    const char *device_match = NULL;
    int ret;
    printk(KERN_INFO "is matched %s \n", __func__);
    ret = of_property_read_string(dev->of_node, "dummy-compatible", &device_match);
    if(ret && drv->of_match_table && drv->of_match_table->compatible)
    {
        return 0;//直接返回0，匹配失败，原因：没有找到关键字属性且驱动没有of_match_table结构体
    }
    return (strcmp(device_match, drv->of_match_table->compatible) == 0);
}

static int dummy_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    printk(KERN_INFO "%s hotplug\n", dev_name(dev));
    return add_uevent_var(env, "MODALIAS=%s", dev_name(dev));
}

static int dummy_bus_probe(struct device *dev)
{
    struct dummy_dev *device =  container_of(dev, struct dummy_dev, dev);
    struct dummy_drv *driver =  container_of(dev->driver, struct dummy_drv, driver);
	int ret = driver->probe(device);
    return ret;
}

static void dummy_bus_remove(struct device *dev)
{
    struct dummy_dev *device =  container_of(dev, struct dummy_dev, dev);
    struct dummy_drv *driver =  container_of(dev->driver, struct dummy_drv, driver);
	driver->remove(device);
}

struct bus_type dummy_bus = 
{
    .name  = "dummy_bus",
    .match = dummy_bus_match,
    .uevent= dummy_bus_uevent,
    .probe = dummy_bus_probe,
    .remove= dummy_bus_remove,
};
EXPORT_SYMBOL(dummy_bus);

static void dummy_device_release(struct device *dev)
{

}

static ssize_t dummy_modalias_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "%s\n", dev_name(dev));
}
static DEVICE_ATTR(modalias, S_IRUSR, dummy_modalias_show, NULL);

struct class dummy_class = {
	.name		= "dummy_class",
	.owner		= THIS_MODULE,
};

static int __init dummy_bus_init(void)
{
    int ret = 0;
    struct device_node *root, *ctrl, *child;
    struct property *prop;
    const char *match = NULL;
    struct dummy_dev *ctrl_dev, *child_dev;
    struct device *bus_dev;
    int dev_index = 0;
    char *bus_dev_name;
    printk(KERN_INFO "dummy bus init!\n");
    ret = class_register(&dummy_class);
    if(ret)
    {
        printk(KERN_ERR "dummy class register error!\n");
        return ret;
    }
    ret = bus_register(&dummy_bus);
    if (ret) {
        printk(KERN_ERR "dummy bus register error!\n");
        goto err;
    }
    root = of_find_node_by_path("/"); //找到设备树的根节点
    //这里我们去搜索根节点得子节点，如果存在matching属性，则进行注册控制器设备
    for_each_child_of_node(root, ctrl){
        prop = of_find_property(ctrl, "dummy-compatible", NULL); //判断是否存在该属性的节点
        if(prop)
        {
            ret = of_property_read_string(ctrl, "dummy-compatible", &match);
            if(ret)
            {
                printk(KERN_ERR "get dummy device property failed!\n");
                goto err0;
            }
            ctrl_dev = kzalloc(sizeof(struct dummy_dev), GFP_KERNEL);
            if (!ctrl_dev) {
                printk(KERN_ERR "alloc ctrl_dev failed!\n");
                goto err0;
            }
            ctrl_dev->dev.init_name = ctrl->name;
            ctrl_dev->dev.of_node = ctrl;
            ctrl_dev->dev.bus = &dummy_bus; 
            ctrl_dev->dev.release = dummy_device_release;
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
            bus_dev->release = dummy_device_release;
            bus_dev->class = &dummy_class;
            bus_dev->parent = &ctrl_dev->dev;
            ret = device_register(bus_dev);
            if(ret) {
                printk(KERN_ERR "%s bus_dev register error!\n", child->name);
                goto err3;
            }
            child = of_get_next_child(ctrl, NULL); //查找该节点的子节点，去注册子设备
            if(child) {
                //如果存在dummy-compatible属性，则进行注册子设备
                ret = of_property_read_string(child, "dummy-compatible", &match); 
                if(ret) {
                    printk(KERN_ERR "get %s device property failed!\n", child->name);
                    goto err4;
                }
                child_dev = kzalloc(sizeof(struct dummy_dev), GFP_KERNEL);
                if (!child_dev) {
                    return -ENOMEM;
                }
                child_dev->dev.init_name = child->name;
                child_dev->dev.of_node = child;
                child_dev->dev.bus = &dummy_bus;
                child_dev->dev.parent = bus_dev;
                child_dev->dev.release = dummy_device_release;
                prop = of_find_property(child, "removable", NULL); //判断是否存在该属性
                if(prop) {
                    child_dev->dev.removable = DEVICE_REMOVABLE;
                }
                ret = device_register(&child_dev->dev);
                if(ret) {
                    printk(KERN_ERR "%s device register error!\n", child->name);
                    goto err5;
                }
                else {
                    printk(KERN_INFO "%s device register successful!\n", child->name);
                    ret = device_create_file(&child_dev->dev, &dev_attr_modalias);
                    if (unlikely(ret)) {
                        dev_err(&child_dev->dev, "Failed to creating modalias attribute file\n");
                        goto err6;
                    }
                }
            }
        }
    }
    return 0;
err6:
    device_unregister(&child_dev->dev);
err5:
    kfree(child_dev);
err4:
    device_unregister(bus_dev);
err3:
    kfree(bus_dev);
err2:
    device_unregister(&ctrl_dev->dev);
err1:
    kfree(ctrl_dev);
err0:
    bus_unregister(&dummy_bus);
err:
    class_unregister(&dummy_class);
    return -1;
}

module_init(dummy_bus_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("dummy bus demo");
