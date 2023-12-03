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
#include <linux/irq.h>
#include <linux/interrupt.h>


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

extern struct bus_type led_bus;

static irqreturn_t check_irq_thread(int irq, void *p)
{
    int status;
    struct hotplug_led_dev *ctrl_dev = (struct hotplug_led_dev*)p;
    struct bus_type *bus = ctrl_dev->dev.bus;
    //根据总线查找设备，在实际中，这里需要与设备进行通信，获取设备的ID
    struct device *child_dev = bus_find_device_by_name(bus, NULL, "hotplug-led");
    printk(KERN_INFO "check device change\n");
    if(!child_dev)
    {
        printk(KERN_ERR "no device\n");
        return IRQ_HANDLED;
    }
    status = gpiod_get_value(ctrl_dev->pin);
    if(status) {
        //检测到设备拔出
        kobject_uevent(&child_dev->kobj, KOBJ_REMOVE);
    }
    else {
        //检测到设备插入
        kobject_uevent(&child_dev->kobj, KOBJ_ADD);
    }
    return IRQ_HANDLED;
}

static int probe(struct hotplug_led_dev *ctrl_dev)
{
    int ret;
    int check_irq;
    ctrl_dev->pin = gpiod_get(&ctrl_dev->dev, "check", GPIOF_OUT_INIT_LOW);
    if(IS_ERR(ctrl_dev->pin)) {
        dev_err(&ctrl_dev->dev, "fail to get check gpio\n");
        return -1;
    }
    check_irq = gpiod_to_irq(ctrl_dev->pin);
    //这里采用下半部分来发送热插拔消息，
    ret = request_threaded_irq(check_irq, NULL, check_irq_thread, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "hotplug_led", ctrl_dev);
    if (ret) {
        dev_err(&ctrl_dev->dev, "fail to request irq \n");
        gpiod_put(ctrl_dev->pin);
        return ret;
    }
    return 0;
}

static void remove(struct hotplug_led_dev *ctrl_dev)
{
    free_irq(gpiod_to_irq(ctrl_dev->pin), ctrl_dev);
    gpiod_put(ctrl_dev->pin);
}

struct hotplug_led_drv ctrl = {
    .match = "hotplug-led-ctl",
    .probe = probe,
    .remove = remove,
    .driver	= {
		.name	= "hotplug_led_ctrl",
	},
};

static int __init led_controller_init(void)
{

    ctrl.driver.bus = &led_bus;
    ctrl.driver.owner = THIS_MODULE;
    printk(KERN_INFO "led controller init!\n");
    return driver_register(&ctrl.driver);
}

static void __exit led_controller_exit(void)
{
    driver_unregister(&ctrl.driver);
    printk(KERN_INFO "led controller exit!\n");
}

module_init(led_controller_init);
module_exit(led_controller_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("hotplug led controller driver");
