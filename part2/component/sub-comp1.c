#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/component.h>

static int sub_comp1_bind(struct device *dev, struct device *master, void *data)
{
	pr_info("sub-comp1: bind\n");
	return 0;
}

static void sub_comp1_unbind(struct device *dev, struct device *master, void *data)
{
	pr_info("sub-comp1: unbind\n");
}

const struct component_ops sub_comp1_ops = {
	.bind   = sub_comp1_bind,
	.unbind = sub_comp1_unbind,
};

static int sub_comp1_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	pr_info("sub-comp1: probe\n");
	if (!dev->of_node) {
		dev_err(dev, "can't find sub-comp1 dt node\n");
		return -ENODEV;
	}
	return component_add(dev, &sub_comp1_ops);
}

static int sub_comp1_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	component_del(dev, &sub_comp1_ops);
	return 0;
}


static const struct of_device_id sub_comp1_ids[] = {
	{ .compatible = "sub-comp1", },
	{ },
};
MODULE_DEVICE_TABLE(of, sub_comp1_ids);

static struct platform_driver sub_comp1_driver = {
	.probe = sub_comp1_probe,
	.remove = sub_comp1_remove,
	.driver = {
		.name = "sub-comp1",
		.of_match_table = sub_comp1_ids,
	},
};

module_platform_driver(sub_comp1_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("component framwork test: sub-comp1");
