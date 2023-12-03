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

struct gpio_key {
    int ev_press;
    wait_queue_head_t wait_head;
    dev_t          dev_num;
    struct cdev    cdev;
    struct class  *class;
    struct device *dev;
};

static struct gpio_key *key;

static irqreturn_t key_irq(int irq, void *args)
{
    wake_up_interruptible(&key->wait_head);   /* 唤醒休眠的进程，此时将继续执行read函数 */
    key->ev_press = 1; //按下按键
    return IRQ_HANDLED;
}

static ssize_t key_read(struct file * file, char __user * userbuf, size_t count, loff_t * off)
{   int ret;
    //当key->ev_press==0时，即没有按键按下，此时进入睡眠
    wait_event_interruptible(key->wait_head, key->ev_press!=0);
    //如果程序执行此处，说明被唤醒
    key->ev_press = 0;
    ret = copy_to_user(userbuf, &key->ev_press, 4);
    if(ret){
        return ret;
    }
    return 4;
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
    .read    = key_read,
    .release = key_close,
};

static int __init blockIO_init(void)
{
    int ret, irq;
    key = kzalloc(sizeof(struct gpio_key), GFP_KERNEL);
    if(key == NULL) {
        printk(KERN_ERR "struct gpio_key alloc failed\n");
        return -ENOMEM;;
    }
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
    init_waitqueue_head(&key->wait_head);
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

static void __exit blockIO_exit(void)
{
    gpio_free(PIN_NUM);
    device_destroy(key->class, key->dev_num);
    class_destroy(key->class);
    unregister_chrdev_region(key->dev_num, 1);
    free_irq(gpio_to_irq(PIN_NUM), key);
    kfree(key);
}

module_init(blockIO_init);
module_exit(blockIO_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("block IO test");
