#ifndef __KERNEL_IO
#define __KERNEL_IO

#include <linux/fs.h>

int write(struct file *filp, const void *buf, size_t size);
int read(struct file *filp, void *buf, size_t size);
char *fgets(struct file *filp, char *buf, size_t size);

#endif
