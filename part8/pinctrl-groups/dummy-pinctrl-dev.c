#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>


static int dummy_pinctrl_dev_probe(struct platform_device *pdev)
{   
    printk(KERN_INFO "register dummy_pinctrl_dev\n");
	return 0;
}

static int dummy_pinctrl_dev_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "unregister dummy_pinctrl_dev\n");
	return 0;
}

static const struct of_device_id dummy_pinctrl_dev_match[] = {
	{ .compatible = "foo-uart-demo", },
	{}
};
MODULE_DEVICE_TABLE(of, dummy_pinctrl_dev_match);

static struct platform_driver dummy_pinctrl_dev_drv = {
	.probe	= dummy_pinctrl_dev_probe,
	.remove	= dummy_pinctrl_dev_remove,
	.driver	= {
		.name = "foo-uart-demo",
		.of_match_table	= dummy_pinctrl_dev_match,
	},
};
module_platform_driver(dummy_pinctrl_dev_drv);

MODULE_AUTHOR("1477153217@qq.com");
MODULE_AUTHOR("Li shanwen");
MODULE_DESCRIPTION("dummy pinctrl device driver");
MODULE_LICENSE("GPL");
