/*
 *  Setup code for AT91SAM Evaluation Kits with Device Tree support
 *
 *  Copyright (C) 2011 Atmel,
 *                2011 Nicolas Ferre <nicolas.ferre@atmel.com>
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/phy.h>
#include <linux/micrel_phy.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>

#include <mach/board.h>
#include <mach/at91_aic.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include "generic.h"
#include "clock.h"

/************************************/
/* TEMPORARY NON-DT STUFF FOR MIURA */
/************************************/
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/fb.h>

#include <video/atmel_lcdfb.h>
#include <mach/atmel_hlcdc.h>

#include <media/soc_camera.h>
#include <media/atmel-isi.h>

#include <mach/sama5d3.h>

/*
 * LCD Controller
 */
static struct fb_videomode at91_tft_vga_modes[] = {
	{
		.name		= "LG",
		.refresh	= 60,
		.xres		= 480,		.yres		= 272,
		.pixclock	= KHZ2PICOS(10000),

		.left_margin	= 1,		.right_margin	= 1,
		.upper_margin	= 40,		.lower_margin	= 1,
		.hsync_len	= 45,		.vsync_len	= 1,

		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},	{
		.name		= "LG",
		.refresh	= 60,
		.xres		= 640,		.yres		= 480,
		.pixclock	= KHZ2PICOS(28214),

		.left_margin	= 145,		.right_margin	= 20,
		.upper_margin	= 14,		.lower_margin	= 8,
		.hsync_len	= 5,		.vsync_len	= 1,

		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	}, {
		.name		= "LG",
		.refresh	= 60,
		.xres		= 800,		.yres		= 480,
		.pixclock	= KHZ2PICOS(30000),

		.left_margin	= 80,		.right_margin	= 15,
		.upper_margin	= 10,		.lower_margin	= 7,
		.hsync_len	= 5,		.vsync_len	= 1,

		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	}, {   /* not check yet */
		.name		= "LG",
		.refresh	= 60,
		.xres		= 800,		.yres		= 600,
		.pixclock	= KHZ2PICOS(40000),

		.left_margin	= 80,		.right_margin	= 15,
		.upper_margin	= 10,		.lower_margin	= 7,
		.hsync_len	= 5,		.vsync_len	= 1,

		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	}, {   /* not check yet */
		.name		= "LG",
		.refresh	= 60,
		.xres		= 1024,		.yres		= 768,
		.pixclock	= KHZ2PICOS(40000),

		.left_margin	= 80,		.right_margin	= 15,
		.upper_margin	= 10,		.lower_margin	= 7,
		.hsync_len	= 5,		.vsync_len	= 1,

		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
};

static struct fb_monspecs at91fb_default_monspecs = {
	.manufacturer	= "LG",
	.monitor	= "LB043WQ1",

	.modedb		= at91_tft_vga_modes,
	.modedb_len	= ARRAY_SIZE(at91_tft_vga_modes),
	.hfmin		= 15000,
	.hfmax		= 17640,
	.vfmin		= 57,
	.vfmax		= 67,
};

/* Default output mode is TFT 24 bit */
#define BPP_OUT_DEFAULT_LCDCFG5	(LCDC_LCDCFG5_MODE_OUTPUT_24BPP)
static void at91_lcdfb_power_control(int on)
{
	static int init_flags = 0;
	if (!init_flags) {
		if (gpio_request(AT91_PIN_PA25, "lcd power")==0) {
			gpio_direction_output(AT91_PIN_PA25, on?0:1);
			init_flags = 1;
		}  
	} else {		
		gpio_set_value(AT91_PIN_PA25, on?0:1);
	}
}

/* Driver datas */
static struct atmel_lcdfb_info __initdata ek_lcdc_data = {
	.lcdcon_is_backlight		= true,
	.alpha_enabled			= false,
	.default_bpp			= 16,
	/* Reserve enough memory for 32bpp */
	.smem_len			= 800 * 480 * 4,
	/* default_lcdcon2 is used for LCDCFG5 */
	.default_lcdcon2		= BPP_OUT_DEFAULT_LCDCFG5,
	.default_monspecs		= &at91fb_default_monspecs,
	.guard_time			= 9,
	.lcd_wiring_mode		= ATMEL_LCDC_WIRING_RGB,
	.atmel_lcdfb_power_control = at91_lcdfb_power_control,
};

/*
 *  ISI
 */
static struct isi_platform_data __initdata isi_data = {
	.frate			= ISI_CFG1_FRATE_CAPTURE_ALL,
	/* to use codec and preview path simultaneously */
	.full_mode		= 1,
	.data_width_flags	= ISI_DATAWIDTH_8 | ISI_DATAWIDTH_10,
	/* ISI_MCK is provided by programmable clock or external clock */
	.mck_hz			= 25000000,
};

static struct clk_lookup isi_mck_lookups[] = {
	CLKDEV_CON_DEV_ID("isi_mck", "atmel_isi", NULL),
};

void __init at91_config_isi(bool use_pck_as_mck, const char *pck_id)
{
	struct clk *pck;
	struct clk *parent;

	if (use_pck_as_mck) {
		pck = clk_get(NULL, pck_id);
		parent = clk_get(NULL, "plla");

		BUG_ON(IS_ERR(pck) || IS_ERR(parent));

		if (clk_set_parent(pck, parent)) {
			pr_err("Failed to set PCK's parent\n");
		} else {
			/* Register PCK as ISI_MCK */
			isi_mck_lookups[0].clk = pck;
			clkdev_add_table(isi_mck_lookups,
				ARRAY_SIZE(isi_mck_lookups));
		}

		clk_put(pck);
		clk_put(parent);
	}
}

/*
 * soc-camera OV2640
 */
#if defined(CONFIG_SOC_CAMERA_OV2640) || \
	defined(CONFIG_SOC_CAMERA_OV2640_MODULE)
static unsigned long isi_camera_query_bus_param(struct soc_camera_link *link)
{
	/* ISI board for ek using default 8-bits connection */
	return SOCAM_DATAWIDTH_8;
}

static int i2c_camera_power_revB(struct device *dev, int on)
{
	int res, ret = 0;

	pr_debug("%s: %s the camera\n", __func__, on ? "ENABLE" : "DISABLE");

	res = devm_gpio_request(dev, AT91_PIN_PE29, "ov2640_power");
	if (res < 0) {
		printk("can't request ov2640_power pin\n");
		return -1;
	}

	res = devm_gpio_request(dev, AT91_PIN_PE28, "ov2640_reset");
	if (res < 0) {
		printk("can't request ov2640_reset pin\n");
		devm_gpio_free(dev, AT91_PIN_PE29);
		return -1;
	}

	/* enable or disable the camera */
	res = gpio_direction_output(AT91_PIN_PE29, !on);
	if (res < 0) {
		printk("can't request output direction for ov2640_power pin\n");
		ret = -1;
		goto out;
	}

	if (!on)
		goto out;

	/* If enabled, give a reset impulse */
	res = gpio_direction_output(AT91_PIN_PE28, 0);
	if (res < 0) {
		printk("can't request output direction for ov2640_reset pin\n");
		ret = -1;
		goto out;
	}
	msleep(20);
	res = gpio_direction_output(AT91_PIN_PE28, 1);
	if (res < 0) {
		printk("can't request output direction for ov2640_reset pin\n");
		ret = -1;
		goto out;
	}
	msleep(100);

out:
	devm_gpio_free(dev, AT91_PIN_PE28);
	devm_gpio_free(dev, AT91_PIN_PE29);
	return ret;
}

static int i2c_camera_power(struct device *dev, int on)
{
	int res, ret = 0;

	pr_debug("%s: %s the camera\n", __func__, on ? "ENABLE" : "DISABLE");

	res = devm_gpio_request(dev, AT91_PIN_PE29, "ov2640_power");
	if (res < 0) {
		printk("can't request ov2640_power pin\n");
		return -1;
	}

	res = devm_gpio_request(dev, AT91_PIN_PE24, "ov2640_reset");
	if (res < 0) {
		printk("can't request ov2640_reset pin\n");
		devm_gpio_free(dev, AT91_PIN_PE29);
		return -1;
	}

	/* enable or disable the camera */
	res = gpio_direction_output(AT91_PIN_PE29, !on);
	if (res < 0) {
		printk("can't request output direction for ov2640_power pin\n");
		ret = -1;
		goto out;
	}

	if (!on)
		goto out;

	/* If enabled, give a reset impulse */
	res = gpio_direction_output(AT91_PIN_PE24, 0);
	if (res < 0) {
		printk("can't request output direction for ov2640_reset pin\n");
		ret = -1;
		goto out;
	}
	msleep(20);
	res = gpio_direction_output(AT91_PIN_PE24, 1);
	if (res < 0) {
		printk("can't request output direction for ov2640_reset pin\n");
		ret = -1;
		goto out;
	}
	msleep(100);

out:
	devm_gpio_free(dev, AT91_PIN_PE24);
	devm_gpio_free(dev, AT91_PIN_PE29);
	return ret;
}

static struct i2c_board_info i2c_camera = {
	I2C_BOARD_INFO("ov2640", 0x30),
};

static struct soc_camera_link iclink_ov2640 = {
	.bus_id			= -1,
	.board_info		= &i2c_camera,
	.i2c_adapter_id		= 0,
	.power			= i2c_camera_power,
	.query_bus_param	= isi_camera_query_bus_param,
};

static struct platform_device isi_ov2640 = {
	.name	= "soc-camera-pdrv",
	.id	= 0,
	.dev	= {
		.platform_data = &iclink_ov2640,
	},
};

static struct platform_device *devices[] __initdata = {
	&isi_ov2640,
};
#else
static struct soc_camera_link iclink_ov2640;
static int i2c_camera_power_revB(struct device *dev, int on)
{
	return -1;
}
#endif

struct of_dev_auxdata at91_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("atmel,at91sam9x5-lcd", 0xf8038000, "atmel_hlcdfb_base", &ek_lcdc_data),
	OF_DEV_AUXDATA("atmel,at91sam9x5-lcd", 0xf8038100, "atmel_hlcdfb_ovl1", &ek_lcdc_data),
	OF_DEV_AUXDATA("atmel,at91sam9x5-lcd", 0xf0030000, "atmel_hlcdfb_base", &ek_lcdc_data),
	OF_DEV_AUXDATA("atmel,at91sam9x5-lcd", 0xf0030140, "atmel_hlcdfb_ovl1", &ek_lcdc_data),
	OF_DEV_AUXDATA("atmel,at91sam9x5-lcd", 0xf0030240, "atmel_hlcdfb_ovl2", &ek_lcdc_data),
	OF_DEV_AUXDATA("atmel,at91sam9g45-isi", 0xf0034000, "atmel_isi", &isi_data),
	{ /* sentinel */ }
};

/************************************/
/*              END                 */
/************************************/

static const struct of_device_id irq_of_match[] __initconst = {

	{ .compatible = "atmel,at91rm9200-aic", .data = at91_aic_of_init },
	{ .compatible = "atmel,sama5d3-aic", .data = at91_aic5_of_init },
	{ /*sentinel*/ }
};

static void __init at91_dt_init_irq(void)
{
	of_irq_init(irq_of_match);
}

static int ksz9021rn_phy_fixup(struct phy_device *phy)
{
	int value;

#define GMII_RCCPSR	260
#define GMII_RRDPSR	261
#define GMII_ERCR	11
#define GMII_ERDWR	12

	/* Set delay values */
	value = GMII_RCCPSR | 0x8000;
	phy_write(phy, GMII_ERCR, value);
	value = 0xF2F4;
	phy_write(phy, GMII_ERDWR, value);
	value = GMII_RRDPSR | 0x8000;
	phy_write(phy, GMII_ERCR, value);
	value = 0x2222;
	phy_write(phy, GMII_ERDWR, value);

	return 0;
}

static void __init at91_dt_device_init(void)
{
	char mb_rev = 255;
	int ret;

	printk("<0>""nihao-------------------\n");
	if (of_machine_is_compatible("atmel,sama5ek")) {
		struct device_node *np;

		phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK,
					   ksz9021rn_phy_fixup);

		np = of_find_node_by_path("/");
		if (np) {
			const char *mb_rev_tmp;
			ret = of_property_read_string(np, "atmel,mb-rev", &mb_rev_tmp);
			if (ret) {
				printk("AT91: error %d while looking for mb-rev property, "
				       "let assume we are using the latest one\n", ret);
			} else {
				printk("AT91: mb rev: %s\n", mb_rev_tmp);
				mb_rev = mb_rev_tmp[0];
			}
		}

		np = of_find_compatible_node(NULL, NULL, "atmel,at91sam9g45-isi");
		if (np) {
			if (of_device_is_available(np)) {
				switch (mb_rev) {
				case 'A':
				case 'B':
					at91_config_isi(true, "pck2");
					iclink_ov2640.power = i2c_camera_power_revB;
					break;
				default:
					at91_config_isi(true, "pck1");
					break;
				}
			}
		}

		np = of_find_compatible_node(NULL, NULL, "atmel,atmel_mxt_ts");
		if (np) {
			if (of_device_is_available(np)) {
				__u8 manufacturer[4] = "Inlx";
				__u8 monitor[14] = "AT043TN24";
				/* set mXT224 and QT1070 IRQ lines as inputs */
				gpio_direction_input(AT91_PIN_PE31);
				gpio_direction_input(AT91_PIN_PE30);
				/* set LCD configuration */
				ek_lcdc_data.smem_len = 480 * 272 * 4;
				memcpy(ek_lcdc_data.default_monspecs->manufacturer, manufacturer, 4);
				memcpy(ek_lcdc_data.default_monspecs->monitor, monitor, 14);
				ek_lcdc_data.default_monspecs->hfmin = 14876;
				ek_lcdc_data.default_monspecs->hfmax = 17142;
				ek_lcdc_data.default_monspecs->vfmin = 50;
				ek_lcdc_data.default_monspecs->vfmax = 67;
				ek_lcdc_data.default_monspecs->modedb->name = "Inlx";
				ek_lcdc_data.default_monspecs->modedb->xres = 480;
				ek_lcdc_data.default_monspecs->modedb->yres = 272;
				ek_lcdc_data.default_monspecs->modedb->pixclock = KHZ2PICOS(9000);
				ek_lcdc_data.default_monspecs->modedb->left_margin = 2;
				ek_lcdc_data.default_monspecs->modedb->right_margin = 2;
				ek_lcdc_data.default_monspecs->modedb->upper_margin = 2;
				ek_lcdc_data.default_monspecs->modedb->lower_margin = 2;
				ek_lcdc_data.default_monspecs->modedb->hsync_len = 41;
				ek_lcdc_data.default_monspecs->modedb->vsync_len = 11;
			}
		}
	}

	of_platform_populate(NULL, of_default_bus_match_table, at91_auxdata_lookup, NULL);
#if defined(CONFIG_SOC_CAMERA_OV2640) \
	|| defined(CONFIG_SOC_CAMERA_OV2640_MODULE)
	/* add ov2640 camera device */
	platform_add_devices(devices, ARRAY_SIZE(devices));
#endif
}

static const char *sama5_dt_board_compat[] __initdata = {
	"atmel,sama5ek",
	NULL
};

DT_MACHINE_START(sama5_dt, "Atmel SAMA5 (Device Tree)")
	/* Maintainer: Atmel */
	.timer		= &at91sam926x_timer,
	.map_io		= at91_map_io,
	.handle_irq	= at91_aic5_handle_irq,
	.init_early	= at91_dt_initialize,
	.init_irq	= at91_dt_init_irq,
	.init_machine	= at91_dt_device_init,
	.dt_compat	= sama5_dt_board_compat,
MACHINE_END

static const char *at91_dt_board_compat[] __initdata = {
	"atmel,at91sam9",
	NULL
};

DT_MACHINE_START(at91sam_dt, "Atmel AT91SAM (Device Tree)")
	/* Maintainer: Atmel */
	.timer		= &at91sam926x_timer,
	.map_io		= at91_map_io,
	.handle_irq	= at91_aic_handle_irq,
	.init_early	= at91_dt_initialize,
	.init_irq	= at91_dt_init_irq,
	.init_machine	= at91_dt_device_init,
	.dt_compat	= at91_dt_board_compat,
MACHINE_END
