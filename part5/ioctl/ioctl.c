#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>

#define IOCTL_READ   0
#define IOCTL_WRITE  1

struct ioctl_test {
    dev_t          dev_num;
    int            data;
    struct cdev    cdev;
    struct class  *class;
    struct device *dev;
};

static struct ioctl_test *test;

static ssize_t test_read(struct file * file, char __user * userbuf, size_t count, loff_t * off)
{   
    if(!copy_to_user(userbuf, &test->data, 4)) {
        return 4;
    }
    else {
        return 0;
    }
}

static long test_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    switch (cmd)
    {
    case IOCTL_READ:
        return copy_to_user(argp, &test->data, 4);
    case IOCTL_WRITE:
        return copy_from_user(&test->data, argp, 4);
    default:
        return -EINVAL;
    }
    return 0;
}

static int test_open (struct inode * inode, struct file * file)
{
    return 0;
}

static int test_close(struct inode * inode, struct file * file)
{
    return 0;
}

static struct file_operations test_ops = {
    .owner   = THIS_MODULE,
    .open    = test_open,
    .read    = test_read,
    .unlocked_ioctl = test_ioctl,
    .release = test_close,
};

static int __init ioctl_init(void)
{
    int ret;
    test = kzalloc(sizeof(struct ioctl_test), GFP_KERNEL);
    if(test == NULL) {
        printk(KERN_ERR "struct ioctl_test alloc failed\n");
        return -ENOMEM;;
    }
    ret = alloc_chrdev_region(&test->dev_num ,0, 1, "test");  //动态申请一个设备号
    if(ret !=0) {
        printk(KERN_ERR "alloc_chrdev_region failed!\n");
        goto err1;
    }
    test->cdev.owner = THIS_MODULE;
    cdev_init(&test->cdev, &test_ops);
    cdev_add(&test->cdev, test->dev_num, 1);
    test->class = class_create(THIS_MODULE, "test_class");
    if(test->class == NULL) {
        printk(KERN_ERR "test_class failed!\n");
        goto err1;
    }
    test->dev = device_create(test->class, NULL, test->dev_num, NULL, "test");
    if(IS_ERR(test->dev)) {
        printk(KERN_ERR "device_create failed!\n");
        goto err2;
    }
    return ret;
err2:
    class_destroy(test->class);
err1:
    unregister_chrdev_region(test->dev_num, 1);
    kfree(test);
    return -1;
}

static void __exit ioctl_exit(void)
{
    device_destroy(test->class, test->dev_num);
    class_destroy(test->class);
    unregister_chrdev_region(test->dev_num, 1);
    kfree(test);
}

module_init(ioctl_init);
module_exit(ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("ioctl IO test");
