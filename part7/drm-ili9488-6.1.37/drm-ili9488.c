#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>       
#include <linux/kobject.h>  
#include <linux/sysfs.h>
#include <drm/drm_drv.h>
#include <drm/drm_edid.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_gem.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_modeset_helper.h>
#include <drm/drm_plane.h>
#include <drm/drm_fourcc.h>
#include <linux/spi/spi.h>
#include <linux/gpio/consumer.h>
#include <drm/drm_probe_helper.h>
#include <linux/delay.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <linux/string.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_format_helper.h>
#include <linux/iosys-map.h>


#define DISPLAY_XRES_DEFAULT    480
#define DISPLAY_YRES_DEFAULT    320
#define DISPLAY_X_WIDTH         83
#define DISPLAY_X_HEIGHT        55

struct drm_display {
	struct spi_device    *spi;
	struct gpio_desc     *reset;
	struct gpio_desc     *dc;
	struct drm_device     drm_dev;
	struct drm_plane      primary;
	struct drm_crtc       crtc;
	struct drm_encoder    encoder;
	struct drm_connector  connector;
	struct drm_display_mode mode;
	char *buffer;
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

    /*   写寄存器，初始化*/

    fb_write_reg(drm, 0xf7);
    fb_write_data(drm, 0xa9);
    fb_write_data(drm, 0x51);
    fb_write_data(drm, 0x2c);
    fb_write_data(drm, 0x82);

    fb_write_reg(drm, 0xc0);
    fb_write_data(drm, 0x11);
    fb_write_data(drm, 0x09);

    fb_write_reg(drm, 0xc1);
    fb_write_data(drm, 0x41);

    fb_write_reg(drm, 0xc5);
    fb_write_data(drm, 0x00);
    fb_write_data(drm, 0x0a);
    fb_write_data(drm, 0x80);

    fb_write_reg(drm, 0xb1);
    fb_write_data(drm, 0xB0);
    fb_write_data(drm, 0x11);

    fb_write_reg(drm, 0xb4);
    fb_write_data(drm, 0x02);

    fb_write_reg(drm, 0xb6);
    fb_write_data(drm, 0x02);
    fb_write_data(drm, 0x42);

    fb_write_reg(drm, 0xb7);
    fb_write_data(drm, 0xc6);

    fb_write_reg(drm, 0xbe);
    fb_write_data(drm, 0x00);
    fb_write_data(drm, 0x04);

    fb_write_reg(drm, 0xe9);
    fb_write_data(drm, 0x00);

    fb_write_reg(drm, 0x36);
    fb_write_data(drm, (1<<3)|(0<<7)|(1<<6)|(1<<5));

    fb_write_reg(drm, 0x3a);
    fb_write_data(drm, 0x66);

    fb_write_reg(drm, 0xe0);
    fb_write_data(drm, 0x00);
    fb_write_data(drm, 0x07);
    fb_write_data(drm, 0x10);
    fb_write_data(drm, 0x09);
    fb_write_data(drm, 0x17);
    fb_write_data(drm, 0x0b);
    fb_write_data(drm, 0x41);
    fb_write_data(drm, 0x89);
    fb_write_data(drm, 0x4b);
    fb_write_data(drm, 0x0a);
    fb_write_data(drm, 0x0c);
    fb_write_data(drm, 0x0e);
    fb_write_data(drm, 0x18);
    fb_write_data(drm, 0x1b);
    fb_write_data(drm, 0x0f);

    fb_write_reg(drm, 0xe1);
    fb_write_data(drm, 0x00);
    fb_write_data(drm, 0x17);
    fb_write_data(drm, 0x1a);
    fb_write_data(drm, 0x04);
    fb_write_data(drm, 0x0e);
    fb_write_data(drm, 0x06);
    fb_write_data(drm, 0x2f);
    fb_write_data(drm, 0x45);
    fb_write_data(drm, 0x43);
    fb_write_data(drm, 0x02);
    fb_write_data(drm, 0x0a);
    fb_write_data(drm, 0x09);
    fb_write_data(drm, 0x32);
    fb_write_data(drm, 0x36);
    fb_write_data(drm, 0x0f);
    
    fb_write_reg(drm, 0x11);
    mdelay(50);
    fb_write_reg(drm, 0x29);
    mdelay(200);
}

static struct drm_display_mode ili9488_mode = {
	//宽度像素480，高度像素320，宽度尺寸83mm，高度尺寸55mm
	DRM_SIMPLE_MODE(DISPLAY_XRES_DEFAULT, DISPLAY_YRES_DEFAULT, DISPLAY_X_WIDTH, DISPLAY_X_HEIGHT),
};

static const u32 ili9488_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_XRGB8888,
};

static const uint64_t modifiers[] = {
    DRM_FORMAT_MOD_LINEAR,
    DRM_FORMAT_MOD_INVALID
};

static bool drm_format_mod_supported(struct drm_plane *plane,
						uint32_t format,
						uint64_t modifier)
{
	return modifier == DRM_FORMAT_MOD_LINEAR;
}

static const struct drm_plane_funcs plane_funcs = {
	.update_plane		= drm_atomic_helper_update_plane,
	.disable_plane		= drm_atomic_helper_disable_plane,
	.destroy		    = drm_plane_cleanup,
	.reset			    = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state	= drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_plane_destroy_state,
	.format_mod_supported   = drm_format_mod_supported,
};

static int drm_plane_atomic_check(struct drm_plane *plane,
					struct drm_atomic_state *state)
{
	struct drm_display *drm = container_of(plane, struct drm_display, primary);
	struct drm_plane_state *plane_state = drm_atomic_get_new_plane_state(state, plane);
	struct drm_crtc_state *crtc_state;
	int ret;

	crtc_state = drm_atomic_get_new_crtc_state(state, &drm->crtc);

	ret = drm_atomic_helper_check_plane_state(plane_state, crtc_state,
						  DRM_PLANE_NO_SCALING,
						  DRM_PLANE_NO_SCALING,
						  false, false);
	return ret;
}

static void drm_plane_atomic_update(struct drm_plane *plane,
					struct drm_atomic_state *state)
{
	int ret;
	int idx;
	struct drm_rect rect;
	struct drm_plane_state *old_pstate,*plane_state;
	struct iosys_map map[DRM_FORMAT_MAX_PLANES];
	struct iosys_map data[DRM_FORMAT_MAX_PLANES];
	struct drm_display *drm = container_of(plane, struct drm_display, primary);
	struct iosys_map dst_map = IOSYS_MAP_INIT_VADDR(drm->buffer);
	plane_state = drm_atomic_get_new_plane_state(state, plane);
	old_pstate = drm_atomic_get_old_plane_state(state, plane);
	drm_atomic_helper_damage_merged(old_pstate, plane_state, &rect);
	if (!drm_dev_enter(plane->dev, &idx))
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

static const struct drm_plane_helper_funcs plane_helper_funcs = {
	.prepare_fb = drm_gem_plane_helper_prepare_fb,
	.atomic_check = drm_plane_atomic_check,
	.atomic_update = drm_plane_atomic_update,
};

static const struct drm_mode_config_funcs mode_config_funcs = {
	.fb_create = drm_gem_fb_create_with_dirty,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static enum drm_mode_status crtc_helper_mode_valid(struct drm_crtc *crtc,
			       const struct drm_display_mode *mode)
{
	return drm_crtc_helper_mode_valid_fixed(crtc, mode, &ili9488_mode);
}

static int drm_atomic_crtc_check(struct drm_crtc *crtc,
				     struct drm_atomic_state *state)
{
	struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state(state, crtc);
	int ret;

	ret = drm_atomic_helper_check_crtc_state(crtc_state, false);
	if (ret)
		return ret;

	return drm_atomic_add_affected_planes(state, crtc);
}

static void drm_atomic_crtc_enable(struct drm_crtc *crtc,
				       struct drm_atomic_state *state)
{
	struct drm_display *drm = container_of(crtc, struct drm_display, crtc);
	myfb_init(drm);
}

static void drm_atomic_crtc_disable(struct drm_crtc *crtc,
					struct drm_atomic_state *state)
{
	//这里可以不做任何事情
}

static const struct drm_crtc_helper_funcs crtc_helper_funcs = {
	.mode_valid = crtc_helper_mode_valid,
	.atomic_check = drm_atomic_crtc_check,
	.atomic_enable = drm_atomic_crtc_enable,
	.atomic_disable = drm_atomic_crtc_disable,
};

static const struct drm_crtc_funcs crtc_funcs = {
	.reset = drm_atomic_helper_crtc_reset,
	.destroy = drm_crtc_cleanup,
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
};

static const struct drm_encoder_funcs encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static int connector_helper_get_modes(struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &ili9488_mode);
}

static const struct drm_connector_helper_funcs connector_helper_funcs = {
	.get_modes = connector_helper_get_modes,
};

static const struct drm_connector_funcs connector_funcs = {
	.reset = drm_atomic_helper_connector_reset,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
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
	.name			= "drm-ili9488",
	.desc			= "drm ili9488 display",
	.date			= "20240202",
	.major			= 1,
	.minor			= 0,
};

static int hd;
static int vd;
static int hd_mm;
static int vd_mm;
module_param_named(hd,hd,int,0644);
module_param_named(vd,vd,int,0644);
module_param_named(hd_mm,hd_mm,int,0644);
module_param_named(vd_mm,vd_mm,int,0644);
MODULE_PARM_DESC(hd, "Horizontal resolution, width");
MODULE_PARM_DESC(vd, "Vertical resolution, height");
MODULE_PARM_DESC(hd_mm, "Display width in millimeters");
MODULE_PARM_DESC(vd_mm, "Display height in millimeters");

static int drm_ili9488_probe(struct spi_device *spi)
{
	int ret;
	struct drm_display *drm_ili9488;
	struct device *dev = &spi->dev;
	struct drm_device *drm_dev;
	struct drm_plane  *primary;
	struct drm_crtc   *crtc;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	u32 width, height, width_mm, height_mm;

	drm_ili9488 = devm_drm_dev_alloc(dev, &test_driver, struct drm_display, drm_dev);
	if(drm_ili9488 == NULL) {
		printk(KERN_ERR "struct drm_display alloc filed\n");
		return -ENOMEM;
	}
	drm_ili9488->reset = devm_gpiod_get(dev,"reset",GPIOD_OUT_HIGH);
	if (IS_ERR(drm_ili9488->reset))
		return dev_err_probe(dev, PTR_ERR(drm_ili9488->reset), "Failed to get GPIO 'reset'\n");

	drm_ili9488->dc = devm_gpiod_get(dev,"dc",GPIOD_OUT_LOW);
	if (IS_ERR(drm_ili9488->dc))
		return dev_err_probe(dev, PTR_ERR(drm_ili9488->dc), "Failed to get GPIO 'dc'\n");
	gpiod_direction_output(drm_ili9488->reset, 0);
	gpiod_direction_output(drm_ili9488->dc, 0);

	if(device_property_read_u32(dev, "width", &width) ||
		device_property_read_u32(dev, "height", &height) ||
		device_property_read_u32(dev, "width-mm", &width_mm) ||
		device_property_read_u32(dev, "height-mm", &height_mm)
	) {
		//设备树中没有width属性和height属性以及width-mm属性和height-mm
		if((hd > 0) && (vd > 0) && (hd_mm > 0) && (vd_mm > 0)) { //获取模块参数
			ili9488_mode.hdisplay = hd;
			ili9488_mode.vdisplay = vd;
			ili9488_mode.width_mm = hd_mm;
			ili9488_mode.height_mm = vd_mm;
			dev_info(dev, "Horizontal resolution         :%d", hd);
			dev_info(dev, "Vertical resolution           :%d", vd);
			dev_info(dev, "Display width in millimeters  :%d", hd_mm);
			dev_info(dev, "Display height in millimeters :%d", vd_mm);
		}
		else {
			dev_info(dev, "[Default] Horizontal resolution         :DISPLAY_XRES_DEFAULT");
			dev_info(dev, "[Default] Vertical resolution           :DISPLAY_YRES_DEFAULT");
			dev_info(dev, "[Default] Display width in millimeters  :DISPLAY_X_WIDTH");
			dev_info(dev, "[Default] Display height in millimeters :DISPLAY_X_HEIGHT");
		}
	}
	else {
		ili9488_mode.hdisplay = width;
		ili9488_mode.vdisplay = height;
		ili9488_mode.width_mm = width_mm;
		ili9488_mode.height_mm = height_mm;
		dev_info(dev, "Horizontal resolution         :%d", width);
		dev_info(dev, "Vertical resolution           :%d", height);
		dev_info(dev, "Display width in millimeters  :%d", width_mm);
		dev_info(dev, "Display height in millimeters :%d", height_mm);
	}

	drm_ili9488->spi = spi;
	drm_dev = &drm_ili9488->drm_dev;
	//分配内存用于LCD显存
	drm_ili9488->buffer = kzalloc(ili9488_mode.hdisplay * ili9488_mode.vdisplay * 2, GFP_KERNEL);
	if(drm_ili9488->buffer == NULL) {
 		printk(KERN_ERR "buffer alloc failed\n");
        return -ENOMEM;
	}
	primary   = &(drm_ili9488->primary);
	crtc      = &(drm_ili9488->crtc);
	encoder   = &(drm_ili9488->encoder);
	connector = &(drm_ili9488->connector);

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
  
	drm_plane_helper_add(primary, &plane_helper_funcs);
    ret = drm_universal_plane_init(drm_dev, primary, 0, &plane_funcs,
				                ili9488_formats, ARRAY_SIZE(ili9488_formats),
				                modifiers, DRM_PLANE_TYPE_PRIMARY, NULL);
    if(ret) {
        printk(KERN_ERR "drm_universal_plane_init failed\n");
        return ret;
    }
 
	drm_crtc_helper_add(crtc, &crtc_helper_funcs);
    ret = drm_crtc_init_with_planes(drm_dev, crtc, primary, NULL,
					&crtc_funcs, NULL);
    if(ret) {
        printk(KERN_ERR "drm_crtc_init_with_planes failed\n");
        return ret;
    }
	encoder->possible_crtcs = drm_crtc_mask(crtc);
    ret = drm_encoder_init(drm_dev, encoder, &encoder_funcs, DRM_MODE_ENCODER_NONE, NULL);
    if(ret) {
        printk(KERN_ERR "drm_encoder_init failed\n");
        return ret;
    }
 
	ret = drm_connector_attach_encoder(connector, encoder);
	if (ret) {
		dev_err(dev, "DRM attach connector to encoder failed: %d\n", ret);
		return ret;
	}

	drm_plane_enable_fb_damage_clips(primary);

	drm_dev->mode_config.funcs = &mode_config_funcs;
	drm_dev->mode_config.min_width = ili9488_mode.hdisplay;
	drm_dev->mode_config.max_width = ili9488_mode.hdisplay;
	drm_dev->mode_config.min_height = ili9488_mode.vdisplay;
	drm_dev->mode_config.max_height = ili9488_mode.vdisplay;

	drm_mode_config_reset(drm_dev);
    ret = drm_dev_register(drm_dev, 0);
    if(ret) {
        printk(KERN_ERR "drm_dev register failed\n");
        return ret;
    }
	spi_set_drvdata(spi, drm_ili9488);
	//设置fb的颜色深度为16位，即RGB5654
	//也可以直接赋值drm_dev->mode_config.preferred_depth = 16;
	drm_fbdev_generic_setup(drm_dev, 16);
	return 0;
}

static void drm_ili9488_remove(struct spi_device *spi)
{
	struct drm_display *drm_ili9488 = spi_get_drvdata(spi);
	spi_set_drvdata(spi, NULL);
	devm_gpiod_put(&spi->dev, drm_ili9488->dc);
	devm_gpiod_put(&spi->dev, drm_ili9488->reset);
    drm_dev_unplug(&drm_ili9488->drm_dev);
	kfree(drm_ili9488->buffer);
}

static const struct of_device_id drm_test_match[] = {
	{ .compatible = "ilitek,ili9488" },
	{},
};

static struct spi_driver drm_driver = {
	.driver = {
		.name = "ili9488",
		.of_match_table = drm_test_match,
	},
	.probe = drm_ili9488_probe,
	.remove = drm_ili9488_remove,
};

module_spi_driver(drm_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");   
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("drm ili9488 display"); 
