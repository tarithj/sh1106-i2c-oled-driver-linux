#ifndef ssd1306fb
#define ssd1306fb

#define OLED_WIDTH 128  
#define OLED_HEIGHT 64
#define OLED_PAGE_COUNT (OLED_HEIGHT / 8)
#define FB_SIZE (OLED_WIDTH * OLED_PAGE_COUNT)
static u8 *fb_buffer;
static struct fb_info *info;

static struct fb_ops sh1106_fb_ops = {
    .owner = THIS_MODULE,
    .fb_read = fb_sys_read,
    .fb_write = fb_sys_write,
    .fb_fillrect = sys_fillrect,
    .fb_copyarea = sys_copyarea,
    .fb_imageblit = sys_imageblit,
    .fb_ioctl = NULL,
};
void ssd1306_flush_display(struct i2c_client *client);

#endif
