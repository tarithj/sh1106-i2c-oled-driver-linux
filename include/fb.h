#ifndef _FB_H
#define _FB_H

#include <linux/types.h>
#include <linux/fb.h>  // For framebuffers

// Define a function to initialize the framebuffer
int fb_init(void);

// Define a function to clean up the framebuffer
void fb_cleanup(void);

// Declare a framebuffer information structure
struct fb_info *fb_info;

struct fb_ops sh1106_fb_ops;

#endif /* _FB_H */
