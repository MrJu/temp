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

#include <linux/fs.h>
#include "kernel_io.h"

int write(struct file *filp, const void *buf, size_t size)
{
	int bytes, pos = 0;

	while (pos < size) {
		bytes = kernel_write(filp, buf + pos, size - pos, &filp->f_pos);
		if (bytes < 0)
			return bytes;

		if (!bytes)
			return -EIO;

		pos += bytes;
	}

	return pos;
}

int read(struct file *filp, void *buf, size_t size)
{
	int bytes, pos = 0;

	while (pos < size) {
		bytes = kernel_read(filp, buf + pos, size - pos, &filp->f_pos);
		if (bytes < 0)
			return bytes;

		if (!bytes)
			return -EIO;

		pos += bytes;
	}

	return pos;
}

char *fgets(struct file *filp, char *buf, size_t size)
{
	int ret;
	int i = 0;

	ret = kernel_read(filp, buf, size, &filp->f_pos);
	if (ret <= 0)
		return NULL;

	while(buf[i++] != '\n' && i < ret);

	if(i < ret)
		filp->f_pos += i-ret;

	if(i < size)
		buf[i] = 0;

	return buf;
}
