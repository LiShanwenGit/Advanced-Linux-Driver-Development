#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinconf.h>

/*下面是声明struct pinctrl_dev结构体，实际上在开发中将本文件放在drivers/pinctrl/xxx目录下即可，并不需要下面重新申明 */
struct pinctrl_dev {
	struct list_head node;
	struct pinctrl_desc *desc;
	struct radix_tree_root pin_desc_tree;
#ifdef CONFIG_GENERIC_PINCTRL_GROUPS
	struct radix_tree_root pin_group_tree;
	unsigned int num_groups;
#endif
#ifdef CONFIG_GENERIC_PINMUX_FUNCTIONS
	struct radix_tree_root pin_function_tree;
	unsigned int num_functions;
#endif
	struct list_head gpio_ranges;
	struct device *dev;
	struct module *owner;
	void *driver_data;
	struct pinctrl *p;
	struct pinctrl_state *hog_default;
	struct pinctrl_state *hog_sleep;
	struct mutex mutex;
#ifdef CONFIG_DEBUG_FS
	struct dentry *device_root;
#endif
};
/*############################################################################################ */

//两个寄存器
//GPIO_MUX0: offset 0x20
//GPIO_MUX1: offset 0x24
uint32_t *mux_reg_base = NULL;
#define  PIN2BANK(pin)           (pin>>4)
#define  PIN2BIT(pin)            (pin%16)

static const struct pinctrl_pin_desc dummy_pins[] = {
	PINCTRL_PIN(0, "GPIO_A0"),
	PINCTRL_PIN(1, "GPIO_A1"),
	PINCTRL_PIN(2, "GPIO_A2"),
	PINCTRL_PIN(3, "GPIO_A3"),
	PINCTRL_PIN(4, "GPIO_A4"),
	PINCTRL_PIN(5, "GPIO_A5"),
	PINCTRL_PIN(6, "GPIO_A6"),
	PINCTRL_PIN(7, "GPIO_A7"),
	PINCTRL_PIN(8, "GPIO_A8"),
	PINCTRL_PIN(9, "GPIO_A9"),
	PINCTRL_PIN(10, "GPIO_A10"),
	PINCTRL_PIN(11, "GPIO_A11"),
	PINCTRL_PIN(12, "GPIO_A12"),
	PINCTRL_PIN(13, "GPIO_A13"),
	PINCTRL_PIN(14, "GPIO_A14"),
	PINCTRL_PIN(15, "GPIO_A15"),
	PINCTRL_PIN(16, "GPIO_B0"),
	PINCTRL_PIN(17, "GPIO_B1"),
	PINCTRL_PIN(18, "GPIO_B2"),
	PINCTRL_PIN(19, "GPIO_B3"),
	PINCTRL_PIN(20, "GPIO_B4"),
	PINCTRL_PIN(21, "GPIO_B5"),
	PINCTRL_PIN(22, "GPIO_B6"),
	PINCTRL_PIN(23, "GPIO_B7"),
	PINCTRL_PIN(24, "GPIO_B8"),
	PINCTRL_PIN(25, "GPIO_B9"),
	PINCTRL_PIN(26, "GPIO_B10"),
	PINCTRL_PIN(27, "GPIO_B11"),
	PINCTRL_PIN(28, "GPIO_B12"),
	PINCTRL_PIN(29, "GPIO_B13"),
	PINCTRL_PIN(30, "GPIO_B14"),
	PINCTRL_PIN(31, "GPIO_B15"),
};

struct dummy_func_desc
{
    const char *name;
    const char * const *group_name;
    const int num_group_name;
	const int mux_select;
};

//只写了部分分组，其他没有使用的引脚全部当作GPIO
static const char * const uart_group[] = {"uart_group"}; //UART引脚分组名称
static const char * const i2c_group[]  = {"i2c_group"};  //I2C引脚分组名称
static const char * const spi_group[]  = {"spi_group"};  //SPI引脚分组名称
static const char * const usb_group[]  = {"usb_group"};  //SPI引脚分组名称

static const struct dummy_func_desc dummy_pinmux_functions[] = {
	{ .name = "uart_func", .group_name = uart_group, .num_group_name = ARRAY_SIZE(uart_group), .mux_select = 1},
   	{ .name = "i2c_func", .group_name =  i2c_group, .num_group_name = ARRAY_SIZE(i2c_group), .mux_select = 2},
	{ .name = "spi_func", .group_name =  spi_group, .num_group_name = ARRAY_SIZE(spi_group), .mux_select = 3}, 
	{ .name = "usb_group", .group_name =  usb_group, .num_group_name = ARRAY_SIZE(usb_group), .mux_select = 1}, 
};

static int dummy_get_groups_count(struct pinctrl_dev *pctldev)
{
	int count = ARRAY_SIZE(dummy_pins);
    printk(KERN_INFO "groups_count: %d\n", count);
	return ARRAY_SIZE(dummy_pins);
}

static const char *dummy_get_group_name(struct pinctrl_dev *pctldev,unsigned selector)
{
    printk(KERN_INFO "groups_name[%d]: %s\n", selector, dummy_pins[selector].name);
	return dummy_pins[selector].name;
}

static int dummy_get_group_pins(struct pinctrl_dev *pctldev, unsigned selector, const unsigned **pins,unsigned *num_pins)
{
	*pins = &dummy_pins[selector].number;
	*num_pins = 1;
	return 0;
}

static void dummy_pin_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s, unsigned int offset)
{
	seq_printf(s, " %s", dev_name(pctldev->dev));
}

static const struct pinctrl_ops dummy_pinctrl_ops =
{
	.get_groups_count = dummy_get_groups_count,
	.get_group_name = dummy_get_group_name, 
	.get_group_pins =  dummy_get_group_pins,
	.pin_dbg_show =  dummy_pin_dbg_show,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_all,
	.dt_free_map = pinconf_generic_dt_free_map,
};

static const char *dummy_get_function_name(struct pinctrl_dev *pctldev, unsigned selector)
{
    printk(KERN_INFO "function_name[%d]: %s\n", selector, dummy_pinmux_functions[selector].name);
	return dummy_pinmux_functions[selector].name;
}

static int dummy_get_functions_count(struct pinctrl_dev *pctldev)
{
    int count = ARRAY_SIZE(dummy_pinmux_functions);
	printk(KERN_INFO "functions_count=%d\n", count);
	return ARRAY_SIZE(dummy_pinmux_functions);
}

static int dummy_get_function_groups (struct pinctrl_dev *pctldev, unsigned selector, const char * const **groups, unsigned *num_groups)
{
	*groups = dummy_pinmux_functions[selector].group_name;
	*num_groups = dummy_pinmux_functions[selector].num_group_name;
	return 0;
}

static int dummy_set_mux(struct pinctrl_dev *pctldev, unsigned func_selector, unsigned group_selector)
{
	int pin = 0;
	//每个引脚对应一个分组对应
	pin = dummy_pins[group_selector].number;
	uint32_t value;
	value = *(mux_reg_base + PIN2BANK(pin));
	value &= ( ~(3 << (PIN2BIT(pin)*2)) );
	value |= (dummy_pinmux_functions[func_selector].mux_select << (PIN2BIT(pin)*2));
	*(mux_reg_base + PIN2BANK(pin))  = value;
	return 0;
}

static int dummy_gpio_request_enable (struct pinctrl_dev *pctldev, struct pinctrl_gpio_range *range, unsigned pin)
{
	//设置寄存器使能GPIO
	printk(KERN_INFO"pin[%d] enable\n", pin);
	return 0;
}

static void dummy_gpio_disable_free(struct pinctrl_dev *pctldev, struct pinctrl_gpio_range *range, unsigned offset)
{
	printk(KERN_INFO "dummy_gpio_disable_free\n");
}

static int dummy_free(struct pinctrl_dev *pctldev, unsigned offset)
{
	printk(KERN_INFO "dummy_free\n");
	return 0;
}

static int dummy_request(struct pinctrl_dev *pctldev, unsigned offset)
{
    printk(KERN_INFO "dummy_request\n");
	return 0;
}

static const struct pinmux_ops dummy_pinmux_ops = 
{
	.get_function_name = dummy_get_function_name,
	.get_functions_count = dummy_get_functions_count,
	.get_function_groups = dummy_get_function_groups,
	.set_mux = dummy_set_mux,
	.gpio_request_enable = dummy_gpio_request_enable,
	.gpio_disable_free = dummy_gpio_disable_free,
	.strict = true,
	.free = dummy_free,
	.request = dummy_request,
};

//用于记录所有引脚的配置状态
unsigned long dummy_pin_configs[ARRAY_SIZE(dummy_pins)] = {0};

static int dummy_pconf_get(struct pinctrl_dev *pctldev, unsigned pin, unsigned long *config)
{
	if(pin >= ARRAY_SIZE(dummy_pins))
		return -EINVAL;
	//实际中直接读取PULL寄存器的值
	*config = dummy_pin_configs[pin];
	return 0;
}

static int dummy_pconf_set(struct pinctrl_dev *pctldev, unsigned pin, unsigned long *configs, unsigned num_configs)
{
	if((num_configs != 1) || (pin >= ARRAY_SIZE(dummy_pins)))
		return -EINVAL;
	//实际中直接设置PULL寄存器的值
	dummy_pin_configs[pin] = *configs;
	printk(KERN_INFO "pin config %s as 0x%lx\n", dummy_pins[pin].name, *configs);
	return 0;
}

static void dummy_pconf_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s, unsigned pin)
{
	if(pin >= ARRAY_SIZE(dummy_pin_configs))
		return ;
	seq_printf(s, "0x%lx", dummy_pin_configs[pin]);
}

static const struct pinconf_ops dummy_pconf_ops = {
   .pin_config_get = dummy_pconf_get,
   .pin_config_set = dummy_pconf_set,
   //这里不提供组配置，只提供引脚配置
   //.pin_config_group_get = dummy_pconf_group_get,
   //.pin_config_group_set = dummy_pconf_group_set,
   .pin_config_dbg_show = dummy_pconf_dbg_show,
};

static ssize_t dummy_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("[GPIO_MUX0]: 0x%08x", *(mux_reg_base + 0));
	printk("[GPIO_MUX1]: 0x%08x", *(mux_reg_base + 1));
    return 0;
}
static DEVICE_ATTR(reg, S_IRUSR, dummy_reg_show, NULL);

static int dummy_pinctrl_probe(struct platform_device *pdev)
{
    struct pinctrl_dev *pinctrl_dev;
	struct pinctrl_desc *pctl_desc;
	pctl_desc = devm_kzalloc(&pdev->dev, sizeof(*pctl_desc), GFP_KERNEL);
	if (!pctl_desc)
		return -ENOMEM;

	pctl_desc->name = "dummy_pinctrl",
	pctl_desc->pins = dummy_pins,
	pctl_desc->npins = ARRAY_SIZE(dummy_pins),
	pctl_desc->pctlops = &dummy_pinctrl_ops,
	pctl_desc->pmxops = &dummy_pinmux_ops,
	pctl_desc->confops = &dummy_pconf_ops,
	pctl_desc->owner = THIS_MODULE,

	pinctrl_dev = devm_pinctrl_register(&pdev->dev, pctl_desc, NULL);
	if (IS_ERR(pinctrl_dev))
	{
        kfree(pctl_desc);
		pr_err("could not register dummy pinctrl driver\n");
		return -1;
	}
    printk(KERN_INFO "register dummy pinctrl driver successful\n");
	platform_set_drvdata(pdev, pinctrl_dev);
	mux_reg_base = devm_kzalloc(&pdev->dev, 8, GFP_KERNEL);
	if(!mux_reg_base) {
		devm_pinctrl_unregister(&pdev->dev, pinctrl_dev);
        kfree(pctl_desc);
		pr_err("could not alloc mux register\n");
		return -ENOMEM;
	}
	device_create_file(&pdev->dev, &dev_attr_reg);
	return 0;
}

static int dummy_pinctrl_remove(struct platform_device *pdev)
{
	struct pinctrl_dev *pinctrl_dev = platform_get_drvdata(pdev);
	device_remove_file(&pdev->dev, &dev_attr_reg);
	kfree(mux_reg_base);
	devm_pinctrl_unregister(&pdev->dev, pinctrl_dev);
    printk(KERN_INFO "unregister dummy pinctrl driver successful\n");
	return 0;
}

static const struct of_device_id dummy_pinctrl_match[] = {
	{ .compatible = "dummy-pinctrl-demo", },
	{}
};
MODULE_DEVICE_TABLE(of, dummy_pinctrl_match);

static struct platform_driver dummy_pinctrl_drv = {
    .driver = {
        .name = "dummy_pinctrl_drv",
		.of_match_table	= dummy_pinctrl_match,
    },
    .probe = dummy_pinctrl_probe,
    .remove = dummy_pinctrl_remove,
};

module_platform_driver(dummy_pinctrl_drv);

MODULE_DESCRIPTION("dummy Pinctrl Controller Drivers");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_LICENSE("GPL");
