/*
 * Copyright (C) 2019 Andrew <mrju.email@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include "kernel_io.h"

#define STR(x) _STR(x)
#define _STR(x) #x

#define VERSION_PREFIX Foo
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define PATCH_VERSION 0

#define VERSION STR(VERSION_PREFIX-MAJOR_VERSION.MINOR_VERSION.PATCH_VERSION)

#define FILE_PATH "/home/pi/Documents/vfs.txt"

int __init foo_init(void)
{
	int ret;
	struct file *filp;
	char *str = "hello world!\n";
	char buf[256];

	filp = filp_open(FILE_PATH, O_RDWR | O_CREAT, 0666);
	if (IS_ERR(filp))
		return PTR_ERR(filp);

	ret = write_file(filp, str, strlen(str));
	if (ret < 0)
		goto exit;

	ret = vfs_llseek(filp, 0, SEEK_SET);
	if (ret < 0)
		goto exit;

	ret = read_file(filp, buf, strlen(str));
	if (ret < 0)
		goto exit;

	filp_close(filp, NULL);

	printk("%s(): %d %s\n", __func__, __LINE__, buf);

	return 0;

exit:
	filp_close(filp, NULL);
	return ret;
}

void __exit foo_exit(void)
{
	printk("%s(): %d\n", __func__, __LINE__);
}

module_init(foo_init);
module_exit(foo_exit);

MODULE_ALIAS("foo-driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);
MODULE_DESCRIPTION("Linux is not Unix");
MODULE_AUTHOR("andrew, mrju.email@gmail.com");
