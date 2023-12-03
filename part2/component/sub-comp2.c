#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/component.h>

static int sub2_bind(struct device *dev, struct device *master, void *data)
{
	pr_info("sub-comp2: bind\n");
	return 0;
}

static void sub2_unbind(struct device *dev, struct device *master, void *data)
{
	pr_info("sub-comp2: unbind\n");
}

const struct component_ops sub_comp2_ops = {
	.bind = sub2_bind,
	.unbind = sub2_unbind,
};

static int sub_comp2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pr_info("sub-comp2: probe\n");
	if (!dev->of_node) {
		dev_err(dev, "can't find sub-comp2 dt node\n");
		return -ENODEV;
	}
	return component_add(dev, &sub_comp2_ops);
}

static int sub_comp2_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	component_del(dev, &sub_comp2_ops);
	return 0;
}

static const struct of_device_id sub_comp2_ids[] = {
	{ .compatible = "sub-comp2", },
	{ },
};
MODULE_DEVICE_TABLE(of, sub_comp2_ids);

static struct platform_driver sub_comp2_driver = {
	.probe = sub_comp2_probe,
	.remove = sub_comp2_remove,
	.driver = {
		.name = "testsub2-drm",
		.of_match_table = sub_comp2_ids,
	},
};

module_platform_driver(sub_comp2_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("component framwork test: sub-comp2");
