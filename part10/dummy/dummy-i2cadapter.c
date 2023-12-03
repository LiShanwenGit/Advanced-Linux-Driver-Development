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

static int dummy_i2cadapter_xfer (struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    int i = 0, index;
    int wr_flag, stop;
    for (index = 0; index < num; index++) {
        //发生错误，无回应，I2C_M_TEN表示支持10bit地址,这里不支持10bit地址
        if( (msgs[index].flags & (I2C_M_NO_RD_ACK | I2C_M_IGNORE_NAK | I2C_M_TEN)) || msgs[i].len == 0) {
            return ENOTSUPP;
        }
        //发送开始信号，从设备地址
        if (!(msgs[index].flags & I2C_M_NOSTART)) {
            wr_flag = (msgs[index].flags & I2C_M_RD ? 0x01 : 0x00);
            printk(KERN_INFO "TX: START %x\n", (msgs[index].addr<<1) | wr_flag );
        }
        //读取数据
        if(msgs[index].flags & I2C_M_RD) {
            for(i = 0; i < msgs[index].len; i++) {
                stop = (index+1 == num && i+1 == msgs[index].len) ? 0x01 : 0x00;
                msgs[index].buf[i] = 'A';
                if(stop)
                    printk(KERN_INFO "TX: STOP\n");
            }
        }
        //发送数据
        else {
            for(i = 0; i < msgs[index].len; i++) {
                stop = (index+1 == num && i+1 == msgs[index].len) ? 0x01 : 0x00;
                printk(KERN_INFO "TX: %c \n", msgs[index].buf[i]);
                if(stop)
                    printk(KERN_INFO "TX: STOP\n");
            }
        }
    }
    return 0;
}

static u32 dummy_i2cadapter_functionality (struct i2c_adapter * adap)
{
    return I2C_FUNC_I2C;
}

static struct i2c_algorithm dummy_i2cadapter_algo = 
{
    .master_xfer = dummy_i2cadapter_xfer,
    .functionality = dummy_i2cadapter_functionality
};


static int dummy_i2cadapter_probe(struct platform_device *pdev)
{
    struct i2c_adapter *adapter;
    adapter = devm_kzalloc(&pdev->dev, sizeof(struct i2c_adapter), GFP_KERNEL);
    if (!adapter) 
    {
        printk(KERN_ERR "unable to alloc i2c adapter \n");
        return -EINVAL;
    }

    adapter->dev.parent = &pdev->dev;
    adapter->owner = THIS_MODULE;
    strcpy(adapter->name, "dummy_i2c_adapter");
    adapter->algo = &dummy_i2cadapter_algo;
    adapter->class = I2C_CLASS_DEPRECATED;
    adapter->dev.parent = &pdev->dev;
	adapter->nr = pdev->id;
	adapter->dev.of_node = pdev->dev.of_node;
    if (i2c_add_adapter(adapter)) 
    {
        printk(KERN_ERR "register i2c adapter failed\n");
        return -EINVAL;
    }
    platform_set_drvdata(pdev, adapter);
    i2c_set_adapdata(adapter, adapter);
    printk(KERN_INFO "register i2c adapter successful\n");
    return 0;
}

static int dummy_i2cadapter_remove(struct platform_device *pdev)
{
    struct i2c_adapter *adapter = platform_get_drvdata(pdev);
    i2c_del_adapter(adapter);
    printk(KERN_INFO "unregister i2c adapter successful\n");
    return 0;
}

static const struct of_device_id dummy_i2cadapter_match[] = {
	{ .compatible = "dummy-i2c-adapter", },
	{}
};
MODULE_DEVICE_TABLE(of, dummy_i2cadapter_match);

static struct platform_driver dummy_i2cadapter_driver = {
    .driver = {
        .name = "dummy_i2cadapter_driver",
        .of_match_table = dummy_i2cadapter_match,
    },
    .probe = dummy_i2cadapter_probe,
    .remove = dummy_i2cadapter_remove,
};

module_platform_driver(dummy_i2cadapter_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("dummy i2c controller driver test");
