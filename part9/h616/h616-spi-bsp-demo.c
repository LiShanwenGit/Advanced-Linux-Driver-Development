#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/dmaengine.h>
#include <linux/spi/spi.h>

#define H616_SPI_GCR_REG      (0x04)
#define H616_SPI_SRST_BIT     BIT(31)
#define H616_SPI_MODE_BIT     BIT(1)
#define H616_SPI_EN_BIT       BIT(0)
#define H616_SPI_TXD_REG      (0x0200)
#define H616_SPI_RXD_REG      (0x0300)

#define SUN6I_TXDATA_REG		0x200
#define SUN6I_RXDATA_REG		0x300
#define SUN6I_INT_STA_REG		0x14
#define SUN6I_FIFO_CTL_REG		0x18
#define SUN6I_FIFO_CTL_RF_RST			BIT(15)
#define SUN6I_FIFO_CTL_TF_RST			BIT(31)
#define SUN6I_FIFO_CTL_RF_RDY_TRIG_LEVEL_BITS	0
#define SUN6I_FIFO_CTL_TF_ERQ_TRIG_LEVEL_BITS	16
#define SUN6I_TFR_CTL_REG		0x08
#define SUN6I_TFR_CTL_DHB			BIT(8)
#define SUN6I_TFR_CTL_CS_MANUAL			BIT(6)
#define SUN6I_TFR_CTL_CS_MASK			0x30
#define SUN6I_TFR_CTL_CS(cs)			(((cs) << 4) & SUN6I_TFR_CTL_CS_MASK)
#define SUN6I_GBL_CTL_REG		0x04
#define SUN6I_BURST_CNT_REG		0x30
#define SUN6I_XMIT_CNT_REG		0x34
#define SUN6I_BURST_CTL_CNT_REG		0x38
#define SUN6I_INT_CTL_TC			BIT(12)
#define SUN6I_INT_CTL_REG		0x10
#define SUN6I_TFR_CTL_XCH			BIT(31)
#define SUN6I_GBL_CTL_BUS_ENABLE		BIT(0)
#define SUN6I_TFR_CTL_CS_LEVEL			BIT(7)
#define SUN6I_FIFO_CTL_TF_DRQ_EN		BIT(24)
#define SUN6I_FIFO_CTL_RF_DRQ_EN		BIT(8)
#define SUN6I_TFR_CTL_CPHA			BIT(0)
#define SUN6I_TFR_CTL_CPOL			BIT(1)
#define SUN6I_TFR_CTL_SPOL			BIT(2)
#define SUN6I_TFR_CTL_FBS			BIT(12)
#define SUN6I_FIFO_STA_REG		0x1c
#define SUN6I_FIFO_STA_REG		0x1c
#define SUN6I_FIFO_STA_RF_CNT_MASK		GENMASK(7, 0)
#define SUN6I_FIFO_STA_TF_CNT_MASK		GENMASK(23, 16)

#define SUN6I_CLK_CTL_REG		0x24
#define SUN6I_CLK_CTL_CDR2_MASK			0xff
#define SUN6I_CLK_CTL_CDR2(div)			(((div) & SUN6I_CLK_CTL_CDR2_MASK) << 0)
#define SUN6I_CLK_CTL_CDR1_MASK			0xf
#define SUN6I_CLK_CTL_CDR1(div)			(((div) & SUN6I_CLK_CTL_CDR1_MASK) << 8)
#define SUN6I_CLK_CTL_DRS			BIT(12)

struct h616_spi {
	struct spi_master	 *master;
	void __iomem		 *base_addr;
	struct clk		     *hclk;
	struct clk		     *mclk;
	struct reset_control *rstc;
	struct completion	done;
};

static inline void h616_spi_write_reg(struct h616_spi *h616, u32 reg, u32 value)
{
	writel(value, h616->base_addr + reg);
}

static inline u32 h616_spi_read_reg(struct h616_spi *h616, u32 reg)
{
	return readl(h616->base_addr + reg);
}


static int h616_spimaster_setup(struct spi_device *spi)
{
    int ret = 0;
    if(spi->chip_select >= spi->master->num_chipselect)
    {
        printk(KERN_INFO " invalid chip_select\n");
        return -1;
    }
    return ret;
}

static void h616_spimaster_set_cs(struct spi_device *spi, bool enable)
{
	u32 reg;
	struct h616_spi *h616 = spi_master_get_devdata(spi->master);
	reg = h616_spi_read_reg(h616, SUN6I_TFR_CTL_REG);
	reg &= ~SUN6I_TFR_CTL_CS_MASK;
	reg |= SUN6I_TFR_CTL_CS(spi->chip_select);
	if (enable) {
        //set IO(spi->chip_select) high level
		reg |= SUN6I_TFR_CTL_CS_LEVEL;
        printk(KERN_INFO "spi cs[%d]: high\n", spi->chip_select);
    }
    else {
        //set IO(spi->chip_select) low level
		reg &= ~SUN6I_TFR_CTL_CS_LEVEL;
        printk(KERN_INFO "spi cs[%d]: low\n", spi->chip_select);
    }
	h616_spi_write_reg(h616, SUN6I_TFR_CTL_REG, reg);
}


static int h616_spimaster_transfer_one(struct spi_master *master, struct spi_device *spi, struct spi_transfer *tfr)
{
	struct h616_spi *h616 = spi_master_get_devdata(master);
    int bytes;
    unsigned char *rx_buff = tfr->rx_buf;
    const unsigned char *tx_buff = tfr->tx_buf;
    bytes = tfr->len;
	unsigned int trig_level;
	int tx_len = 0, rx_len = 0;
	trig_level = 64 / 4 *3;
	uint32_t reg;
	unsigned int mclk_rate, div, div_cdr1, div_cdr2, timeout;
	int fifo_count;
	uint8_t tx_byte;

	/* Clear pending interrupts */
	h616_spi_write_reg(h616, SUN6I_INT_STA_REG, ~0);
	h616_spi_write_reg(h616, SUN6I_FIFO_CTL_REG, SUN6I_FIFO_CTL_RF_RST | SUN6I_FIFO_CTL_TF_RST);

	if (tfr->tx_buf)
		reg |= SUN6I_FIFO_CTL_TF_DRQ_EN;
	if (tfr->rx_buf)
		reg |= SUN6I_FIFO_CTL_RF_DRQ_EN;

	reg |= (trig_level << SUN6I_FIFO_CTL_RF_RDY_TRIG_LEVEL_BITS) |
	       (trig_level << SUN6I_FIFO_CTL_TF_ERQ_TRIG_LEVEL_BITS);

	h616_spi_write_reg(h616, SUN6I_FIFO_CTL_REG, reg);
	/*
	 * Setup the transfer control register: Chip Select,
	 * polarities, etc.
	 */
	reg = h616_spi_read_reg(h616, SUN6I_TFR_CTL_REG);

	if (spi->mode & SPI_CPOL)
		reg |= SUN6I_TFR_CTL_CPOL;
	else
		reg &= ~SUN6I_TFR_CTL_CPOL;

	if (spi->mode & SPI_CPHA)
		reg |= SUN6I_TFR_CTL_CPHA;
	else
		reg &= ~SUN6I_TFR_CTL_CPHA;

	if (spi->mode & SPI_LSB_FIRST)
		reg |= SUN6I_TFR_CTL_FBS;
	else
		reg &= ~SUN6I_TFR_CTL_FBS;
	/*
	 * If it's a TX only transfer, we don't want to fill the RX
	 * FIFO with bogus data
	 */
	if (tfr->rx_buf) {
		reg &= ~SUN6I_TFR_CTL_DHB;
		rx_len = tfr->len;
	} else {
		reg |= SUN6I_TFR_CTL_DHB;
	}
	/* We want to control the chip select manually */
	reg |= SUN6I_TFR_CTL_CS_MANUAL;

	h616_spi_write_reg(h616, SUN6I_TFR_CTL_REG, reg);
/* Ensure that we have a parent clock fast enough */
	mclk_rate = clk_get_rate(h616->mclk);
	if (mclk_rate < (2 * tfr->speed_hz)) {
		clk_set_rate(h616->mclk, 2 * tfr->speed_hz);
		mclk_rate = clk_get_rate(h616->mclk);
	}

	/*
	 * Setup clock divider.
	 *
	 * We have two choices there. Either we can use the clock
	 * divide rate 1, which is calculated thanks to this formula:
	 * SPI_CLK = MOD_CLK / (2 ^ cdr)
	 * Or we can use CDR2, which is calculated with the formula:
	 * SPI_CLK = MOD_CLK / (2 * (cdr + 1))
	 * Wether we use the former or the latter is set through the
	 * DRS bit.
	 *
	 * First try CDR2, and if we can't reach the expected
	 * frequency, fall back to CDR1.
	 */
	div_cdr1 = DIV_ROUND_UP(mclk_rate, tfr->speed_hz);
	div_cdr2 = DIV_ROUND_UP(div_cdr1, 2);
	if (div_cdr2 <= (SUN6I_CLK_CTL_CDR2_MASK + 1)) {
		reg = SUN6I_CLK_CTL_CDR2(div_cdr2 - 1) | SUN6I_CLK_CTL_DRS;
		tfr->effective_speed_hz = mclk_rate / (2 * div_cdr2);
	} else {
		div = min(SUN6I_CLK_CTL_CDR1_MASK, order_base_2(div_cdr1));
		reg = SUN6I_CLK_CTL_CDR1(div);
		tfr->effective_speed_hz = mclk_rate / (1 << div);
	}

	h616_spi_write_reg(h616, SUN6I_CLK_CTL_REG, reg);
	/* Finally enable the bus - doing so before might raise SCK to HIGH */
	reg = h616_spi_read_reg(h616, SUN6I_GBL_CTL_REG);
	reg |= SUN6I_GBL_CTL_BUS_ENABLE;
	h616_spi_write_reg(h616, SUN6I_GBL_CTL_REG, reg);

	/* Setup the counters */
	h616_spi_write_reg(h616, SUN6I_BURST_CNT_REG, tfr->len);
	h616_spi_write_reg(h616, SUN6I_XMIT_CNT_REG, tfr->len);
	h616_spi_write_reg(h616, SUN6I_BURST_CTL_CNT_REG, tfr->len);

	reg = h616_spi_read_reg(h616, SUN6I_FIFO_STA_REG);
	fifo_count =  FIELD_GET(SUN6I_FIFO_STA_TF_CNT_MASK, reg);

	fifo_count = 64 - fifo_count;
	tx_len = min((int)fifo_count, tfr->len);

	tx_buff = tfr->tx_buf;
	while (tx_len--) {
		if(tx_buff) {
			tx_byte = tx_buff ? *tx_buff : 0;
			writeb(tx_byte, h616->base_addr + SUN6I_TXDATA_REG);
		}
		//tx_buff ++;
	}

	/* Start the transfer */
	reg = h616_spi_read_reg(h616, SUN6I_TFR_CTL_REG);
	h616_spi_write_reg(h616, SUN6I_TFR_CTL_REG, reg | SUN6I_TFR_CTL_XCH);

	//h616_spi_write_reg(h616, SUN6I_INT_CTL_REG, 0);

    // while(bytes) {
    //     if(tx_buff) {
    //         //h616_spi_write_reg(h616, H616_SPI_TXD_REG, *tx_buff);
    //         tx_buff ++ ;
    //     }
    //     if(rx_buff) {
    //         //*rx_buff = h616_spi_read_reg(h616, H616_SPI_RXD_REG);
    //         rx_buff ++ ;
    //     }
    //     bytes -- ;
    // }
    return 0;
}

void h616_spi_init(void)
{

}

static int h616_spi_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *mem;
	struct h616_spi *h616 = NULL;
	struct spi_master	*master;
	master = spi_alloc_master(&pdev->dev, sizeof(struct h616_spi));
	if (!master) {
		dev_err(&pdev->dev, "allocate spi master failed!\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, master);
	h616 = spi_master_get_devdata(master);
	h616->base_addr = devm_platform_get_and_ioremap_resource(pdev, 0, &mem);
	if (IS_ERR(h616->base_addr)) {
		ret = PTR_ERR(h616->base_addr);
		goto err_0;
	}

	h616->master = master;
	master->max_speed_hz = 100 * 1000 * 1000;
	master->min_speed_hz = 3 * 1000;
	master->use_gpio_descriptors = true;
	master->set_cs = h616_spimaster_set_cs;
	master->transfer_one = h616_spimaster_transfer_one;
	master->num_chipselect = 4;
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LSB_FIRST;
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->dev.of_node = pdev->dev.of_node;
	master->auto_runtime_pm = false;
	master->setup = h616_spimaster_setup;
	//master->max_transfer_size = dummy_spi_max_transfer_size;

	h616->hclk = devm_clk_get(&pdev->dev, "ahb");
	if (IS_ERR(h616->hclk)) {
		dev_err(&pdev->dev, "Unable to acquire AHB clock\n");
		ret = PTR_ERR(h616->hclk);
		goto err_0;
	}

	h616->mclk = devm_clk_get(&pdev->dev, "mod");
	if (IS_ERR(h616->mclk)) {
		dev_err(&pdev->dev, "Unable to acquire module clock\n");
		ret = PTR_ERR(h616->mclk);
		goto err_0;
	}

	h616->rstc = devm_reset_control_get_exclusive(&pdev->dev, NULL);
	if (IS_ERR(h616->rstc)) {
		dev_err(&pdev->dev, "Couldn't get reset controller\n");
		ret = PTR_ERR(h616->rstc);
		goto err_0;
	}

	ret = clk_prepare_enable(h616->hclk);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't enable AHB clock\n");
		goto err_0;
	}

	ret = clk_prepare_enable(h616->mclk);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't enable module clock\n");
		goto err_1;
	}

	ret = reset_control_deassert(h616->rstc);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't deassert the device from reset\n");
		goto err_2;
	}

	//h616_spi_init(h616);

	ret = devm_spi_register_master(&pdev->dev, master);
	if (ret) {
		dev_err(&pdev->dev, "cannot register SPI master\n");
		goto err_2;
	}
	init_completion(&h616->done);
	return 0;

err_2:
	clk_disable_unprepare(h616->mclk);
err_1:
	clk_disable_unprepare(h616->hclk);
err_0:
	spi_unregister_master(master);
	return ret;
}

static int h616_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct h616_spi   *h616 = spi_master_get_devdata(master);
	reset_control_assert(h616->rstc);
	clk_disable_unprepare(h616->mclk);
	clk_disable_unprepare(h616->hclk);
	spi_unregister_master(master);
	return 0;
}

static const struct of_device_id h616_spi_bsp_match[] = {
	{ .compatible = "h616-spi-bsp-demo", },
	{}
};
MODULE_DEVICE_TABLE(of, h616_spi_bsp_match);

static struct platform_driver h616_spi_bsp = {
	.probe	= h616_spi_probe,
	.remove	= h616_spi_remove,
	.driver	= {
		.name = "h616-spi-demo",
		.of_match_table	= h616_spi_bsp_match,
	},
};
module_platform_driver(h616_spi_bsp);

MODULE_AUTHOR("1477153217@qq.com");
MODULE_AUTHOR("Li shanwen");
MODULE_DESCRIPTION("h616 spi bsp demo");
MODULE_LICENSE("GPL");
