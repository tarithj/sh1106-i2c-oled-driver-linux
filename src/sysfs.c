#include "sh1106.h"
#include "../include/sysfs.h"

static int contrast_value = 127; // Default contrast value (range from 0 to 255)

static ssize_t contrast_show(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", contrast_value); // Return the current contrast value
}

static ssize_t contrast_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
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
