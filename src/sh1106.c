/**
 * @file		sh1106.c
 * @brief		Linux Kernel Module for sh1106 OLED display
 *
 * @author		Tarith Jayasooriya
 * @copyright	Copyright (c) 2025 Tarith Jayasooriya
 *
 * @par License
 *	Released under the MIT and GPL Licenses.
 *	- https://github.com/tarithj/sh1106-i2c-oled-driver-linux/blob/master/LICENSE-MIT
 *	- https://github.com/tarithj/sh1106-i2c-oled-driver-linux/blob/master/LICENSE-GPL
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/kernel.h> 
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/version.h>

#include "sh1106.h"
#include "fb.h"



// ----- Module Parameters ----- //
static bool enable_fb = true;
module_param(enable_fb, bool, 0444);
MODULE_PARM_DESC(enable_fb, "Enable framebuffer support");

/* Workqueue for asynchronous display updates */
static struct workqueue_struct *display_wq;

/* Work item for flushing the SH1106 display */
static DECLARE_WORK(flush_display_work, sh1106_flush_display_work);

/* Timer for periodic display refresh */
static struct timer_list sh1106_timer;

/* Pointer to the SH1106 I2C client device */
static struct i2c_client *sh1106_i2c_client;

/* Buffer to store pixel data */
uint8_t buffer[OLED_WIDTH * ((OLED_HEIGHT + 7) / 8)];


// ----- Attributes ----- //
static int contrast_value = 127; // Default contrast value (range from 0 to 255)

static ssize_t contrast_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", contrast_value); // Return the current contrast value
}
static ssize_t contrast_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret;
    struct i2c_client *client = to_i2c_client(dev); 
    ret = kstrtoint(buf, 10, &contrast_value);  // Convert input string to integer
    if (ret < 0)
        return ret;

    if (contrast_value < 0 || contrast_value > 255)
        return -EINVAL;  // Validate input

    // Apply the new contrast value (adjust the OLED accordingly)
    sh1106_send_command(client, 0x81);        // Set contrast control command
    sh1106_send_command(client, contrast_value);  // Set contrast value

    return count; // Return number of bytes written
}

// Define the sysfs attributes
static DEVICE_ATTR(contrast, S_IRUGO | S_IWUSR, contrast_show, contrast_store);


void sh1106_flush_display(struct i2c_client *client)
{
    uint8_t page, col, i;
    uint8_t buf[17];
    uint8_t *p = buffer;

    for (page = 0; page < OLED_PAGE_COUNT; page++) {
        // Set page address
        sh1106_send_command(client, 0xB0 + page);
        // Set lower column address
        sh1106_send_command(client, 0x00);
        // Set higher column address
        sh1106_send_command(client, 0x10);

        for (col = 0; col < (OLED_WIDTH / 16); col++) {
            buf[0] = 0x40; // Control byte for data
            for (i = 0; i < 16; i++) {
                buf[i + 1] = *p++;
            }
            i2c_master_send(client, buf, 17);
        }
    }
}


// Queues the refresh display work if it isnt already running
void queue_refresh_display_delayed_work(void) {
    if (!(work_busy(&flush_display_work) & WORK_BUSY_PENDING))
        queue_work(display_wq, &flush_display_work);
}


void sh1106_flush_display_work(struct work_struct *work) {
    sh1106_sync_fb_to_buffer();
    sh1106_flush_display(sh1106_i2c_client);
}


void sh1106_init(struct i2c_client *client) {
    // Turn off display
    sh1106_send_command(client, 0xAE);

    // Set display clock divide
    sh1106_send_command(client, 0xD5);
    sh1106_send_command(client, 0x80);  // Default value

    // Set multiplex ratio (SH1106 is typically 0x3F for 128x128 or 0x1F for 128x64)
    sh1106_send_command(client, 0xA8);
    sh1106_send_command(client, 0x3F);  // 64MUX for 128x64

    // Set display offset
    sh1106_send_command(client, 0xD3);
    sh1106_send_command(client, 0x00);  // No offset

    // Set start line
    sh1106_send_command(client, 0x40);

    // Set segment remap
    sh1106_send_command(client, 0xA1);  // Reverse mapping (A1 for reverse, A0 for normal)

    // Set COM output scan direction
    sh1106_send_command(client, 0xC8);  // Reverse (C8 for reverse, C0 for normal)

    // Set COM pins hardware configuration
    sh1106_send_command(client, 0xDA);
    sh1106_send_command(client, 0x12);  // Sequential COM pin config

    // Set contrast control
    sh1106_send_command(client, 0x81);
    sh1106_send_command(client, 0x7F);  // Default contrast

    // Entire display ON (RAM content)
    sh1106_send_command(client, 0xA4);

    // Normal display (A6 for normal, A7 for inverse)
    sh1106_send_command(client, 0xA6);

    // Charge pump setting (Enable charge pump for SH1106)
    sh1106_send_command(client, 0x8D);
    sh1106_send_command(client, 0x14);  // Enable charge pump

    // Memory addressing mode (horizontal)
    sh1106_send_command(client, 0x20);
    sh1106_send_command(client, 0x02);  // Horizontal addressing mode

    // Turn on display
    sh1106_send_command(client, 0xAF);
}


void draw_pixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        pr_info("x y out of bounds");
        return;
    }


    switch (color) {
        case sh1106_WHITE:
            buffer[x + (y / 8) * OLED_WIDTH] |= (1 << (y & 7));
            break;
        case sh1106_BLACK:
            buffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y & 7));
            break;
        case sh1106_INVERSE:
            buffer[x + (y / 8) * OLED_WIDTH] ^= (1 << (y & 7));
            break;
        default:
            pr_err("Unknown color");
    }
}

// Sync framebuffer pixel data to internel buffer 
void sh1106_sync_fb_to_buffer(void) {
     // contains 8 pixels in one byte
    for (int y = 0; y < OLED_HEIGHT; y++) {
        for (int x = 0; x < OLED_WIDTH; x++) {
            int byte_index = (y * (OLED_WIDTH / 8)) + (x / 8);
            int bit_pos = x % 8;

            // Read pixel
            int pixel = (fb_buffer[byte_index] >> bit_pos) & 1;
            draw_pixel(x,y,pixel);
        }
    }
}

static void sh1106_timer_callback(struct timer_list *t)
{
    if (!sh1106_i2c_client)
        return;

    queue_refresh_display_delayed_work();

    mod_timer(&sh1106_timer, jiffies + msecs_to_jiffies(FLUSH_INTERVAL_MS));
}

int sh1106_send_command(struct i2c_client *client, u8 command)
{
    // Send control byte (0x00) + command
    u8 buf[2] = { 0x00, command };
    struct i2c_msg msg = {
        .addr = client->addr,
        .flags = 0, // write
        .len = 2,
        .buf = buf,
    };
    return i2c_transfer(client->adapter, &msg, 1);
}



static int sh1106_detect(struct i2c_client *i, struct i2c_board_info *) {
    pr_info("Detecint OLED displays");
    if (i->addr != OLED_I2C_ADDR) {
        pr_info("SH1106 OLED display found but not correct\n");
        return 1;
    }
    return 0;
}

// Switch probe signature depending on version
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,3,0)
static int sh1106_probe(struct i2c_client *client, const struct i2c_device_id *)
#else
static int sh1106_probe(struct i2c_client *client)
{
    memset(buffer, 0, sizeof(buffer));
    pr_info("SH1106 OLED display found at 0x%02x\n", client->addr);

    // Cleanup previous attr
    device_remove_file(&client->dev, &dev_attr_contrast);
    
    display_wq = alloc_workqueue("my_wq", WQ_UNBOUND, 0);

    sh1106_init(client);
    sh1106_i2c_client = client;

    // Create sysfs entry for contrast
    int ret = device_create_file(&client->dev, &dev_attr_contrast);
    if (ret) {
        dev_err(&client->dev, "Failed to create sysfs entry for contrast\n");
        return ret;
    }


    if (enable_fb) {
        // alloc memory for fb
        fb_buffer = devm_kzalloc(&client->dev, FB_SIZE, GFP_KERNEL);
        if (!fb_buffer) {
            return -ENOMEM;
        }
        // declare fb
        info = framebuffer_alloc(0, &client->dev);
        if (!info)
            return -ENOMEM;

        info->screen_base = (char __force __iomem *)fb_buffer;
        info->screen_size = FB_SIZE;

        info->fix = (struct fb_fix_screeninfo){
            .id = "SH1106",
            .type = FB_TYPE_PACKED_PIXELS,
            .visual = FB_VISUAL_MONO01,
            .line_length = OLED_WIDTH,
        };

        info->var = (struct fb_var_screeninfo){
            .xres = OLED_WIDTH,
            .yres = OLED_HEIGHT,
            .bits_per_pixel = 1,
            .xres_virtual = OLED_WIDTH,
            .yres_virtual = OLED_HEIGHT,
        };

        info->fbops = &sh1106_fb_ops;
        info->flags = FBINFO_FLAG_DEFAULT;

        ret = register_framebuffer(info);
        if (ret < 0) {
            dev_err(&client->dev, "Failed to create fb\n");
            return ret;
        }
    } else {
        pr_info("Framebuffer support disabled by user");
    }


    for(int x =0;x < OLED_WIDTH; x++) {
        for(int y =0;y < OLED_HEIGHT; y++) {
            if (y%2 == 1) {
                draw_pixel(x, y, sh1106_WHITE);
            }else{
                draw_pixel(x,y, sh1106_BLACK);
            }
        }
    }

    timer_setup(&sh1106_timer, sh1106_timer_callback, 0);
    mod_timer(&sh1106_timer, jiffies + msecs_to_jiffies(FLUSH_INTERVAL_MS));

    return 0;
}

static void sh1106_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "Removing SH1106 OLED device\n");

    device_remove_file(&client->dev, &dev_attr_contrast);

    del_timer_sync(&sh1106_timer);
    flush_workqueue(display_wq); 
    destroy_workqueue(display_wq);
    unregister_framebuffer(info);

    pr_info("SH1106 OLED driver removed\n");
}



static const struct i2c_device_id sh1106_id[] = {
    { "SH1106", 0 },
    { }
};
static const unsigned short i2c_address_list[] = {OLED_I2C_ADDR};
MODULE_DEVICE_TABLE(i2c, sh1106_id);

static struct i2c_driver sh1106_driver = {
    .driver = {
        .name = "SH1106",
        
    },
    
    .probe = sh1106_probe,
    .remove = sh1106_remove,
    .id_table = sh1106_id,
    .address_list = i2c_address_list,
    .detect = sh1106_detect
};

module_i2c_driver(sh1106_driver);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Tarith Jayasooriya");
MODULE_DESCRIPTION("SH1106 OLED driver (I2C)");
