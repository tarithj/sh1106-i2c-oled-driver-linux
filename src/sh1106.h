#ifndef SSD1306
#define SSD1306

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

#endif