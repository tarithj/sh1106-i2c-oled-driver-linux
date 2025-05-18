#include "../include/sh1106.h"
#include "../include/sysfs.h"
#include "../include/fb.h"

// Module Parameters
static bool enable_fb = true;
module_param(enable_fb, bool, 0444);
MODULE_PARM_DESC(enable_fb, "Enable framebuffer support");

uint8_t buffer[SH1106_WIDTH * ((SH1106_HEIGHT + 7) / 8)];

static int sh1106_probe(struct i2c_client *client, const struct i2c_device_id *id);
static void sh1106_remove(struct i2c_client *client);
static int sh1106_detect(struct i2c_client *i, struct i2c_board_info *info);

static const struct i2c_device_id sh1106_id[] = {
    { "SH1106", 0 },
    { }
};

static const unsigned short i2c_address_list[] = {OLED_I2C_ADDR};

static struct i2c_driver sh1106_driver = {
    .driver = {
        .name = "SH1106",
    },
    .probe = sh1106_probe,
    .remove = sh1106_remove,
    .id_table = sh1106_id,
    .address_list = i2c_address_list,
    .detect = sh1106_detect,
};

module_i2c_driver(sh1106_driver);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Tarith Jayasooriya");
MODULE_DESCRIPTION("SH1106 OLED driver (I2C)");


static void sh1106_timer_callback(struct timer_list *t)
{
    if (!sh1106_i2c_client)
        return;
    queue_refresh_display_delayed_work();
    mod_timer(&sh1106_timer, jiffies + msecs_to_jiffies(FLUSH_INTERVAL_MS));
}

// Probe function
static int sh1106_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    memset(buffer, 0, sizeof(buffer));
    pr_info("SH1106 OLED display found at 0x%02x\n", client->addr);

    display_wq = alloc_workqueue("my_wq", WQ_UNBOUND, 0);
    sh1106_init(client);
    sh1106_i2c_client = client;
    int ret = 0;
    sysfs_init();
    
    if (enable_fb) {
        fb_buffer = devm_kzalloc(&client->dev, FB_SIZE, GFP_KERNEL);
        if (!fb_buffer) {
            return -ENOMEM;
        }

        info = framebuffer_alloc(0, &client->dev);
        if (!info)
            return -ENOMEM;

        info->screen_base = (char __force __iomem *)fb_buffer;
        info->screen_size = FB_SIZE;

        info->fix = (struct fb_fix_screeninfo){
            .id = "SH1106",
            .type = FB_TYPE_PACKED_PIXELS,
            .visual = FB_VISUAL_MONO01,
            .line_length = SH1106_WIDTH,
        };

        info->var = (struct fb_var_screeninfo){
            .xres = SH1106_WIDTH,
            .yres = SH1106_HEIGHT,
            .bits_per_pixel = 1,
            .xres_virtual = SH1106_WIDTH,
            .yres_virtual = SH1106_HEIGHT,
        };

        info->fbops = &sh1106_fb_ops;
        info->flags = 0; // FBINFO_FLAG_DEFAULT is 0 (https://patchwork.kernel.org/project/linux-input/patch/20230715185343.7193-12-tzimmermann@suse.de/)

        ret = register_framebuffer(info);
        if (ret < 0) {
            dev_err(&client->dev, "Failed to create fb\n");
            return ret;
        }
    } else {
        pr_info("Framebuffer support disabled by user");
    }

    timer_setup(&sh1106_timer, sh1106_timer_callback, 0);
    mod_timer(&sh1106_timer, jiffies + msecs_to_jiffies(FLUSH_INTERVAL_MS));

    return 0;
}

// Remove function
static void sh1106_remove(struct i2c_client *client) {
    dev_info(&client->dev, "Removing SH1106 OLED device\n");
    sysfs_cleanup();
    del_timer_sync(&sh1106_timer);
    flush_workqueue(display_wq); 
    destroy_workqueue(display_wq);
    unregister_framebuffer(info);

    pr_info("SH1106 OLED driver removed\n");
}

// Detect function
static int sh1106_detect(struct i2c_client *i, struct i2c_board_info *info) {
    pr_info("Detecting OLED display");
    if (i->addr != OLED_I2C_ADDR) {
        pr_info("SH1106 OLED display found but not correct\n");
        return 1;
    }
    return 0;
}
