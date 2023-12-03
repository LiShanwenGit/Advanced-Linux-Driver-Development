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
#include <linux/poll.h>

#define PIN_NUM    71 //PC7 = 32*2+7

struct gpio_key {
    dev_t          dev_num;
    struct cdev    cdev;
    struct class  *class;
    struct device *dev;
    struct tasklet_struct tasklet;
};

static struct gpio_key *key;

static irqreturn_t key_irq(int irq, void *args)
{
    tasklet_schedule(&key->tasklet);
    return IRQ_HANDLED;
}

static int key_open (struct inode * inode, struct file * file)
{
    return 0;
}

static int key_close(struct inode * inode, struct file * file)
{
    return 0;
}

static struct file_operations key_ops = {
    .owner   = THIS_MODULE,
    .open    = key_open,
    .release = key_close,
};

static void tasklet_handler(struct tasklet_struct *t)
{
    printk(KERN_INFO "tasklet demo!\n");
}

static int __init async_init(void)
{
    int ret, irq;
    key = kzalloc(sizeof(struct gpio_key), GFP_KERNEL);
    if(key == NULL) {
        printk(KERN_ERR "struct gpio_key alloc failed\n");
        return -ENOMEM;;
    }
    tasklet_setup(&key->tasklet, tasklet_handler);
	if (!gpio_is_valid(PIN_NUM)) {
        kfree(key);
        printk(KERN_ERR "gpio is invalid\n");
		return -EPROBE_DEFER;
    }
    ret = gpio_request(PIN_NUM, "key");
    if(ret) {
        kfree(key);
        printk(KERN_ERR "gpio request failed\n");
		return ret;
    }
    irq = gpio_to_irq(PIN_NUM);
    if (irq < 0) {
		printk(KERN_ERR "get gpio irq failed\n");
        goto err;
    }
    ret = request_irq(irq, key_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "key", key);
    if(ret) {
        printk(KERN_ERR "request irq failed\n");
        goto err;
    }
    ret = alloc_chrdev_region(&key->dev_num ,0, 1, "key");  //动态申请一个设备号
    if(ret !=0) {
        unregister_chrdev_region(key->dev_num, 1);
        printk(KERN_ERR "alloc_chrdev_region failed!\n");
        return -1;
    }
    key->cdev.owner = THIS_MODULE;
    cdev_init(&key->cdev, &key_ops);
    cdev_add(&key->cdev, key->dev_num, 1);
    key->class = class_create(THIS_MODULE, "key_class");
    if(key->class == NULL) {
        printk(KERN_ERR "key_class failed!\n");
        goto err1;
    }
    key->dev = device_create(key->class, NULL, key->dev_num, NULL, "key");
    if(IS_ERR(key->dev)) {
        printk(KERN_ERR "device_create failed!\n");
        goto err2;
    }
    return ret;
err2:
    class_destroy(key->class);
err1:
    unregister_chrdev_region(key->dev_num, 1);
err:
    gpio_free(PIN_NUM);
    kfree(key);
    return -1;
}

static void __exit async_exit(void)
{
    //停止tasklet任务
    tasklet_disable(&key->tasklet);
    // 清理tasklet相关资源
    tasklet_kill(&key->tasklet);
    gpio_free(PIN_NUM);
    device_destroy(key->class, key->dev_num);
    class_destroy(key->class);
    unregister_chrdev_region(key->dev_num, 1);
    free_irq(gpio_to_irq(PIN_NUM), key);
    kfree(key);
}

module_init(async_init);
module_exit(async_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("async notify test");
