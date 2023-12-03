#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>

/*
虚拟设备的GPIO总共有如下：
GPIO_A0 ~ GPIO_A15   BANK0
GPIO_B0 ~ GPIO_B15   BANK1
虚拟设备寄存器如下：
-----------------------------------------------------
 GPIO Register     Base  Address   reg_base 
-----------------------------------------------------
Register Name       Offset         Description
-----------------------------------------------------
GPIO_n_CFG          n*0xC+0x00    n=0, 1
GPIO_n_DAT          n*0xC+0x04    n=0, 1
GPIO_n_PUL          n*0xC+0x08    n=0, 1
-----------------------------------------------------
*/
#define  PIN2BANK(pin)      (pin>>4)
#define  PIN2BIT(pin)       (pin%16)

#define GPIO_CFG_OFFSET(pin)       (PIN2BANK(pin)*0xC+0x00)
#define GPIO_DAT_OFFSET(pin)       (PIN2BANK(pin)*0xC+0x04)
#define GPIO_PUL_OFFSET(pin)       (PIN2BANK(pin)*0xC+0x08)

struct dummy_gpio_chip
{
	struct gpio_chip chip;
	struct spinlock  lock;
	//寄存器基地址
	void *reg_base;
};

static int dummy_gpio_direction_input(struct gpio_chip *chip, unsigned pin_num)
{
	unsigned long flags;
	volatile uint32_t *pin_conf_reg;
	volatile uint32_t conf;
	struct dummy_gpio_chip *dummy_chip = container_of(chip, struct dummy_gpio_chip, chip);
	printk(KERN_INFO "[%d]: gpio_direction_input\n", pin_num);
	pin_conf_reg = (uint32_t*)(((uint8_t*)(dummy_chip->reg_base)) + GPIO_CFG_OFFSET(pin_num));
	spin_lock_irqsave(&dummy_chip->lock, flags);
	//conf = readl(pin_conf_reg);
	conf = *pin_conf_reg;
	conf &= ~(1<<(PIN2BIT(pin_num)));
	//writel(conf, pin_conf_reg);
	*pin_conf_reg = conf;
	spin_unlock_irqrestore(&dummy_chip->lock, flags);
	return 0;
}

static int dummy_gpio_direction_output(struct gpio_chip *chip, unsigned pin_num, int value)
{
	unsigned long flags;
	volatile uint32_t *pin_conf_reg;
	volatile uint32_t conf;
	struct dummy_gpio_chip *dummy_chip = container_of(chip, struct dummy_gpio_chip, chip);
	printk(KERN_INFO "[%d]: gpio_direction_output\n", pin_num);
	pin_conf_reg = (uint32_t*)(((uint8_t*)(dummy_chip->reg_base)) + GPIO_CFG_OFFSET(pin_num));
	spin_lock_irqsave(&dummy_chip->lock, flags);
	//conf = readl(pin_conf_reg);
	conf = *pin_conf_reg;
	conf |= (1<<(PIN2BIT(pin_num)));
	//writel(conf, pin_conf_reg);
	*pin_conf_reg = conf;
	spin_unlock_irqrestore(&dummy_chip->lock, flags);
	return 0;
}

static int dummy_gpio_get(struct gpio_chip *chip, unsigned pin_num)
{
	unsigned long flags;
	volatile uint32_t *pin_data_reg;
	volatile uint32_t data;
	struct dummy_gpio_chip *dummy_chip = container_of(chip, struct dummy_gpio_chip, chip);
	printk(KERN_INFO "[%d]: gpio_get\n", pin_num);
	pin_data_reg = (uint32_t*)(((uint8_t*)(dummy_chip->reg_base)) + GPIO_DAT_OFFSET(pin_num));
	spin_lock_irqsave(&dummy_chip->lock, flags);
	//data = readl(pin_data_reg);
	data = *pin_data_reg;
	spin_unlock_irqrestore(&dummy_chip->lock, flags);
	if(data >> PIN2BIT(pin_num))
		return 1;
	else
		return 0;
}

static void dummy_gpio_set(struct gpio_chip *chip, unsigned pin_num, int value)
{
	unsigned long flags;
	volatile uint32_t *pin_data_reg;
	volatile uint32_t data;
	struct dummy_gpio_chip *dummy_chip = container_of(chip, struct dummy_gpio_chip, chip);
	printk(KERN_INFO "[%d]: gpio_set\n", pin_num);
	pin_data_reg = (uint32_t*)(((uint8_t*)(dummy_chip->reg_base)) + GPIO_DAT_OFFSET(pin_num));
	spin_lock_irqsave(&dummy_chip->lock, flags);
	//data = readl(pin_data_reg);
	data = *pin_data_reg;
	if(value)
		data |= (1 << PIN2BIT(pin_num));
	else
		data &= ~(1 << PIN2BIT(pin_num));
	//writel(data, pin_data_reg);
	*pin_data_reg = data;
	spin_unlock_irqrestore(&dummy_chip->lock, flags);
}

static int dummy_gpio_to_irq(struct gpio_chip *chip, unsigned pin_num)
{
	printk(KERN_INFO "Not support, [%d]: gpio_to_irq\n", pin_num);
	return -1;
}

int dummy_gpio_request(struct gpio_chip *chip, unsigned int pin_num)
{
	printk(KERN_INFO "[%d]: gpio_request\n", pin_num);
	//gpiochip_generic_request(chip, pin_num);
	return 0;
}

static int dummy_pinctrl_gpio_of_xlate(struct gpio_chip *chip, const struct of_phandle_args *gpiospec, u32 *flags)
{
	int pin, base;
	base = 16 * gpiospec->args[0];
	pin = base + gpiospec->args[1];
	if (pin > chip->ngpio)
		return -EINVAL;
	//if (flags)//逻辑电平，不使用
	//	*flags = gpiospec->args[2];
	return pin;
}

static ssize_t dummy_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dummy_gpio_chip *dummy_chip = dev_get_drvdata(dev);
	uint32_t *reg_addr = (uint32_t*)(((uint8_t*)(dummy_chip->reg_base)));
	printk("[GPIO_A_CFG]: 0x%08x", *(reg_addr + 0));
	printk("[GPIO_A_DAT]: 0x%08x", *(reg_addr + 1));
	printk("[GPIO_A_PUL]: 0x%08x", *(reg_addr + 2));
	printk("[GPIO_B_CFG]: 0x%08x", *(reg_addr + 3));
	printk("[GPIO_B_DAT]: 0x%08x", *(reg_addr + 4));
	printk("[GPIO_B_PUL]: 0x%08x\n", *(reg_addr + 5));
    return 0;
}
static DEVICE_ATTR(reg, S_IRUSR, dummy_reg_show, NULL);

static int dummy_gpioctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dummy_gpio_chip *dummy_chip = NULL;
	int ret = 0;
	dummy_chip = devm_kzalloc(dev, sizeof(struct dummy_gpio_chip), GFP_KERNEL);
	if (dummy_chip ==  NULL)
		return -ENOMEM;
	//总共有6个寄存器，每个寄存器32位，因此这里分配24字节
	dummy_chip->reg_base = devm_kzalloc(dev, 24, GFP_KERNEL);
	if (dummy_chip->reg_base ==  NULL) {
		devm_kfree(dev, dummy_chip);
		return -ENOMEM;
	}
	dummy_chip->chip.direction_input = dummy_gpio_direction_input;
	dummy_chip->chip.direction_output = dummy_gpio_direction_output;
	dummy_chip->chip.get = dummy_gpio_get;
	dummy_chip->chip.set = dummy_gpio_set;
	dummy_chip->chip.owner = THIS_MODULE;
	dummy_chip->chip.request = dummy_gpio_request;
	dummy_chip->chip.to_irq = dummy_gpio_to_irq;
	dummy_chip->chip.free = gpiochip_generic_free;
	dummy_chip->chip.set_config = gpiochip_generic_config;
	dummy_chip->chip.ngpio  = 32; //32 (GPIO_A0...GPIO_A15 ~ GPIO_B0...GPIO_B15)
	dummy_chip->chip.base = 400; //0~383已经被使用了
	dummy_chip->chip.of_gpio_n_cells = 2;
	dummy_chip->chip.of_xlate = dummy_pinctrl_gpio_of_xlate;
	dummy_chip->chip.label = dev_name(dev);
	dummy_chip->chip.parent = dev;
	dummy_chip->chip.owner = THIS_MODULE;
	spin_lock_init(&dummy_chip->lock);
	ret = devm_gpiochip_add_data(dev, &dummy_chip->chip, dummy_chip);
	if (ret) 
	{
		devm_kfree(dev, dummy_chip->reg_base);
		devm_kfree(dev, dummy_chip);
		printk(KERN_ERR "gpiochip_add failed\n");
		return ret;
	}
	platform_set_drvdata(pdev, dummy_chip);
	device_create_file(dev, &dev_attr_reg);
	printk(KERN_INFO "register dummy gpioctrl successful\n");
	return 0;
}

static const struct of_device_id dummy_gpioctrl_match[] = {
	{ .compatible = "dummy-gpioctrl" },
	{ }
};
MODULE_DEVICE_TABLE(of, dummy_gpioctrl_match);

static struct platform_driver dummy_gpioctrl_driver = {
    .driver = {
        .name = "dummy-gpioctrl",
		.of_match_table = dummy_gpioctrl_match,
    },
    .probe = dummy_gpioctrl_probe,
};

module_platform_driver(dummy_gpioctrl_driver);

MODULE_DESCRIPTION("dummy GPIO Controller Drivers");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_LICENSE("GPL");