#include "sh1106.h"
#include "../include/fb.h"
void sh1106_sync_fb_to_buffer(void) {
    // Contains 8 pixels in one byte
    for (int y = 0; y < OLED_HEIGHT; y++) {
        for (int x = 0; x < OLED_WIDTH; x++) {
            int byte_index = (y * (OLED_WIDTH / 8)) + (x / 8);
            int bit_pos = x % 8;

            // Read pixel
            int pixel = (fb_buffer[byte_index] >> bit_pos) & 1;
            draw_pixel(x, y, pixel);
        }
    }
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
