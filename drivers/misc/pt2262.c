#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <mach/board.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>

#include <asm/io.h>
#include <asm/uaccess.h>


#define MAX_BYTES 256
#define PT2262_SET_SLAVE_ADDRESS		(1)

#define NARROW_TIME	(4*130*1000)
#define WIDE_TIME	(12*130*1000)
#define SYSNC_BIT_TIME ((124*130)/1000)

struct pt2262_platform_data {
	unsigned int	sda_pin;
	int address;
	int udelay;
};

static DEFINE_MUTEX(pt2262_mtx);
static struct pt2262_platform_data *pt2262_data;

static inline void pt2262_set_pulse_width(struct pt2262_platform_data *pdata, int time0, int time1)
{
	gpio_set_value(pdata->sda_pin, 1);
	ndelay(time0);
	gpio_set_value(pdata->sda_pin, 0);
	ndelay(time1);
}

static inline void pt2262_write_sync_bit(struct pt2262_platform_data *pdata)
{
	gpio_set_value(pdata->sda_pin, 1);
	ndelay(NARROW_TIME);
	gpio_set_value(pdata->sda_pin, 0);
	mdelay(SYSNC_BIT_TIME+1);
}


static void pt2262_write_data_bit(struct pt2262_platform_data *pdata, int state)
{
	if (state==0) {
		pt2262_set_pulse_width(pdata, NARROW_TIME, WIDE_TIME);
		pt2262_set_pulse_width(pdata, NARROW_TIME, WIDE_TIME);
	} else if(state == 1) {
		pt2262_set_pulse_width(pdata, WIDE_TIME, NARROW_TIME);
		pt2262_set_pulse_width(pdata, WIDE_TIME, NARROW_TIME);
	} else {
		pt2262_set_pulse_width(pdata, NARROW_TIME, WIDE_TIME);
		pt2262_set_pulse_width(pdata, WIDE_TIME, NARROW_TIME);
	}
}

static void pt2262_write_byte(u8 byte)
{
	struct pt2262_platform_data *pdata = pt2262_data;
	int i, j;

	for (j=0; j<4; j++) {
		int state = (byte<<4)|(pdata->address&0x0f);
		
		for (i=0; i<12; i++) {
			pt2262_write_data_bit(pdata, (state&0x1));
			state = state >> 1;
		}
		
		pt2262_write_sync_bit(pdata);		
	}
}

static ssize_t pt2262_write (struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	u8 kbuf[MAX_BYTES];
	u8 *tmp;
	ssize_t length = 0;

	mutex_lock(&pt2262_mtx);
    if (copy_from_user(kbuf, buffer, count))
		return -EFAULT;

	for (tmp = kbuf; count--; tmp++) {
		pt2262_write_byte(*tmp);
	}

	length = tmp - kbuf;
	mutex_unlock(&pt2262_mtx);
	
	return length;
}

static long pt2262_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
	case PT2262_SET_SLAVE_ADDRESS:
		if (copy_from_user(&pt2262_data->address, (const void *)arg, sizeof(int))) {
			return -EFAULT;
		}
		break;
	default:
		break;
	}
		
    return 0;
}

static int pt2262_open (struct inode *inode, struct file *file)
{
    return 0;
}

static int pt2262_release (struct inode *inode, struct file *filp)
{
    return 0;
}


static const struct file_operations pt2262_fops = {
    .owner  = THIS_MODULE,
    .open   = pt2262_open,
    .write  = pt2262_write,
    .unlocked_ioctl = pt2262_ioctl,
    .release    = pt2262_release,
};

static struct miscdevice pt2262_miscdev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "pt2262",
    .fops   = &pt2262_fops,
};

static int __devinit of_pt2262_gpio_probe(struct device_node *np,
			     struct pt2262_platform_data *pdata)
{
	if (of_gpio_count(np) < 1)
		return -ENODEV;

	pdata->sda_pin = of_get_gpio(np, 0);
	
	if (!gpio_is_valid(pdata->sda_pin)) {
		pr_err("%s: invalid GPIO pins, sda=%d\n",
		       np->full_name, pdata->sda_pin);
		return -ENODEV;
	}

	of_property_read_u32(np, "pt2262-gpio,delay-us", &pdata->udelay);
	of_property_read_u32(np, "pt2262-gpio,slave-addr", &pdata->address);

	return 0;
}

static int __devinit pt2262_probe(struct platform_device *pdev)
{
	struct pt2262_platform_data *pdata;
	int ret;
	
	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;
	
	if (pdev->dev.of_node) {
		ret = of_pt2262_gpio_probe(pdev->dev.of_node, pdata);
		if (ret)
			return ret;
	} else {
		if (!pdev->dev.platform_data)
			return -ENXIO;
		memcpy(pdata, pdev->dev.platform_data, sizeof(*pdata));
	}

	ret = gpio_request(pdata->sda_pin, "sda");
	if (ret) {
		dev_err(&pdev->dev, "could not request gpio, %d\n", ret);
		goto err_request_sda;
	}
	ret = gpio_direction_output(pdata->sda_pin, 0);
	if (ret) {
		dev_err(&pdev->dev, "could not set sda gpio, %d\n", ret);
		goto err_request_sda;
	}

	if (pdata->udelay==0) {
		pdata->udelay = 50;
	}

	platform_set_drvdata(pdev, pdata);

	pt2262_data = pdata;

	ret = misc_register(&pt2262_miscdev);
	if (ret)
		goto err_misc_register;
	
 	dev_info(&pdev->dev, "using pins %u (SDA)\n", pdata->sda_pin);

	return 0;

err_misc_register:
	gpio_free(pdata->sda_pin);
err_request_sda:
	return ret;	
}

static int __devexit pt2262_remove(struct platform_device *pdev)
{
	int ret;
	
	ret = misc_deregister(&pt2262_miscdev);
	if (!ret) {
		struct pt2262_platform_data *pdata;
		
		pdata = platform_get_drvdata(pdev);
		gpio_free(pdata->sda_pin);
	}

	return ret;	
}


#ifdef CONFIG_OF
static const struct of_device_id atmel_pt2262_dt_ids[] = {
	{
		.compatible = "atmel,at91sam9x5-pt2262",
	}
};
MODULE_DEVICE_TABLE(of, atmel_pt2262_dt_ids);
#endif

static struct platform_driver pt2262_driver = {
	.driver		= {
		.name		= "pt2262",
		.owner		= THIS_MODULE,
		.of_match_table	= of_match_ptr(atmel_pt2262_dt_ids),
	},
	.probe		= pt2262_probe,
	.remove		= __devexit_p(pt2262_remove),
};

module_platform_driver(pt2262_driver);

MODULE_AUTHOR("lianxijian <lianxj2008@gmail.com>");
MODULE_LICENSE("GPL v2");

