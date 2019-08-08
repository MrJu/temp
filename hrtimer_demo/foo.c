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

#include <linux/module.h>
#include <linux/hrtimer.h>

#define STR(x) _STR(x)
#define _STR(x) #x

#define VERSION_PREFIX Foo
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define PATCH_VERSION 0

#define VERSION STR(VERSION_PREFIX-MAJOR_VERSION.MINOR_VERSION.PATCH_VERSION)

#define MS_TO_NS(x) (x * 1E6L)
#define DELAY_MS 500L

struct hrtimer_

static struct hrtimer timer;
ktime_t kt;

static enum hrtimer_restart  hrtimer_handler(struct hrtimer *timer)
{
	printk("%s(): %d\n", __func__, __LINE__);

	hrtimer_forward_now(timer, kt);
	return HRTIMER_RESTART;
}

static int __init foo_init(void)
{
	printk("%s(): %d\n", __func__, __LINE__);

	kt = ktime_set(0, MS_TO_NS(DELAY_MS));
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = hrtimer_handler;
	hrtimer_start(&timer, kt, HRTIMER_MODE_REL);

	return 0;
}

static void __exit foo_exit(void)
{
	printk("%s(): %d\n", __func__, __LINE__);
	hrtimer_cancel(&timer);
}

module_init(foo_init);
module_exit(foo_exit);

MODULE_ALIAS("foo-driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);
MODULE_DESCRIPTION("Linux is not Unix");
MODULE_AUTHOR("andrew, mrju.email@gmail.com");
