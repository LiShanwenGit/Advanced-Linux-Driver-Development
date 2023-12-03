#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>       
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/sysfs.h> 
#include <linux/slab.h>

struct i2c_client *i2c = NULL;

static ssize_t data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	i2c_master_recv(i2c, buf, 4);
    return 4; 
}

static ssize_t data_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    i2c_master_send(i2c, buf, count);
    return count;
}

static DEVICE_ATTR(data, 0660, data_show, data_store); //定义文件属性


static int dummy_i2cdev_probe(struct i2c_client *client)
{
    int ret;
    ret = device_create_file(&client->dev,&dev_attr_data);//创建属性文件
    if(ret != 0)
    {
        printk(KERN_INFO "create data file failed!\n");
        return -1;
    }
	i2c = client;
	printk(KERN_INFO "register i2c device successful\n");
    return 0;
}

static void dummy_i2cdev_remove(struct i2c_client *client)
{
    device_remove_file(&client->dev,&dev_attr_data);//删除属性文件
    printk(KERN_INFO "unregister i2c device successful!\n");
}

static const struct of_device_id dummy_i2cdev_match[] = {
	{ .compatible = "dummy-i2c-device" },
	{},
};
MODULE_DEVICE_TABLE(of, dummy_i2cdev_match);

static const struct i2c_device_id dummy_i2cdev_id[] = {
	{ "dummy-i2c-device" },
	{},
};
MODULE_DEVICE_TABLE(i2c, dummy_i2cdev_id);


static struct i2c_driver dummy_i2cdev_driver = {
	.driver = {
		.name = "dummy_i2cdev_driver",
		.of_match_table = dummy_i2cdev_match,
	},
	.probe_new = dummy_i2cdev_probe,
	.remove = dummy_i2cdev_remove,
	.id_table = dummy_i2cdev_id,
};

module_i2c_driver(dummy_i2cdev_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("dummy i2c device driver test");

