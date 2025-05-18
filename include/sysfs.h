#ifndef _SH1106_SYSFS_H
#define _SH1106_SYSFS_H

#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/device.h>  // For device attributes
#include <linux/string.h>  // For string manipulations
#include <linux/errno.h>   // For error numbers

// Define a structure for sysfs operations
struct sysfs_attr {
    struct kobject *kobj;
    struct attribute attr;
    ssize_t (*show)(struct kobject *kobj, struct attribute *attr, char *buf);
    ssize_t (*store)(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count);
};

// Declare the function for initializing the sysfs entries
int sh1106_sysfs_init(struct i2c_client *client);

// Declare the function for cleaning up the sysfs entries
void sh1106_sysfs_cleanup(struct i2c_client *client);

// Define the device attribute structure
extern struct device_attribute dev_attr_contrast;

#endif /* _SH1106_SYSFS_H */
