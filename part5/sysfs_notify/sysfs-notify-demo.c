#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#define PIN_NUM    71 //PC7 = 32*2+7

static int ev_press=0;
static struct kobject *notify_obj;

static ssize_t notify_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    ssize_t ret = sprintf(buf, "%d\n", ev_press);
    ev_press = 0; //按键放开
    return ret;
}

static struct kobj_attribute notify_attr = __ATTR(notify, 0664, notify_show, NULL);

static irqreturn_t key_irq(int irq, void *args)
{
    return IRQ_WAKE_THREAD;
}

static irqreturn_t key_irq_thread(int irq, void *args)
{
    ev_press = 1; //按下按键
    sysfs_notify(notify_obj, NULL, notify_attr.attr.name);
    return IRQ_HANDLED;
}

static int __init sysfs_notify_demo_init(void)
{
    int ret, irq;
    notify_obj = kobject_create_and_add("sysfs_notify", NULL);
    if(notify_obj == NULL)
    {
        printk(KERN_INFO"create notify_obj failed!\n");
        return -1;
    }
    ret = sysfs_create_file(notify_obj, &notify_attr.attr);
    if(ret != 0)
    {
        kobject_put(notify_obj);
        printk(KERN_INFO"create notify sysfs file failed!\n");
        return -1;
    }
	if (!gpio_is_valid(PIN_NUM)) {
        printk(KERN_ERR "gpio is invalid\n");
		return -EPROBE_DEFER;
    }
    ret = gpio_request(PIN_NUM, "key");
    if(ret) {
        printk(KERN_ERR "gpio request failed\n");
		return -1;
    }
    irq = gpio_to_irq(PIN_NUM);
    if (irq < 0) {
        gpio_free(PIN_NUM);
		printk(KERN_ERR "get gpio irq failed\n");
        return -1;
    }
    ret = request_threaded_irq(irq, key_irq, key_irq_thread, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "key", &ev_press);
    if(ret) {
        gpio_free(PIN_NUM);
        printk(KERN_ERR "request irq failed\n");
        return -1;
    }
    return 0;
}

static void __exit sysfs_notify_demo_exit(void)
{
    gpio_free(PIN_NUM);
    free_irq(gpio_to_irq(PIN_NUM), &ev_press);
    sysfs_remove_file(notify_obj, &notify_attr.attr);
    kobject_put(notify_obj);
}

module_init(sysfs_notify_demo_init);
module_exit(sysfs_notify_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("sysfs notify test");
