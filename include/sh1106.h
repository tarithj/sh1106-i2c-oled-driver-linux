#ifndef SSD1306
#define SSD1306

#include <linux/i2c.h>
#include <linux/fb.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

#define OLED_I2C_ADDR 0x3C
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

#define sh1106_WHITE 0
#define sh1106_BLACK 1
#define sh1106_INVERSE 2


#define FLUSH_INTERVAL_MS 300


int sh1106_send_command(struct i2c_client *client, u8 command);


/**
 * sh1106_flush_display - Flush the display buffer to the SH1106 screen
 * @client: Pointer to the I2C client representing the display
 *
 * Transfers the framebuffer contents over I2C to update the screen.
 * Must be called in a non-atomic context.
 *
 * Return: 0 on success, negative errno on failure.
 */
void sh1106_flush_display_work(struct work_struct *work);


void sh1106_sync_fb_to_buffer(void);

// Function prototypes
void sh1106_init(struct i2c_client *client);
void sh1106_flush_display(struct i2c_client *client);
void sh1106_sync_fb_to_buffer(void);
int sh1106_send_command(struct i2c_client *client, u8 command);
void draw_pixel(int16_t x, int16_t y, uint8_t color);
void queue_refresh_display_delayed_work(void);
void sh1106_flush_display_work(struct work_struct *work);

extern struct i2c_client *sh1106_i2c_client;
extern uint8_t buffer[OLED_WIDTH * ((OLED_HEIGHT + 7) / 8)];
extern int contrast_value;

extern struct workqueue_struct *display_wq;
extern struct timer_list sh1106_timer;
extern struct framebuffer_info *info;
extern uint8_t *fb_buffer;



#endif

