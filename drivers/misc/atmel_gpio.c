/**
 * SAMA5 GPIO Demo Driver 
 * Author:  Jeffery Cheng
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <mach/board.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include <asm/io.h>
#include <mach/gpio.h>

#include <asm/uaccess.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME "gpio.0"

#define GPIO_PA16   (AT91_PIN_PA16 << 8 | 1)
#define GPIO_PA17   (AT91_PIN_PA17 << 8 | 2)
#define GPIO_PA18   (AT91_PIN_PA18 << 8 | 3)
#define GPIO_PA19   (AT91_PIN_PA19 << 8 | 4)
#define GPIO_PA20   (AT91_PIN_PA20 << 8 | 5)
#define GPIO_PA21   (AT91_PIN_PA21 << 8 | 6)
#define GPIO_PA22   (AT91_PIN_PA22 << 8 | 7)
#define GPIO_PA23   (AT91_PIN_PA23 << 8 | 8)
#define GPIO_PE8    (AT91_PIN_PE8  << 8 | 9)
#define GPIO_PE9    (AT91_PIN_PE9  << 8 | 10)
#define GPIO_PE10   (AT91_PIN_PE10 << 8 | 11)
#define GPIO_PE11   (AT91_PIN_PE11 << 8 | 12)
#define GPIO_PE12   (AT91_PIN_PE12 << 8 | 13)
#define GPIO_PE13   (AT91_PIN_PE13 << 8 | 14)
#define GPIO_PE14   (AT91_PIN_PE14 << 8 | 15)
#define GPIO_PE15   (AT91_PIN_PE15 << 8 | 16)



enum GPIO_TYPE {
    GPIO_OUTPUT_ONLY,
    GPIO_INPUT_ONLY,
    GPIO_BIDIRECTION,
};

/* GPIO_IOCTL_CMD */
#define GPIO_SET_PIN    _IOR('G', 0, int)
#define GPIO_CLR_PIN    _IOR('G', 1, int)
#define GPIO_GET_VALUE  _IOR('G', 2, int)  /* input */

static struct gpio_t {
    unsigned int pin;
    unsigned int val;
    enum GPIO_TYPE type;
} gpio[] = {
    /* output */
    { GPIO_PA16, 0, GPIO_BIDIRECTION, },
    { GPIO_PA17, 0, GPIO_BIDIRECTION, },
    { GPIO_PA18, 0, GPIO_BIDIRECTION, },
    { GPIO_PA19, 0, GPIO_BIDIRECTION, },
    { GPIO_PA20, 0, GPIO_BIDIRECTION, },
    { GPIO_PA21, 0, GPIO_BIDIRECTION, },
    { GPIO_PA22, 0, GPIO_BIDIRECTION, },
    { GPIO_PA23, 0, GPIO_BIDIRECTION, },
    { GPIO_PE8,  0, GPIO_BIDIRECTION, },
    { GPIO_PE9,  0, GPIO_BIDIRECTION, },
    { GPIO_PE10, 0, GPIO_BIDIRECTION, },
    { GPIO_PE11, 0, GPIO_BIDIRECTION, },
    { GPIO_PE12, 0, GPIO_BIDIRECTION, },
    { GPIO_PE13, 0, GPIO_BIDIRECTION, },
    { GPIO_PE14, 0, GPIO_BIDIRECTION, },
    { GPIO_PE15, 0, GPIO_BIDIRECTION, },
};

static int gpio_open (struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t gpio_read (struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
    return sizeof(int);
}

static ssize_t gpio_write (struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    return sizeof(int);
}

static long gpio_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    int retval = 0;
    switch (cmd) {
        case GPIO_SET_PIN: {
            unsigned int pin = arg;
            int i;
            for (i = 0; i < ARRAY_SIZE(gpio); i++) {
                if ((gpio[i].type == GPIO_OUTPUT_ONLY || gpio[i].type == GPIO_BIDIRECTION) && gpio[i].pin == pin) {
                    gpio[i].val = 1;    /* set */
                    gpio_direction_output (pin >> 8, gpio[i].val);
                    break;
                }
            }
        }
        break;
        
        case GPIO_CLR_PIN: {
            unsigned int pin = arg;
            int i;
            for (i = 0; i < ARRAY_SIZE(gpio); i++) {
                if ((gpio[i].type == GPIO_OUTPUT_ONLY || gpio[i].type == GPIO_BIDIRECTION) && gpio[i].pin == pin) {
                    gpio[i].val = 0;    /* clear */
                    gpio_direction_output (pin >> 8, gpio[i].val);
                    break;
                }
            }
        }
        break;

        case GPIO_GET_VALUE: {
            unsigned int pin = (unsigned int)arg;
            int i;
            for (i = 0; i < ARRAY_SIZE(gpio); i++) {
                if (gpio[i].pin == pin) {
                    if (gpio[i].type == GPIO_BIDIRECTION) {
                        at91_set_deglitch (pin >> 8, 1);
                        gpio_direction_input (pin >> 8);
                        msleep_interruptible (10);
                        gpio[i].val = __gpio_get_value (pin >> 8);
                    }
                    else if (gpio[i].type == GPIO_INPUT_ONLY) {
                        gpio[i].val = __gpio_get_value (pin >> 8);
                    }
                    else if (gpio[i].type == GPIO_OUTPUT_ONLY) {
                        /* nop */
                    }
                    else {
                        printk (KERN_INFO "Warning: the pin can't set input\n");
                        break;
                    }
                    retval = gpio[i].val;
                    break;
                }
            }
        }
        break;

        default:
            printk(KERN_ERR "%s: command type unsupport\n", __func__);
        break;
    }

    return retval;
}

static int gpio_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations gpio_fops = {
    .owner  = THIS_MODULE,
    .open   = gpio_open,
    .read   = gpio_read,
    .write  = gpio_write,
    .unlocked_ioctl = gpio_ioctl,
    .release    = gpio_release,
};

static struct miscdevice gpio_miscdev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = DEVICE_NAME,
    .fops   = &gpio_fops,
};

static int __init atmel_gpio_init (void)
{
    int ret = 0;
    unsigned int i;

    ret = misc_register (&gpio_miscdev);
    if (ret) {
        printk (KERN_ERR "cannot register miscdev on minor=%d (%d)\n", MISC_DYNAMIC_MINOR, ret);
        goto out;
    }

    for (i = 0; i < ARRAY_SIZE(gpio); i++) {

        unsigned int pin = gpio[i].pin;

        gpio_request(pin >> 8, "cm_gpio");

        if (gpio[i].type == GPIO_INPUT_ONLY || gpio[i].type == GPIO_BIDIRECTION) {

            at91_set_deglitch (pin >> 8, 1);
            gpio_direction_input (pin >> 8);
        }
        else if (gpio[i].type == GPIO_OUTPUT_ONLY) {
            gpio_direction_output (pin >>8, gpio[i].val);
        }
    }

    printk (KERN_INFO "\natmel %s initialized\n", DEVICE_NAME);

    return 0;

out:
    return ret;
}

static void __exit atmel_gpio_exit (void)
{
    int i;

    printk (KERN_INFO "\natmel %s removed\n", DEVICE_NAME);

    for (i = 0; i < ARRAY_SIZE(gpio); i++) {
        unsigned int pin = gpio[i].pin;
        gpio_free (pin >> 8);
    }

    misc_deregister(&gpio_miscdev);
}

module_init (atmel_gpio_init);
module_exit (atmel_gpio_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Atmel GPIO Device");
