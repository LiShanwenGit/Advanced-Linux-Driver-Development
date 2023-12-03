#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/component.h>

static int master_comp_bind(struct device *dev)
{
	int ret;
	ret = component_bind_all(dev, NULL);
	if (ret) {
		pr_info("master-comp: bind error\n");
		return -1;
	}
	pr_info("master-comp: bind\n");
	return 0;
}

static void master_comp_unbind(struct device *dev)
{
	component_unbind_all(dev, NULL);
	pr_info("master-comp: unbind\n");
}
static const struct component_master_ops master_comp_ops = {
	.bind   = master_comp_bind,
	.unbind = master_comp_unbind,
};

static int master_comp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct component_match *match = NULL;
	struct device_node *np = dev->of_node;
	struct device_node *port;
	int i;

	pr_info("master-comp: probe\n");

	for (i = 0;; i++) {
		//查询master-comp设备节点下的ports属性中phandle
		port = of_parse_phandle(np, "ports", i);
		if (!port)
			break;

		if (!of_device_is_available(port)) {
			continue;
		}

		component_match_add(dev, &match, component_compare_of, port);
	}

	if (i == 0) {
		dev_err(dev, "can not find any 'ports' property\n");
		return -ENODEV;
	}

	if (!match) {
		dev_err(dev, "can not find master-comp dt node\n");
		return -ENODEV;
	}

	return component_master_add_with_match(dev, &master_comp_ops, match);
}

static int master_comp_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	component_master_del(dev, &master_comp_ops);
	return 0;
}

static const struct of_device_id master_comp_ids[] = {
	{ .compatible = "master-comp", },
	{ },
};
MODULE_DEVICE_TABLE(of, master_comp_ids);

static struct platform_driver master_comp_driver = {
	.probe = master_comp_probe,
	.remove = master_comp_remove,
	.driver = {
		.name = "master-comp",
		.of_match_table = master_comp_ids,
	},
};

module_platform_driver(master_comp_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("component framwork test: master-comp");
