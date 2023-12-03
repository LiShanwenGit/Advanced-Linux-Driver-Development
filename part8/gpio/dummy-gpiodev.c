#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/kobject.h>   
#include <linux/sysfs.h>

struct gpio_desc *led_pin;

static ssize_t direct_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    if(strncmp(buf,"out",3) == 0) {
        gpiod_direction_output(led_pin, 0);
    }
    else if (strncmp(buf,"in",2) == 0) {
        gpiod_direction_input(led_pin);
    }
	return count;
}
static DEVICE_ATTR(direct, S_IWUSR, NULL, direct_store);

static ssize_t value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    if(strncmp(buf,"high",4) == 0) {
        gpiod_set_value(led_pin, 1);
    }
    else if (strncmp(buf,"low",3) == 0) {
        gpiod_set_value(led_pin, 0);
    }
	return count;
}
static DEVICE_ATTR(value, S_IWUSR, NULL, value_store);

static int dummy_gpioctrl_dev_probe(struct platform_device *pdev)
{   
    led_pin = gpiod_get(&pdev->dev, "led", GPIOF_OUT_INIT_LOW);
    if(IS_ERR(led_pin)) {
        dev_err(&pdev->dev, "fail to get led gpio\n");
        return -1;
    }
    gpiod_direction_output(led_pin, 0);
    device_create_file(&pdev->dev, &dev_attr_direct);
    device_create_file(&pdev->dev, &dev_attr_value);
    printk(KERN_INFO "probe dummy_gpioctrl_dev_probe\n");
	return 0;
}

static int dummy_gpioctrl_dev_remove(struct platform_device *pdev)
{
    gpiod_put(led_pin);
    //gpio_free(600);
    printk(KERN_INFO "remove dummy_gpioctrl_dev_remove\n");
	return 0;
}

static const struct of_device_id dummy_gpioctrl_dev_match[] = {
	{ .compatible = "foo-led-demo", },
	{}
};
MODULE_DEVICE_TABLE(of, dummy_gpioctrl_dev_match);

static struct platform_driver dummy_gpioctrl_dev_drv = {
	.probe	= dummy_gpioctrl_dev_probe,
	.remove	= dummy_gpioctrl_dev_remove,
	.driver	= {
		.name = "foo-led-demo",
		.of_match_table	= dummy_gpioctrl_dev_match,
	},
};
module_platform_driver(dummy_gpioctrl_dev_drv);

MODULE_AUTHOR("1477153217@qq.com");
MODULE_AUTHOR("Li shanwen");
MODULE_DESCRIPTION("dummy GPIO Controller Drivers Device Test");
MODULE_LICENSE("GPL");
