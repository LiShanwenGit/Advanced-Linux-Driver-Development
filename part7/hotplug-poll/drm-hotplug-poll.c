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
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_mipi_dbi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_fb_dma_helper.h>
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

static void fb_write_data(struct drm_display *drm, u16 data)
{   
	struct spi_device *spi = drm->spi;
    u8 buf[2];
    buf[0] = ((u8)(data>>8));
    buf[1] = ((u8)(data&0x00ff));
    gpiod_set_value(drm->dc, 1);  //高电平，数据
    spi_write(spi, &buf[0], 1);
    spi_write(spi, &buf[1], 1);   
}

static void fb_set_win(struct drm_display *drm, u8 xStar, u8 yStar,u8 xEnd,u8 yEnd)
{
    fb_write_reg(drm,0x2a);   
    fb_write_data(drm,xStar);
    fb_write_data(drm,xEnd);
    fb_write_reg(drm,0x2b);   
    fb_write_data(drm,yStar);
    fb_write_data(drm,yEnd);
    fb_write_reg(drm,0x2c); 
}

static void myfb_init(struct drm_display *drm)
{
    gpiod_set_value(drm->reset, 0); //设低电平
    msleep(100);
    gpiod_set_value(drm->reset, 1); //设高电平
    msleep(50);
	/* 写寄存器，初始化 */
    fb_write_reg(drm,0x36);
    fb_write_data(drm,0x0000);
    fb_write_reg(drm,0x3A);
    fb_write_data(drm,0x0500);
    fb_write_reg(drm,0xB2);
    fb_write_data(drm,0x0C0C);
    fb_write_data(drm,0x0033);
    fb_write_data(drm,0x3300);
    fb_write_data(drm,0x0033);
    fb_write_data(drm,0x3300);
    fb_write_reg(drm,0xB7);
    fb_write_data(drm,0x3500);
    fb_write_reg(drm,0xB8);
    fb_write_data(drm,0x1900);
    fb_write_reg(drm,0xC0);
    fb_write_data(drm,0x2C00);
    fb_write_reg(drm,0xC2);
    fb_write_data(drm,0xC100);
    fb_write_reg(drm,0xC3);
    fb_write_data(drm,0x1200);
    fb_write_reg(drm,0xC4);
    fb_write_data(drm,0x2000);
    fb_write_reg(drm,0xC6);
    fb_write_data(drm,0x0F00);
    fb_write_reg(drm,0xD0);
    fb_write_data(drm,0xA4A1);
    fb_write_reg(drm,0xE0);
    fb_write_data(drm,0xD004);
    fb_write_data(drm,0x0D11);
    fb_write_data(drm,0x132B);
    fb_write_data(drm,0x3F54);
    fb_write_data(drm,0x4C18);
    fb_write_data(drm,0x0D0B);
    fb_write_data(drm,0x1F23);
    fb_write_reg(drm,0xE1);
    fb_write_data(drm,0xD004);
    fb_write_data(drm,0x0C11);
    fb_write_data(drm,0x132C);
    fb_write_data(drm,0x3F44);
    fb_write_data(drm,0x512F);
    fb_write_data(drm,0x1F1F);
    fb_write_data(drm,0x2023);
    fb_write_reg(drm,0x21);
    fb_write_reg(drm,0x11);
    mdelay(50);  
    fb_write_reg(drm,0x29);
    mdelay(200);
}

static const struct drm_display_mode st7789v_mode = {
	//宽度像素240，高度像素240，宽度尺寸23.4mm，高度尺寸23.4mm
	DRM_SIMPLE_MODE(240, 240, 23, 23),
};

static const u32 st7789v_formats[] = {
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

static void st7789v_pipe_enable(struct drm_simple_display_pipe *pipe,
			       struct drm_crtc_state *crtc_state,
			       struct drm_plane_state *plane_state)
{
	struct drm_display *drm = container_of(pipe, struct drm_display, pipe);
	myfb_init(drm);
}

static void st7789v_pipe_disable(struct drm_simple_display_pipe *pipe)
{

}

static void st7789v_pipe_update(struct drm_simple_display_pipe *pipe, struct drm_plane_state *old_state)
{
	int ret;
	int idx;
	struct drm_rect rect;
	struct drm_plane_state *plane_state;
	struct iosys_map map[DRM_FORMAT_MAX_PLANES];
	struct iosys_map data[DRM_FORMAT_MAX_PLANES];
	struct drm_display *drm = container_of(pipe, struct drm_display, pipe);
	struct iosys_map dst_map = IOSYS_MAP_INIT_VADDR(drm->buffer);
	plane_state = pipe->plane.state;
	drm_atomic_helper_damage_merged(old_state, plane_state, &rect);
	if (!drm_dev_enter(pipe->plane.dev, &idx))
		return;
	ret = drm_gem_fb_begin_cpu_access(plane_state->fb, DMA_FROM_DEVICE);
	if (ret)
		return;
	ret = drm_gem_fb_vmap(plane_state->fb, map, data);
	if (ret)
		return;
	if(plane_state->fb->format->format == DRM_FORMAT_XRGB8888) {
		drm_fb_xrgb8888_to_rgb565(&dst_map, NULL, data, plane_state->fb, &rect, 1);
	}
	else if(plane_state->fb->format->format == DRM_FORMAT_RGB565) {
		drm_fb_memcpy(&dst_map, NULL, data, plane_state->fb, &rect);
	}
	drm_gem_fb_vunmap(plane_state->fb, map);
	fb_set_win(drm, rect.x1, rect.y1, rect.x2 - 1, rect.y2 - 1);
	gpiod_set_value(drm->dc, 1);  //高电平，发送数据
	spi_write(drm->spi, drm->buffer, (rect.x2 - rect.x1) * (rect.y2 - rect.y1)*2);
	drm_dev_exit(idx);
}

static const struct drm_simple_display_pipe_funcs st7789v_pipe_funcs = {
	.enable		= st7789v_pipe_enable,
	.disable	= st7789v_pipe_disable,
	.update		= st7789v_pipe_update,
};

static int connector_helper_get_modes(struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &st7789v_mode);
}

int connector_helper_detect_ctx(struct drm_connector *connector, struct drm_modeset_acquire_ctx *ctx, bool force)
{
	int status;
	struct drm_display *drm = container_of(connector, struct drm_display, connector);
	status = gpiod_get_value(drm->detect);
	printk(KERN_INFO "detect: %d\n", status);
	if(status) {
		connector->dev->unplugged = 0;
	}
	else {
		connector->dev->unplugged = 1;
	}
	if(drm_dev_is_unplugged(connector->dev)) {
		return connector_status_disconnected;
	}
	else {
		return connector_status_connected;
	}
}

static const struct drm_connector_helper_funcs connector_helper_funcs = {
	.get_modes = connector_helper_get_modes,
	.detect_ctx = connector_helper_detect_ctx,
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
	DRM_GEM_DMA_UNMAPPED_AREA_FOPS
};

static struct drm_driver test_driver = {
	.driver_features	= DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
    .fops           = &drm_fops,
	DRM_GEM_DMA_DRIVER_OPS_VMAP,
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

static int drm_st7789v_probe(struct spi_device *spi)
{
    int ret;
    struct drm_display *drm_st7789v;
    struct drm_device *drm_dev;
    struct drm_simple_display_pipe *pipe;
    struct device *dev = &spi->dev;
    struct drm_connector *connector;
    drm_st7789v = devm_drm_dev_alloc(dev, &test_driver, struct drm_display, drm_dev);
    if(IS_ERR(drm_st7789v)) {
		printk(KERN_ERR "struct drm_display alloc filed\n");
		return -ENOMEM;
	}
    drm_dev = &drm_st7789v->drm_dev;
    pipe    = &drm_st7789v->pipe;
    drm_st7789v->spi = spi;
    connector = &(drm_st7789v->connector);

	drm_st7789v->reset = devm_gpiod_get(dev,"reset",GPIOD_OUT_HIGH);
	if (IS_ERR(drm_st7789v->reset))
		return dev_err_probe(dev, PTR_ERR(drm_st7789v->reset), "Failed to get GPIO 'reset'\n");

	drm_st7789v->dc = devm_gpiod_get(dev,"dc",GPIOD_OUT_LOW);
	if (IS_ERR(drm_st7789v->dc))
		return dev_err_probe(dev, PTR_ERR(drm_st7789v->dc), "Failed to get GPIO 'dc'\n");
	gpiod_direction_output(drm_st7789v->reset, 0);
	gpiod_direction_output(drm_st7789v->dc, 0);

	drm_st7789v->detect = devm_gpiod_get(dev,"detect",GPIOF_OUT_INIT_LOW);
	if (IS_ERR(drm_st7789v->detect))
		return dev_err_probe(dev, PTR_ERR(drm_st7789v->detect), "Failed to get GPIO 'detect'\n");
	gpiod_direction_input(drm_st7789v->detect);

	//分配内存用于LCD显存
	drm_st7789v->buffer = kzalloc(st7789v_mode.hdisplay*st7789v_mode.vdisplay*2, GFP_KERNEL);
	if(drm_st7789v->buffer == NULL) {
 		printk(KERN_ERR "buffer alloc failed\n");
        return -ENOMEM;
	}

    dma_coerce_mask_and_coherent(dev, DMA_BIT_MASK(32));
    ret = drmm_mode_config_init(drm_dev);
    if(ret) {
        printk(KERN_ERR "drmm_mode_config_init failed\n");  
        return ret;
    }

	connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
	drm_connector_helper_add(connector, &connector_helper_funcs);
    ret = drm_connector_init(drm_dev, connector, &connector_funcs, DRM_MODE_CONNECTOR_SPI);
    if(ret) {
        printk(KERN_ERR "drm_connector_init failed\n");
        return ret;
    }

	ret = drm_simple_display_pipe_init(drm_dev, pipe, &st7789v_pipe_funcs, st7789v_formats, ARRAY_SIZE(st7789v_formats),
					   modifiers, connector);
	if (ret) {
        printk(KERN_ERR "drm_simple_display_pipe_init failed\n");  
        return ret;
    }

    drm_dev->mode_config.funcs = &mode_config_funcs;
	drm_dev->mode_config.min_width = 0;
	drm_dev->mode_config.max_width = st7789v_mode.hdisplay;
	drm_dev->mode_config.min_height = 0;
	drm_dev->mode_config.max_height = st7789v_mode.vdisplay;
	drm_mode_config_reset(drm_dev);
    ret = drm_dev_register(drm_dev, 0);
    if(ret) {
        printk(KERN_ERR "drm_dev register failed\n");
        return ret;
    }
	spi_set_drvdata(spi, drm_st7789v);
	//设置fb的颜色深度为16位，即RGB5654
	//也可以直接赋值drm_dev->mode_config.preferred_depth = 16;
	drm_fbdev_generic_setup(drm_dev, 16);
	drm_kms_helper_poll_init(drm_dev);
    return 0;
}

static void drm_st7789v_remove(struct spi_device *spi)
{
	struct drm_display *drm = spi_get_drvdata(spi);
	spi_set_drvdata(spi, NULL);
	drm_kms_helper_poll_fini(&drm->drm_dev);
	devm_gpiod_put(&spi->dev, drm->dc);
	devm_gpiod_put(&spi->dev, drm->reset);
    drm_dev_unplug(&drm->drm_dev);
	kfree(drm->buffer);
}

static const struct of_device_id st7789v_of_match[] = {
	{
		.compatible = "drm-test",
		.data = NULL,
	},
    { },
};
MODULE_DEVICE_TABLE(of, st7789v_of_match);

static const struct spi_device_id st7789v_id[] = {
	{ "drm-test", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, st7789v_id);

static struct spi_driver st7789v_driver = {
	.probe = drm_st7789v_probe,
	.remove = drm_st7789v_remove,
	.id_table = st7789v_id,
	.driver = {
		.name = "panel-st7789v",
		.of_match_table = st7789v_of_match,
	},
};
module_spi_driver(st7789v_driver);

MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("drm hotplug poll test");
MODULE_LICENSE("GPL v2");
