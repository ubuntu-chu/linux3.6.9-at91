#include <linux/module.h>
#include <linux/init.h>
#include <linux/atomic.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/fs.h>  
#include <asm/uaccess.h> 

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/freezer.h>

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/mman.h>
#include <linux/types.h>
#include <linux/leds.h>

static struct gpio_led indicator_leds[] = {
	{	
		.name				= "system",
		.gpio				= AT91_PIN_PB14,
		.default_trigger	= "heartbeat",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
		.active_low			= 1,
	},
	{	
		.name				= "run",
		.gpio				= AT91_PIN_PB13,
		.default_trigger	= "gpio",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
		.active_low			= 1,
	},
	{	
		.name				= "alarm",
		.gpio				= AT91_PIN_PB12,
		.default_trigger	= "gpio",
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
		.active_low			= 1,
	},
};

static struct gpio_led_platform_data indicator_data = {

	.leds					= indicator_leds,
	.num_leds				= ARRAY_SIZE(indicator_leds),
};

static struct platform_device	indicator_device = {
	.name					= "leds-gpio",
	.id						= -1,
	.dev.platform_data		= &indicator_data,
};

/* Driver init function */
static int __init indicator_init(void)
{
	return	platform_device_register(&indicator_device);
}

/* Driver exit function */
static void __exit indicator_exit(void)
{
	platform_device_unregister(&indicator_device);
}

/* register after spi postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall(indicator_init);
module_exit(indicator_exit);

MODULE_AUTHOR("jspower");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("indicator module");
MODULE_ALIAS("indicator");

