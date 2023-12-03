#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>       
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>   
#include <linux/mm.h>

struct dummy_test {
    struct device *dev;
    void          *buffer_addr;
    dev_t         dummy_dev_num;
    struct cdev   dummy_cdev;
    struct class  *dummy_class;
    struct device *dummy_dev;
    //...
};

static void dev_test_release(struct device *dev)
{

}

//作者这里增加一个设备仅仅是为了方便管理结构体
static struct device dev_mmap_test = {
    .init_name  = "mmap_test",
    .release    = dev_test_release,
};

static int dummy_open(struct inode *inode, struct file *filep)
{
    filep->private_data = container_of(inode->i_cdev, struct dummy_test, dummy_cdev);
    return 0;
}

static int dummy_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct dummy_test *demo = file->private_data;
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long phy;

	dev_dbg(demo->dev, "create mmap region\n");

    //合法性检查
	if ((size != PAGE_SIZE) || (offset & ~PAGE_MASK)) {
		dev_err(demo->dev, "invalid params for mmap region\n");
		return -EINVAL;
	}
	/* 获得物理地址 */
	phy = virt_to_phys(demo->buffer_addr);

	/* 是否使用 cache, buffer */
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	/* 开始创建映射内存区域 */
	if (remap_pfn_range(vma, start, phy >> PAGE_SHIFT, size, vma->vm_page_prot)) {
		printk(KERN_ERR "mmap: remap_pfn_range failed\n");
		return -ENOBUFS;
	}

	return 0;
}

static ssize_t dummy_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int ret;
    struct dummy_test *demo = file->private_data;
    ret = copy_to_user(buf, demo->buffer_addr, count&(PAGE_SIZE-1));
    if( ret != 0) {
    	return -EFAULT;
    }
    return count&(PAGE_SIZE-1);
}

static ssize_t dummy_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret;
    struct dummy_test *demo = file->private_data;
    ret = copy_from_user(demo->buffer_addr, buf, count&(PAGE_SIZE-1));
    if( ret != 0) {
    	return -EFAULT;
    }
    return count&(PAGE_SIZE-1);
}

static int dummy_close(struct inode *inode, struct file *filep)
{
    filep->private_data = NULL;
    return 0;
}

static const struct file_operations dummy_fops = {
	.owner		= THIS_MODULE,
	.open		= dummy_open,
    .read       = dummy_read,
    .write      = dummy_write,
    .mmap       = dummy_mmap,
	.release	= dummy_close,
};

static ssize_t mmap_data_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    struct dummy_test *demo = dev_get_drvdata(dev);
	return sprintf(buf, "%s", (char*)demo->buffer_addr);
}

static ssize_t mmap_data_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
    struct dummy_test *demo = dev_get_drvdata(dev);
	sprintf(demo->buffer_addr, "%s", buf);
	return size;
}
static DEVICE_ATTR(data, 0644, mmap_data_read, mmap_data_write);

static int __init dymmy_mmap_init(void)
{
    int ret;
    struct device *dev = &dev_mmap_test;
    struct dummy_test *demo = NULL;
    ret = device_register(&dev_mmap_test);
    if(ret)
    {
        printk(KERN_ERR "dev_mmap_test register error!\n");
        return ret;        
    }
    demo = kmalloc(sizeof(struct dummy_test), GFP_KERNEL);
    if (demo == NULL) {
        dev_err(dev, "alloc buffer failed!\n");
        return -1;
    }
    ret = alloc_chrdev_region(&demo->dummy_dev_num ,0, 1, "dummy");  //动态申请一个设备号
    if(ret !=0) {
        dev_err(dev, "alloc_chrdev_region failed!\n");
        return -1;
    }
    demo->dummy_cdev.owner = THIS_MODULE;
    cdev_init(&demo->dummy_cdev, &dummy_fops);
    cdev_add(&demo->dummy_cdev, demo->dummy_dev_num, 1);
    demo->dummy_class = class_create(THIS_MODULE, "dummy_class");
    if(demo->dummy_class == NULL) {
        dev_err(dev, "dummy_class failed!\n");
        return -1;
    }
    demo->dummy_dev = device_create(demo->dummy_class, NULL, demo->dummy_dev_num, NULL, "dummy");
    if(IS_ERR(demo->dummy_dev)) {
        dev_err(dev, "device_create failed!\n");
        return -1;
    }
    demo->buffer_addr = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if(demo->buffer_addr == NULL) {
        dev_err(dev, "alloc file buffer failed!\n");
        return -1;
    }
    dev_set_drvdata(dev, demo);
    demo->dev = dev;
    device_create_file(dev, &dev_attr_data);
    return 0;
}

static void __exit dymmy_mmap_exit(void)
{
    struct dummy_test *demo = dev_get_drvdata(&dev_mmap_test);
    cdev_del(&demo->dummy_cdev);
    unregister_chrdev_region(demo->dummy_dev_num, 1);
    device_destroy(demo->dummy_class, demo->dummy_dev_num);
    class_destroy(demo->dummy_class);
    kfree(demo);
    device_unregister(&dev_mmap_test);
}

module_init(dymmy_mmap_init);
module_exit(dymmy_mmap_exit);

MODULE_LICENSE("GPL");      
MODULE_AUTHOR("1477153217@qq.com");   
MODULE_VERSION("0.1");          
MODULE_DESCRIPTION("mmap test"); 
