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

static int dummy_spimaster_setup(struct spi_device *spi)
{
    int ret = 0;
    if(spi->chip_select >= spi->master->num_chipselect)
    {
        printk(KERN_INFO " invalid chip_select\n");
        return -1;
    }
    return ret;
}

static void dummy_spimaster_set_cs(struct spi_device *spi, bool enable)
{
	if (enable) {
        //set IO(spi->chip_select) high level
        printk(KERN_INFO "spi cs[%d]: high\n", spi->chip_select);
    }
    else {
        //set IO(spi->chip_select) low level
        printk(KERN_INFO "spi cs[%d]: low\n", spi->chip_select);
    }
}

static int dummy_spimaster_transfer_one(struct spi_master *master, struct spi_device *spi, struct spi_transfer *tfr)
{
    int bytes;
    unsigned char *rx_buff = tfr->rx_buf;
    const unsigned char *tx_buff = tfr->tx_buf;
    bytes = tfr->len;
    while(bytes) {
        if(tx_buff) {
            printk(KERN_INFO "TX: %c \n", *tx_buff);
            tx_buff ++ ;
        }
        if(rx_buff) {
            *rx_buff = 'a';//读取时每次获取'a'
            rx_buff ++ ;
        }
        bytes -- ;
    }
    return 0;
}

static int dummy_spimaster_probe(struct platform_device *pdev)
{
    int ret;
    struct spi_master *master;

    master = spi_alloc_master(&pdev->dev, 0);
    if (!master) 
    {
        printk(KERN_ERR "unable to alloc SPI master\n");
        return -EINVAL;
    }
    master->dev.of_node = pdev->dev.of_node;
    master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LSB_FIRST;
    master->bus_num = -1;
    master->auto_runtime_pm = false;//不支持PM，如果需要支持，则需要实现platform_driver中.driver.pm
    master->num_chipselect = 4;
    master->transfer_one = dummy_spimaster_transfer_one;
    master->setup = dummy_spimaster_setup;
	master->max_speed_hz = 100 * 1000 * 1000; //100MHz
	master->min_speed_hz = 3 * 1000;    //3kHz
	master->set_cs = dummy_spimaster_set_cs;
	ret = devm_spi_register_master(&pdev->dev, master);
	if (ret) {
		dev_err(&pdev->dev, "spi register master failed!\n");
		spi_master_put(master);
		return ret;
	}
    platform_set_drvdata(pdev, master);
    return 0;
} 

static int dummy_spimaster_remove(struct platform_device *pdev)
{
    struct spi_master	*master = platform_get_drvdata(pdev);
    spi_unregister_master(master);
    printk("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

static const struct of_device_id dummy_spimaster_match[] = {
	{ .compatible = "dummy-spi-master", },
	{}
};
MODULE_DEVICE_TABLE(of, dummy_spimaster_match);

static struct platform_driver dummy_spimaster_driver = {
    .driver = {
        .name = "dummy_spimaster_driver",
        .owner = THIS_MODULE,
        .of_match_table = dummy_spimaster_match,
    },
    .probe = dummy_spimaster_probe,
    .remove = dummy_spimaster_remove,
    //.driver.pm = 
};

module_platform_driver(dummy_spimaster_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("dummy spi controller driver test");
