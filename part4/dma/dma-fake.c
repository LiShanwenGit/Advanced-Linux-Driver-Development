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
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>


static int dma_fake_dma_xfr(struct platform_device *pdev)
{
    ...
    /* 下面是配置DMA的RX通道 */
    struct dma_slave_config rxconf = {
        .direction = DMA_DEV_TO_MEM,
        .src_addr = sspi->dma_addr_rx,
        .src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES,
        .src_maxburst = 8,
    };

    dmaengine_slave_config(master->dma_rx, &rxconf);

    rxdesc = dmaengine_prep_slave_sg(master->dma_rx,
                        tfr->rx_sg.sgl,
                        tfr->rx_sg.nents,
                        DMA_DEV_TO_MEM,
                        DMA_PREP_INTERRUPT);
    if (!rxdesc)
        return -EINVAL;
    ...

    /* 下面是配置DMA的TX通道 */
    struct dma_slave_config txconf = {
        .direction = DMA_MEM_TO_DEV,
        .dst_addr = sspi->dma_addr_tx,
        .dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES,
        .dst_maxburst = 8,
    };

    dmaengine_slave_config(master->dma_tx, &txconf);

    txdesc = dmaengine_prep_slave_sg(master->dma_tx,
                        tfr->tx_sg.sgl,
                        tfr->tx_sg.nents,
                        DMA_MEM_TO_DEV,
                        DMA_PREP_INTERRUPT);
    if (!txdesc) {
        if (rxdesc)
            dmaengine_terminate_sync(master->dma_rx);
        return -EINVAL;
    }

    dmaengine_submit(rxdesc);
    dma_async_issue_pending(master->dma_rx);
    dmaengine_submit(txdesc);
	dma_async_issue_pending(master->dma_tx);
}

static int dma_fake_probe(struct platform_device *pdev)
{
    ...
    /* 下面代码摘自drivers/spi/spi-sun6i.c */
	master->dma_tx = dma_request_chan(&pdev->dev, "tx");
	if (IS_ERR(master->dma_tx)) {
		/* Check tx to see if we need defer probing driver */
		if (PTR_ERR(master->dma_tx) == -EPROBE_DEFER) {
			ret = -EPROBE_DEFER;
			goto err_free_master;
		}
		dev_warn(&pdev->dev, "Failed to request TX DMA channel\n");
		master->dma_tx = NULL;
	}

	master->dma_rx = dma_request_chan(&pdev->dev, "rx");
	if (IS_ERR(master->dma_rx)) {
		if (PTR_ERR(master->dma_rx) == -EPROBE_DEFER) {
			ret = -EPROBE_DEFER;
			goto err_free_dma_tx;
		}
		dev_warn(&pdev->dev, "Failed to request RX DMA channel\n");
		master->dma_rx = NULL;
	}
    ...
    return 0;
}

static int dma_fake_remove(struct platform_device *pdev)
{
    
    return 0;
}

static const struct of_device_id dma_fake_match[] = {
	{.name = "dma-fake",},
	{}
};
MODULE_DEVICE_TABLE(of, dma_fake_match);

static struct platform_driver dma_fake_driver = {  
    .driver = {  
        .owner = THIS_MODULE,  
        .of_match_table = dma_fake_match,
    },  
    .probe  = dma_fake_probe,  
    .remove = dma_fake_remove,  
};  

module_platform_driver(dma_fake_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("dam fake driver demo");
