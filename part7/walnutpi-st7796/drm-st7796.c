#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <video/mipi_display.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_mipi_dbi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_format_helper.h>
#include <drm/drm_framebuffer.h>

struct drm_display {
	struct spi_device             *spi;
	struct gpio_desc              *reset;
	struct gpio_desc              *dc;
	struct gpio_desc              *detect;
	struct drm_device              drm_dev;
    struct drm_simple_display_pipe pipe;
	struct drm_display_mode        mode;
    struct drm_connector           connector;
	char                          *buffer;
};

static void fb_write_reg(struct drm_display *drm, u8 reg)
{
	struct spi_device *spi = drm->spi;
    gpiod_set_value(drm->dc, 0); //低电平，命令
    spi_write(spi, &reg, 1);
}

static void fb_write_data(struct drm_display *drm, u8 data)
{   
	struct spi_device *spi = drm->spi;
    gpiod_set_value(drm->dc, 1);  //高电平，数据
    spi_write(spi, &data, 1);
}

static void fb_set_win(struct drm_display *drm, u16 xStar, u16 yStar, u16 xEnd, u16 yEnd)
{
    fb_write_reg(drm, 0x2a);
    fb_write_data(drm, (xStar >> 8) & 0xff);
	fb_write_data(drm, xStar & 0xff);
    fb_write_data(drm, (xEnd >> 8) & 0xff);
	fb_write_data(drm, xEnd & 0xff);
    fb_write_reg(drm, 0x2b); 
    fb_write_data(drm, (yStar >> 8) & 0xff);
	fb_write_data(drm, yStar & 0xff );
    fb_write_data(drm, (yEnd >> 8) & 0xff);
	fb_write_data(drm, yEnd & 0xff);
    fb_write_reg(drm, 0x2c); 
}

static void myfb_init(struct drm_display *drm)
{
    gpiod_set_value(drm->reset, 0); //设低电平
    msleep(300);
    gpiod_set_value(drm->reset, 1); //设高电平
    msleep(300);
    /* startup sequence for MI0283QT-9A */
    fb_write_reg(drm, 0x01);
    mdelay(10);
    fb_write_reg(drm, 0x28);

    /* ------------power control-------------------------------- */
    fb_write_reg(drm, 0xC0);
    fb_write_data(drm,0x0c);
	fb_write_data(drm,0x02);

    fb_write_reg(drm, 0xC1);
    fb_write_data(drm,0x44);
    /* ------------VCOM --------- */
    fb_write_reg(drm, 0xC5);
    fb_write_data(drm,0x00);
	fb_write_data(drm,0x16);
    fb_write_data(drm,0x80);

    fb_write_reg(drm, 0x36);
    fb_write_data(drm, 0x28);

    fb_write_reg(drm, 0x3a); // Interface Mode Control
    fb_write_data(drm, 0x55);

    fb_write_reg(drm, 0xB0);
	fb_write_data(drm, 0x00);

    /* ------------frame rate----------------------------------- */
    fb_write_reg(drm, 0xB1); // 70HZ
    fb_write_data(drm, 0xB0);

    fb_write_reg(drm, 0xB4);
    fb_write_data(drm, 0x02);

    fb_write_reg(drm, 0xB6); // RGB/MCU Interface Control
    fb_write_data(drm, 0x02);
	fb_write_data(drm, 0x02);

    fb_write_reg(drm, 0xE9);
    fb_write_data(drm, 0x00);

    fb_write_reg(drm, 0xF7);
    fb_write_data(drm, 0xA9);
	fb_write_data(drm, 0x51);
    fb_write_data(drm, 0x2C);
	fb_write_data(drm, 0x82);

	fb_write_reg(drm, 0x36);
	fb_write_data(drm, 0x28);
    fb_write_reg(drm, 0x11);
    mdelay(120);
    fb_write_reg(drm, 0x29);
    mdelay(20);
    mdelay(200);
}

static const struct drm_display_mode st7796_mode = {
	//宽度像素480，高度像素320，宽度尺寸83mm，高度尺寸55mm
	DRM_SIMPLE_MODE(480, 320, 83, 55),
};

static const u32 st7796_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_XRGB8888,
};

static const uint64_t modifiers[] = {
    DRM_FORMAT_MOD_LINEAR,
    DRM_FORMAT_MOD_INVALID
};

static const struct drm_connector_funcs connector_funcs = {
	.reset = drm_atomic_helper_connector_reset,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static void st7796_pipe_enable(struct drm_simple_display_pipe *pipe,
			       struct drm_crtc_state *crtc_state,
			       struct drm_plane_state *plane_state)
{
	struct drm_display *drm = container_of(pipe, struct drm_display, pipe);
	myfb_init(drm);
}

static void st7796_pipe_disable(struct drm_simple_display_pipe *pipe)
{
	//由于核桃派没有背光使能引脚，故不做处理。
}

static void st7796_pipe_update(struct drm_simple_display_pipe *pipe, struct drm_plane_state *old_state)
{
	struct drm_display *drm_st7796;
	struct drm_plane_state *state = pipe->plane.state;
	struct drm_framebuffer *fb = state->fb;
	struct drm_rect rect;
	int idx, ret;
	struct drm_gem_cma_object *src_cma_obj = drm_fb_cma_get_gem_obj(fb, 0);
	struct drm_gem_object *gem = drm_gem_fb_get_obj(fb, 0);
	void *src = src_cma_obj->vaddr;
	unsigned int height;
	unsigned int width;
	drm_st7796 = container_of(fb->dev, struct drm_display, drm_dev);
	if (!pipe->crtc.state->active)
		return;

	if (drm_atomic_helper_damage_merged(old_state, state, &rect)) {
		height = rect.y2 - rect.y1;
		width = rect.x2 - rect.x1;

		if (!drm_dev_enter(fb->dev, &idx))
			return;
		ret = drm_gem_fb_begin_cpu_access(fb, DMA_FROM_DEVICE);
		if (ret)
			return;
		if(fb->format->format == DRM_FORMAT_RGB565) {
			#if 1
			drm_fb_swab(drm_st7796->buffer, src, fb, &rect, !gem->import_attach);
			#else 
			drm_fb_memcpy(drm_st7796->buffer, src, fb, &rect);
			#endif
		}
		else if(fb->format->format == DRM_FORMAT_XRGB8888) {
			drm_fb_xrgb8888_to_rgb565(drm_st7796->buffer, src, fb, &rect, 1);
		}
		drm_gem_fb_end_cpu_access(fb, DMA_FROM_DEVICE);
		fb_set_win(drm_st7796, rect.x1, rect.y1, rect.x2 - 1, rect.y2 - 1);
		gpiod_set_value(drm_st7796->dc, 1);  //高电平，发送数据
		spi_write(drm_st7796->spi, drm_st7796->buffer, width * height * 2);
		drm_dev_exit(idx);
	}
}

static const struct drm_simple_display_pipe_funcs st7796_pipe_funcs = {
	.enable		= st7796_pipe_enable,
	.disable	= st7796_pipe_disable,
	.update		= st7796_pipe_update,
};

static int connector_helper_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(dev, &st7796_mode);
	if (!mode) {
		drm_err(dev, "Failed to duplicate mode " DRM_MODE_FMT "\n",
			DRM_MODE_ARG(&st7796_mode));
		return 0;
	}

	if (mode->name[0] == '\0')
		drm_mode_set_name(mode);

	mode->type |= DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	if (mode->width_mm)
		connector->display_info.width_mm = mode->width_mm;
	if (mode->height_mm)
		connector->display_info.height_mm = mode->height_mm;

	return 1;
}

static const struct drm_connector_helper_funcs connector_helper_funcs = {
	.get_modes = connector_helper_get_modes,
};

static const struct file_operations drm_fops = {
	.owner		= THIS_MODULE,
	.open		= drm_open,
	.release	= drm_release,
	.unlocked_ioctl	= drm_ioctl,
	.compat_ioctl	= drm_compat_ioctl,
	.poll		= drm_poll,
	.read		= drm_read,
	.llseek		= noop_llseek,
	.mmap		= drm_gem_mmap,
	DRM_GEM_CMA_UNMAPPED_AREA_FOPS
};

static struct drm_driver test_driver = {
	.driver_features	= DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
    .fops           = &drm_fops,
	DRM_GEM_CMA_DRIVER_OPS_VMAP,
	.name			= "drm-test",
	.desc			= "drm dummy test",
	.date			= "20230627",
	.major			= 1,
	.minor			= 0,
};

static const struct drm_mode_config_funcs mode_config_funcs = {
	.fb_create = drm_gem_fb_create_with_dirty,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static int drm_st7796_probe(struct spi_device *spi)
{
    int ret;
    struct drm_display *drm_st7796;
    struct drm_device *drm_dev;
    struct drm_simple_display_pipe *pipe;
    struct device *dev = &spi->dev;
    struct drm_connector *connector;
    drm_st7796 = devm_drm_dev_alloc(dev, &test_driver, struct drm_display, drm_dev);
    if(IS_ERR(drm_st7796)) {
		printk(KERN_ERR "struct drm_display alloc filed\n");
		return -ENOMEM;
	}
    drm_dev = &drm_st7796->drm_dev;
    pipe    = &drm_st7796->pipe;
    drm_st7796->spi = spi;
    connector = &(drm_st7796->connector);

	drm_st7796->reset = devm_gpiod_get(dev,"reset",GPIOD_OUT_HIGH);
	if (IS_ERR(drm_st7796->reset))
		return dev_err_probe(dev, PTR_ERR(drm_st7796->reset), "Failed to get GPIO 'reset'\n");

	drm_st7796->dc = devm_gpiod_get(dev,"dc",GPIOD_OUT_LOW);
	if (IS_ERR(drm_st7796->dc))
		return dev_err_probe(dev, PTR_ERR(drm_st7796->dc), "Failed to get GPIO 'dc'\n");
	gpiod_direction_output(drm_st7796->reset, 0);
	gpiod_direction_output(drm_st7796->dc, 0);

	//分配内存用于LCD显存
	drm_st7796->buffer = kzalloc(st7796_mode.hdisplay*st7796_mode.vdisplay*2, GFP_KERNEL);
	if(drm_st7796->buffer == NULL) {
 		printk(KERN_ERR "buffer alloc failed\n");
        return -ENOMEM;
	}

    dma_coerce_mask_and_coherent(dev, DMA_BIT_MASK(32));
    ret = drmm_mode_config_init(drm_dev);
    if(ret) {
        printk(KERN_ERR "drmm_mode_config_init failed\n");  
        return ret;
    }

	drm_connector_helper_add(connector, &connector_helper_funcs);
    ret = drm_connector_init(drm_dev, connector, &connector_funcs, DRM_MODE_CONNECTOR_SPI);
    if(ret) {
        printk(KERN_ERR "drm_connector_init failed\n");
        return ret;
    }

	ret = drm_simple_display_pipe_init(drm_dev, pipe, &st7796_pipe_funcs, st7796_formats, ARRAY_SIZE(st7796_formats),
					   modifiers, connector);
	if (ret) {
        printk(KERN_ERR "drm_simple_display_pipe_init failed\n");  
        return ret;
    }

	drm_plane_enable_fb_damage_clips(&pipe->plane);

    drm_dev->mode_config.funcs = &mode_config_funcs;
	drm_dev->mode_config.min_width = st7796_mode.hdisplay;
	drm_dev->mode_config.max_width = st7796_mode.hdisplay;
	drm_dev->mode_config.min_height = st7796_mode.vdisplay;
	drm_dev->mode_config.max_height = st7796_mode.vdisplay;
	drm_mode_config_reset(drm_dev);
    ret = drm_dev_register(drm_dev, 0);
    if(ret) {
        printk(KERN_ERR "drm_dev register failed\n");
        return ret;
    }
	spi_set_drvdata(spi, drm_st7796);
	//设置fb的颜色深度为16位，即RGB5654
	//也可以直接赋值drm_dev->mode_config.preferred_depth = 16;
	drm_fbdev_generic_setup(drm_dev, 16);
    return 0;
}

static int drm_st7796_remove(struct spi_device *spi)
{
	struct drm_display *drm = spi_get_drvdata(spi);
	spi_set_drvdata(spi, NULL);
	devm_gpiod_put(&spi->dev, drm->dc);
	devm_gpiod_put(&spi->dev, drm->reset);
    drm_dev_unplug(&drm->drm_dev);
	kfree(drm->buffer);
	return 0;
}

static const struct of_device_id st7796_of_match[] = {
	{
		.compatible = "walnutpi,st7796",
		.data = NULL,
	},
    { },
};
MODULE_DEVICE_TABLE(of, st7796_of_match);

static const struct spi_device_id st7796_id[] = {
	{ "st7796", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, st7796_id);

static struct spi_driver st7796_driver = {
	.probe = drm_st7796_probe,
	.remove = drm_st7796_remove,
	.id_table = st7796_id,
	.driver = {
		.name = "panel-st7796",
		.of_match_table = st7796_of_match,
	},
};
module_spi_driver(st7796_driver);

MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("walnutpi st7796 drm driver");
MODULE_LICENSE("GPL v2");
