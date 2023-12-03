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

#ifndef __DUMMY_H__
#define __DUMMY_H__

struct dummy_dev
{
    struct device dev;
    //下面需要保存GPIO
    struct gpio_desc *pin1;
    struct gpio_desc *pin2;
};

struct dummy_drv 
{
    struct device_driver driver;
    int (*probe) (struct dummy_dev *dev);
	void (*remove) (struct dummy_dev *dev);
};

#endif // !__DUMMY_H__