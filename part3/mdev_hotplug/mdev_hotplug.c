#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>       
#include <asm/io.h>         
#include <linux/device.h> 
#include <linux/cdev.h>
#include <linux/platform_device.h> 
#include <linux/of.h>      
#include <linux/kobject.h>   
#include <linux/sysfs.h>    
#include <linux/slab.h>
#include <linux/string.h>

static dev_t hpsim_dev_num;      
static struct cdev *hpsim_dev;   
static struct class *hpsim_class; 
static struct device *hpsim;    

static ssize_t hpsim_add(struct device *dev,struct device_attribute *attr,char *buf)
{
    kobject_uevent(&hpsim->kobj, KOBJ_ADD);
    return sprintf(buf,"hotplug add\n");
}

static DEVICE_ATTR(add, S_IRUGO, hpsim_add, NULL);

static ssize_t hpsim_remove(struct device *dev,struct device_attribute *attr,char *buf)
{
    kobject_uevent(&hpsim->kobj, KOBJ_REMOVE);
    return sprintf(buf,"hotplug remove\n");
}

static DEVICE_ATTR(remove, S_IRUGO, hpsim_remove, NULL);

static struct file_operations hpsim_ops = {
    .owner = THIS_MODULE,
};

static int __init hpsim_init(void)
{
    int ret;
    hpsim_dev = cdev_alloc();  
    if(hpsim_dev == NULL)
    {
        printk(KERN_ERR"cdev_alloc failed!\n");
        return -1;
    }
    ret = alloc_chrdev_region(&hpsim_dev_num,0,1,"hpsim"); 
    if(ret !=0)
    {
        printk(KERN_ERR"alloc_chrdev_region failed!\n");
        return -1;
    }
    hpsim_dev->owner = THIS_MODULE;
    hpsim_dev->ops = &hpsim_ops;     
    cdev_add(hpsim_dev,hpsim_dev_num,1); 
    hpsim_class = class_create(THIS_MODULE, "hpsim_class");
    if(hpsim_class == NULL)
    {
        printk(KERN_ERR"hpsim_class failed!\n");
        return -1;
    }

    hpsim = device_create(hpsim_class,NULL,hpsim_dev_num,NULL,"hpsim");  
    if(IS_ERR(hpsim))
    {
        printk(KERN_ERR"device_create failed!\n");
        return -1;
    }
    ret = device_create_file(hpsim,&dev_attr_add);
    if(ret != 0)
    {
        printk(KERN_ERR"create attribute file failed!\n");
        return -1;
    }
    ret = device_create_file(hpsim,&dev_attr_remove);
    if(ret != 0)
    {
        printk(KERN_ERR"create attribute file failed!\n");
        return -1;
    }
    return 0;
//注意：这里还需要添加错误处理代码
}

static void __exit hpsim_exit(void)
{
    device_remove_file(hpsim,&dev_attr_add);  
    device_remove_file(hpsim,&dev_attr_remove); 
    cdev_del(hpsim_dev);  
    unregister_chrdev_region(hpsim_dev_num,1); 
    device_destroy(hpsim_class,hpsim_dev_num);  
    class_destroy(hpsim_class);   
}

module_init(hpsim_init);
module_exit(hpsim_exit);

MODULE_LICENSE("GPL");        
MODULE_AUTHOR("1477153217@qq.com");  
MODULE_VERSION("0.1");      
MODULE_DESCRIPTION("hotplug sim"); 
