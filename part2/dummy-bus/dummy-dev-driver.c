#include <linux/irq.h>
#include <linux/interrupt.h>
#include "dummy.h"

extern struct bus_type dummy_bus;


static int probe(struct dummy_dev *dev)
{
    dev->pin1 = gpiod_get(&dev->dev, "led", GPIOF_OUT_INIT_LOW);
    if(IS_ERR(dev->pin1)) {
        dev_err(&dev->dev, "fail to get led gpio\n");
        return -1;
    }
    return 0;
}

static void remove(struct dummy_dev *dev)
{
    if(dev->pin1)
        gpiod_put(dev->pin1);
    if(dev->pin2)
        gpiod_put(dev->pin2);
}

static const struct of_device_id dummy_match[] = {
	{ .compatible = "dummy-demo-dev" },
	{}
};

struct dummy_drv dev = {
    .probe = probe,
    .remove = remove,
    .driver	= {
		.name	= "dummy_device",
        .of_match_table = dummy_match,
	},
};

static int __init dummy_device_init(void)
{

    dev.driver.bus = &dummy_bus;
    dev.driver.owner = THIS_MODULE;
    printk(KERN_INFO "dummy device init!\n");
    return driver_register(&dev.driver);
}

static void __exit dummy_device_exit(void)
{
    driver_unregister(&dev.driver);
    printk(KERN_INFO "dummy device exit!\n");
}

module_init(dummy_device_init);
module_exit(dummy_device_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("dummy device driver");
