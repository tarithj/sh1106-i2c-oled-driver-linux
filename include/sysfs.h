#ifndef _SYSFS_H
#define _SYSFS_H

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
int sysfs_init(void);

// Declare the function for cleaning up the sysfs entries
void sysfs_cleanup(void);

// Define the device attribute structure
extern struct device_attribute dev_attr_contrast;

#endif /* _SYSFS_H */
