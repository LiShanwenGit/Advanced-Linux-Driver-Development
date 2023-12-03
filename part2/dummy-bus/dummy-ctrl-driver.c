#include <linux/irq.h>
#include <linux/interrupt.h>
#include "dummy.h"

extern struct bus_type dummy_bus;

static int probe(struct dummy_dev *ctrl_dev)
{
    ctrl_dev->pin1 = gpiod_get(&ctrl_dev->dev, "data1", GPIOF_OUT_INIT_LOW);
    if(IS_ERR(ctrl_dev->pin1)) {
        dev_err(&ctrl_dev->dev, "fail to get data1 gpio\n");
        return -1;
    }
    ctrl_dev->pin2 = gpiod_get(&ctrl_dev->dev, "data2", GPIOF_OUT_INIT_LOW);
    if(IS_ERR(ctrl_dev->pin2)) {
        dev_err(&ctrl_dev->dev, "fail to get data2 gpio\n");
        return -1;
    }
    return 0;
}

static void remove(struct dummy_dev *ctrl_dev)
{
    if(ctrl_dev->pin1)
        gpiod_put(ctrl_dev->pin1);
    if(ctrl_dev->pin2)
        gpiod_put(ctrl_dev->pin2);
}

static const struct of_device_id dummy_match[] = {
	{ .compatible = "dummy-demo-ctrl" },
	{}
};

struct dummy_drv ctrl = {
    .probe = probe,
    .remove = remove,
    .driver	= {
		.name	= "dummy_controller",
        .of_match_table = dummy_match,
	},
};

static int __init dummy_controller_init(void)
{

    ctrl.driver.bus = &dummy_bus;
    ctrl.driver.owner = THIS_MODULE;
    printk(KERN_INFO "dummy controller init!\n");
    return driver_register(&ctrl.driver);
}

static void __exit dummy_controller_exit(void)
{
    driver_unregister(&ctrl.driver);
    printk(KERN_INFO "dummy controller exit!\n");
}

module_init(dummy_controller_init);
module_exit(dummy_controller_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("dummy controller driver");
