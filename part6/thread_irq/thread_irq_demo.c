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
#include <linux/wait.h>

#define PIN_NUM    71 //PC7 = 32*2+7

static int ev_press=0;

static irqreturn_t key_irq(int irq, void *args)
{
    return IRQ_WAKE_THREAD;
}

static irqreturn_t key_irq_thread(int irq, void *args)
{
    ev_press = 1; //按下按键
    printk(KERN_INFO "key press!\n");
    return IRQ_HANDLED;
}

static int __init thread_irq_init(void)
{
    int ret, irq;
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

static void __exit thread_irq_exit(void)
{
    gpio_free(PIN_NUM);
    free_irq(gpio_to_irq(PIN_NUM), &ev_press);
}

module_init(thread_irq_init);
module_exit(thread_irq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("thread irq test");
