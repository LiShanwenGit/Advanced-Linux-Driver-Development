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

static int probe(struct hotplug_led_dev *led_dev)
{
    led_dev->pin = gpiod_get(&led_dev->dev, "on_off", GPIOF_OUT_INIT_LOW);
    if(IS_ERR(led_dev->pin)) {
        dev_err(&led_dev->dev, "fail to get on_off gpio\n");
        return -1;
    }
    gpiod_direction_output(led_dev->pin, 0);
    return 0;
}

static void remove(struct hotplug_led_dev *led_dev)
{
    gpiod_put(led_dev->pin);
}

struct hotplug_led_drv led = {
    .match = "hotplug-led-demo",
    .probe = probe,
    .remove = remove,
    .driver		= {
		.name	= "hotplug_led_demo",
	},
};

static int __init led_device_init(void)
{
    led.driver.bus = &led_bus;
    led.driver.owner = THIS_MODULE;
    printk(KERN_INFO "led device init!\n");
    return driver_register(&led.driver);
}

static void __exit led_device_exit(void)
{
    driver_unregister(&led.driver);
    printk(KERN_INFO "led device exit!\n");
}

module_init(led_device_init);
module_exit(led_device_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("hotplug led device driver");
