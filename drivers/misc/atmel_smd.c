/**
 * SAMA5 SMD Demo Driver 
 * Author:  Jeffery Cheng
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <mach/board.h>
#include <linux/fs.h>
#include <linux/delay.h>
//#include <linux/gpio.h>

#include <asm/io.h>
//#include <mach/gpio.h>
#include <mach/sama5d3.h>
#include <mach/at91_pmc.h>
#include <mach/at91_smd.h>

#include <asm/uaccess.h>
#include <linux/miscdevice.h>

#include "music.h"

#define DEVICE_NAME "smd"


#define anchor()        printk("-> %s [%d]\n", __func__, __LINE__)

/* IOCTL_CMD */
#define GPIO_SET_PIN    _IOR('G', 0, int)
#define GPIO_CLR_PIN    _IOR('G', 1, int)
#define GPIO_GET_VALUE  _IOR('G', 2, int)  /* input */

static struct at91_smd {
    unsigned char *reg_base;
} at91_smd;

#define smd_readl(reg)          __raw_readl(at91_smd.reg_base+reg)
#define smd_writel(reg, value)  __raw_writel(value, at91_smd.reg_base+reg)
#define smd_clrbits(reg, mask)  (smd_writel(reg, smd_readl(reg) & ~(mask)))
#define smd_setbits(reg, mask)  (smd_writel(reg, smd_readl(reg) | (mask)))


static int smd_hw_txFIFOfull (void)
{
    if (smd_readl (AT91_SMD_STATUS) & AT91_SMD_STATUS_TX_FIFO_NOT_FULL)
        return false;
    return true;
}

static int smd_hw_lsd_write (uint32_t reg, int value)
{
    int maxErr;
    int wait_data_complete;

    if ((smd_readl (AT91_SMD_CONTROL) & 0xff) != 0x03) {
        /* SMD has no power */
        return false;
    }

    maxErr = 254;
    while (maxErr) {
        smd_writel (AT91_SMD_CTRL1L, value);
        smd_writel (AT91_SMD_CTRL1M, reg | 0x80);

        wait_data_complete = 1;
        /* Wait for Data to complete or Error */
        while (wait_data_complete) {
            /* If data complete we are done */
            if (smd_readl (AT91_SMD_STATUS) & AT91_SMD_STATUS_CTRL_COMP_SUCCESS) {
                wait_data_complete = 0;
                maxErr = 0;
                return true;
            }
            else if (smd_readl (AT91_SMD_STATUS) & AT91_SMD_STATUS_CTRL_COMP_ERR) {
                /* Else error try again until max */
                wait_data_complete = 0;
                maxErr--;
            }
        }
    }
    /* too many error */
    return false;
}

static uint32_t smd_hw_pio_tx (unsigned char *pData, uint32_t size)
{
    uint32_t txSend = size;

    /* Make sure data length is even number */
    if (pData && !(txSend & 0x01)) {
        while (!smd_hw_txFIFOfull ()) {
            if (txSend) {
                smd_writel (AT91_SMD_DATA1L, *(pData++));
                smd_writel (AT91_SMD_DATA1M, *(pData++));
                txSend -= 2;
            }
            else {
                break;
            }
        }
    }
    return (size - txSend);
}

static void hssd_hw_diffTxcverCtrl (int enable)
{
    if (enable) {
        smd_clrbits (AT91_SMD_LPWR_CLK_ENB, AT91_SMD_LWPR_DIFF_EN | AT91_SMD_LWPR_CLK_EN);
    }
    else {
        smd_setbits (AT91_SMD_LPWR_CLK_ENB, AT91_SMD_LWPR_DIFF_EN | AT91_SMD_LWPR_CLK_EN);
    }
}
        
static int smd_hw_reset (int reset)
{
    uint32_t value = smd_readl (AT91_SMD_CONTROL);

    if (reset)
        value &= ~0x03;
    else
        value |= 0x03;
    smd_writel (AT91_SMD_CONTROL, value);

    mdelay (100);
    return true;
}

static void smd_reset (void)
{
    smd_hw_reset (true);
    mdelay (200);
    smd_writel (AT91_SMD_WRAP, AT91_SMD_WRAPVALUE);
    smd_hw_reset (false);
}

static int smd_hw_init (void)
{
    smd_writel (AT91_SMD_AUX, 0x00);

    hssd_hw_diffTxcverCtrl (true);   /* To access HSSD register set */

    smd_reset ();

    smd_writel (AT91_SMD_DRIVE, 0);
    smd_writel (AT91_SMD_VOLC, 0x88);

    smd_writel (AT91_SMD_ADJ, AT91_SMD_ADJVALUE);
    smd_writel (AT91_SMD_STATUS, 0x3E);

    mdelay (200);

    smd_writel (AT91_SMD_CTRL1M, 0);
    /* Enable data channel */
    smd_writel (AT91_SMD_ADJ, smd_readl (AT91_SMD_ADJ)|0x04);

    return 0;
}

static int smd_hw_dibPolarityTest (void)
{
    uint32_t value;
    int rwCount = 10;

    while (rwCount) {
        smd_writel (AT91_SMD_CTRL1M, 0x0E);
        printk("[cheng]: CTRL1M = 0x%X\n", smd_readl(AT91_SMD_CTRL1M));
        do {
            value = smd_readl (AT91_SMD_STATUS);
            printk ("SMD_STATUS: 0x%X\n", value);
        } while (!(value & (AT91_SMD_STATUS_CTRL_COMP_SUCCESS | AT91_SMD_STATUS_CTRL_COMP_ERR)));
        if (value & AT91_SMD_STATUS_CTRL_COMP_SUCCESS)
            break;
        else if (value & AT91_SMD_STATUS_CTRL_COMP_ERR)
            rwCount--;
    }
    return (rwCount ? true : false);
}

static int smd_hw_lsd_init (void)
{
    int ret = true;

    if (smd_hw_dibPolarityTest() == false) {
        smd_writel (AT91_SMD_ADJ, smd_readl (AT91_SMD_ADJ) ^ 0x80);
        if (smd_hw_dibPolarityTest() == false) {
            smd_writel (AT91_SMD_ADJ, smd_readl (AT91_SMD_ADJ) & 0x80);
            ret = false;
        }
    }
    return ret;
}
static int smd_hw_offhook (void)
{
    int i;
    struct regs {
        uint8_t reg;
        uint8_t val;
    } regs[] = {
        { 0x01, 0x31 },
        { 0x03, 0x00 },
        { 0x04, 0x00 },
        { 0x05, 0x4C },
        { 0x06, 0x06 },
        { 0x07, 0xEC },
        { 0x08, 0x13 },
        { 0x09, 0x80 },
        { 0x0A, 0x23 },
        { 0x0B, 0x00 },
        { 0x0C, 0x04 },
        { 0x0D, 0x10 },
        { 0x0E, 0x80 },
        { 0x0F, 0x07 },
        { 0x10, 0x88 },
        { 0x11, 0x01 },
        { 0x12, 0x00 },
    };

    if (smd_readl (AT91_SMD_CONTROL) != 0x03) {
        /* SMD has no power */
        return false;
    }

    for (i = 0; i < ARRAY_SIZE(regs); i++) {
        smd_hw_lsd_write (regs[i].reg, regs[i].val);
    }
    return true;
}
static void smd_hw_onhook (void)
{
    /* For Smart DAA only */
    smd_hw_lsd_write (0x0C, 0xC4);
    smd_hw_lsd_write (0x10, 0x12);
}

static int smd_hw_lowPowerEventInit (void)
{
    smd_hw_lsd_write (0x0B, 0xFF);
    smd_hw_lsd_write (0x0C, 0xF7);

    smd_writel (AT91_SMD_DATA1L, 0xF7);

    smd_writel (AT91_SMD_PARAM, 0x10);
    smd_writel (AT91_SMD_ADJ, smd_readl (AT91_SMD_ADJ) | AT91_SMD_BANK1_REG_SELECT);
    smd_writel (AT91_SMD_PARAM, 0x03);
    smd_writel (AT91_SMD_ADJ, smd_readl (AT91_SMD_ADJ) & ~AT91_SMD_BANK1_REG_SELECT);

    return true;
}

static void smd_hw_enableRingDetect (int enable)
{
    uint32_t value;

    value = smd_readl (AT91_SMD_AUX);
    if (enable) {
        /* Enable ring detection state machine */
        value |= AT91_SMD_AUX_HW_RING_DETECT_ENA;
    }
    else {
        value &= ~AT91_SMD_AUX_HW_RING_DETECT_ENA;
    }
    smd_writel (AT91_SMD_AUX, value);
}

static void smd_hw_clearRingState (void)
{
    smd_writel (AT91_SMD_AUX, smd_readl (AT91_SMD_AUX) | 
        AT91_SMD_AUX_HW_RING_DETECT_STAT);
}

static int smd_hw_readRingState (void)
{
    return (smd_readl (AT91_SMD_AUX) & AT91_SMD_AUX_HW_RING_DETECT_STAT);
}

static int lsd_init (void)
{
    anchor();
    smd_hw_init ();
    anchor();
    smd_hw_lsd_init ();
    anchor();

    return 0;
}

static int off_hook (void)
{
    return (smd_hw_offhook() ? 0 : 1);
}
static int on_hook (void)
{
    smd_hw_onhook ();
    return 0;
}

static int send_test_tone (void)
{
    int cnt = 3;
    unsigned int i = 0;
    unsigned int numofsent = 0;

    while (true) {
        numofsent = smd_hw_pio_tx (&music[i], 2);
        i += numofsent;
        if (i + 1 >= sizeof(music)) {
            i = 0;
        }
    }
    return 0;
}

static int wait_for_ring (void)
{
    int ret = 1;

    smd_hw_lowPowerEventInit ();
    smd_hw_enableRingDetect (true);
    smd_hw_clearRingState ();

    while (!smd_hw_readRingState ()) {
        /* wait for ring */
        mdelay (100);
    }
    ret = 0;
out:
    smd_hw_clearRingState ();
    smd_hw_enableRingDetect (false);
    smd_reset ();

    return ret;
}

/*-----------------------------------------*/
static int swm_open (struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t swm_read (struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
    return sizeof(int);
}

static ssize_t swd_write (struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    return sizeof(int);
}

static long swd_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    int retval = 0;
    switch (cmd) {
        case GPIO_SET_PIN: 
            anchor();
            smd_reset ();
            anchor();
            lsd_init ();
            anchor();
            off_hook ();
            anchor();
            send_test_tone ();
            anchor();
            on_hook ();
            anchor();
            wait_for_ring ();
            anchor();
            smd_reset ();
            anchor();
            lsd_init ();
            anchor();
        break;
        
        case GPIO_CLR_PIN: 
        break;

        case GPIO_GET_VALUE: 
        break;

        default:
            printk(KERN_ERR "%s: command type unsupport\n", __func__);
        break;
    }

    return retval;
}

static int swd_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations swd_fops = {
    .owner  = THIS_MODULE,
    .open   = swm_open,
    .read   = swm_read,
    .write  = swd_write,
    .unlocked_ioctl = swd_ioctl,
    .release    = swd_release,
};

static struct miscdevice swd_miscdev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = DEVICE_NAME,
    .fops   = &swd_fops,
};

static int __init atmel_swd_init (void)
{
    int ret = 0;

    /* Enable SMD clock */
    at91_pmc_write(AT91_PMC_SCER, at91_pmc_read (AT91_PMC_SCER) | AT91_PMC_SMDCK);

    at91_pmc_write(AT91_PMC_SMD, AT91_PMC_SMDDIV(0x1F));

    at91_pmc_write(AT91_PMC_PCER, 1<<SAMA5D3_ID_SMD);

    /* remap regs */
    at91_smd.reg_base = ioremap_nocache (AT91_SMD_BASE_ADDR, 0x60);
    if (!at91_smd.reg_base) {
        printk (KERN_ERR "%s: remap regs failed\n", __func__);
        goto out;
    }

    /* reset */
    smd_reset();

    ret = misc_register (&swd_miscdev);
    if (ret) {
        printk (KERN_ERR "cannot register miscdev on minor=%d (%d)\n", MISC_DYNAMIC_MINOR, ret);
        goto out;
    }

    printk (KERN_INFO "atmel %s initialized\n", DEVICE_NAME);

    return 0;

out:
    return ret;
}

static void __exit atmel_swd_exit (void)
{
    printk (KERN_INFO "atmel %s removed\n", DEVICE_NAME);

    misc_deregister(&swd_miscdev);

    if (at91_smd.reg_base) {
        iounmap (at91_smd.reg_base); at91_smd.reg_base = NULL;
    }
}

module_init (atmel_swd_init);
module_exit (atmel_swd_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Atmel SMD Device");
