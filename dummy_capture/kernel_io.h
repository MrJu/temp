#ifndef __KERNEL_IO
#define __KERNEL_IO

#include <linux/fs.h>

extern int write_file(struct file *filp, const void *buf, size_t size);
extern int read_file(struct file *filp, void *buf, size_t size);

#endif
