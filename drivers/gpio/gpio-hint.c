#include <linux/kernel.h>
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
#include <linux/spi/spi.h>
#include <linux/freezer.h>

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/mman.h>
#include <linux/types.h>

#define    P_DEBUG_SWITCH        (0)
 
#if    (P_DEBUG_SWITCH > 0)
    #define P_DEBUG_DEV(dev, fmt, args...)   printk("<1>" "<kernel>[%s %s %s(%d):%s]"fmt,\
			                        dev_driver_string(dev), \
			                        dev_name(dev), __FILE__, __LINE__,__FUNCTION__, ##args)
    #define P_DEBUG(fmt, args...)   printk("<0>" "<kernel>[%s(%d):%s]"fmt, __FILE__, __LINE__, \
			                        __FUNCTION__, ##args)
    #define P_DEBUG_SIMPLE(fmt, args...)   printk("<0>" "<kernel>[%s]"fmt,  \
			__FUNCTION__, ##args)

	#define P_DEBUG_PORT(port, fmt, args...)    do{ if (port->line > 1){\
				printk("<1>" "<kernel>[%s(%d):%s]"fmt, __FILE__, __LINE__, \
						__FUNCTION__, ##args);}\
				}while(0)
#else
    #define P_DEBUG_DEV(dev, fmt, args...)
    #define P_DEBUG(fmt, args...)
	#define P_DEBUG_SIMPLE(fmt, args...)
	#define P_DEBUG_PORT(port, fmt, args...)    
#endif

struct gpio_detect{
	int			pc29;
	int			pc28;
	int			pc27;
	int			pc26;
};

static ssize_t pc29_value_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t			status;
	int value;
	struct gpio_detect	*pgpio_detect	= (struct gpio_detect *)dev->platform_data;
	int pc29				= pgpio_detect->pc29;

	value = !!gpio_get_value_cansleep(pc29);
	status = sprintf(buf, "%d\n", value);

	return status;
}

static ssize_t pc29_value_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t			status;
	struct gpio_detect	*pgpio_detect	= (struct gpio_detect *)dev->platform_data;
	int pc29				= pgpio_detect->pc29;
	long		value;

	status = strict_strtol(buf, 0, &value);
	if (status == 0) {
		gpio_set_value_cansleep(pc29, value != 0);
		status = size;
	}

	return status;
}

static const DEVICE_ATTR(pc29_value, 0444,
	pc29_value_show, pc29_value_store);

static int __init gpio_detect_probe(struct platform_device *pdev)
{
	int ret;
	unsigned gpio;
	struct gpio_detect	*pgpio_detect	= (struct gpio_detect *)pdev->dev.platform_data;
	char *name		= "gpio detect";

	P_DEBUG_SIMPLE("\n");
	if (NULL == pgpio_detect){
		printk(KERN_INFO "gpio must not be null\n");
		return 0;
	}
	gpio	= pgpio_detect->pc29;

	if (!gpio_is_valid(gpio)) {
		printk(KERN_INFO "unavailable gpio %d (%s)\n", gpio, name);
		return 0;
	}

	ret = gpio_request(gpio, name);
	if (ret < 0){
		printk(KERN_INFO "gpio %d (%s) request failed\n", gpio, name);
		return ret;
	}
	P_DEBUG_SIMPLE("gpio %d (%s) request sucess\n", gpio, name);

	ret = gpio_direction_input(gpio);
	if (ret < 0)
		goto err;
		
	ret = device_create_file(&(pdev->dev), &dev_attr_pc29_value);
	if (ret != 0){
		printk(KERN_INFO "device pc29 value create failed\n");
		goto err;
	}

	return 0;
err:
	gpio_free(gpio);
	return ret;
}

static int __exit gpio_detect_remove(struct platform_device *pdev)
{
	device_remove_file(&(pdev->dev), &dev_attr_pc29_value);
	return 0;
}

static struct gpio_detect		t_gpio_detect = {

	.pc29				= AT91_PIN_PC29,
	.pc28				= AT91_PIN_PC28,
	.pc27				= AT91_PIN_PC27,
	.pc26				= AT91_PIN_PC26,
};

static struct platform_device gpio_detect_device = {
	.name		= "gpio_detect",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &t_gpio_detect,
	}
};

static struct platform_driver gpio_detect_driver = {
	.driver = {
		.name = "gpio_detect",
		.owner = THIS_MODULE,
	},
	.probe = gpio_detect_probe,
	.remove = __exit_p(gpio_detect_remove),
};

/* Driver init function */
static int __init gpio_detect_init(void)
{
	P_DEBUG_SIMPLE("\n");
	platform_device_register(&gpio_detect_device);
	P_DEBUG_SIMPLE("call platform driver probe");

	return platform_driver_register(&gpio_detect_driver);
}

/* Driver exit function */
static void __exit gpio_detect_exit(void)
{
	P_DEBUG_SIMPLE("\n");
	platform_driver_unregister(&gpio_detect_driver);
}

/* register after spi postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall(gpio_detect_init);
module_exit(gpio_detect_exit);

MODULE_AUTHOR("jspower");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gpio_detect driver");
MODULE_ALIAS("spi lcd:DRIVER_NAME");
