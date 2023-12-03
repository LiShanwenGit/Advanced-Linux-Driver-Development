#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/sysfs.h> 
#include <linux/slab.h>


struct spi_device *spi_dev;     //保存spi设备结构体

static ssize_t data_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    spi_read(spi_dev, buf, 4);//这里只读取四个字节
    return 4; 
}

static ssize_t data_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    spi_write(spi_dev, buf, count);
    return count;
}

static DEVICE_ATTR(data, 0660, data_show, data_store); //定义文件属性


static int dummy_spidev_probe(struct spi_device *spi)
{
    int ret;
    ret = device_create_file(&spi->dev,&dev_attr_data);//创建属性文件
    if(ret != 0)
    {
        printk(KERN_INFO "create data file failed!\n");
        return -1;
    }
    printk(KERN_INFO "register spi device successful!\n");
    spi_dev = spi_dev_get(spi);
    return 0;
}

static void dummy_spidev_remove(struct spi_device *spi)
{
    device_remove_file(&spi->dev,&dev_attr_data);//删除属性文件
    printk(KERN_INFO "unregister spi device successful!\n");
}

static struct spi_device_id dummy_spidev_ids[] = {
    {.name = "dummy-spi-device",},
    {},
};

static const struct of_device_id dummy_spidev_match[] = {
	{ .compatible = "dummy-spi-device", },
	{}
};
MODULE_DEVICE_TABLE(of, dummy_spidev_match);

static struct spi_driver dummy_spidev_driver = {  
    .driver = {  
        .name =  "dummy-spi-device",  
        .owner = THIS_MODULE,  
        .of_match_table = dummy_spidev_match,
    },  
    .probe  = dummy_spidev_probe,  
    .remove = dummy_spidev_remove,  
    .id_table = dummy_spidev_ids,
};  

module_spi_driver(dummy_spidev_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("dummy spi device driver test");
