/*
 * Copyright (C) 2019 Andrew <mrju.email@gail.com>
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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#define STR(x) _STR(x)
#define _STR(x) #x

#define VERSION_PREFIX Foo
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define PATCH_VERSION 0

#define VERSION STR(VERSION_PREFIX-MAJOR_VERSION.MINOR_VERSION.PATCH_VERSION)

#define DEVICE_NAME "foo"

struct val_node {
	struct list_head list;
	int val;
};

struct val_queue {
	struct list_head *list;
	unsigned int max_count;
	unsigned int count;
};

struct val_queue *queue;
static struct proc_dir_entry *linked_list_dentry;
static struct proc_dir_entry *add_head_dentry;
static struct proc_dir_entry *add_tail_dentry;
static struct proc_dir_entry *del_head_dentry;
static struct proc_dir_entry *del_tail_dentry;
static struct proc_dir_entry *show_dentry;

static struct list_head *val_list_create(void)
{
	struct list_head *list = kzalloc(sizeof(*list), GFP_KERNEL);
	if (!list)
		return  ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(list);

	return list;
}

static struct val_node *val_node_alloc(int val)
{
	struct val_node *node;

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&node->list);
	node->val = val;

	return node;
}

static int val_list_add_head(struct list_head *list, int val)
{
	struct val_node *node = val_node_alloc(val);
	if (!node)
		return -ENOMEM;

	list_add(&node->list, list);

	return 0;
}

static int val_list_add_tail(struct list_head *list, int val)
{
	struct val_node *node = val_node_alloc(val);
	if (!node)
		return -ENOMEM;

	list_add_tail(&node->list, list);

	return 0;
}

static int val_list_del_head(struct list_head *list)
{
	struct list_head *next;
	struct val_node *node;

	if (list_empty(list))
		return 0;

	next = list->next;
	node = list_entry(next, struct val_node, list);

	list_del(next);
	kfree(node);

	return 0;
}

static int val_list_del_tail(struct list_head *list)
{
	struct list_head *prev;
	struct val_node *node;

	if (list_empty(list))
		return 0;

	prev = list->prev;
	node = list_entry(prev, struct val_node, list);

	list_del(prev);
	kfree(node);

	return 0;
}

static void val_list_destroy(struct list_head *list)
{
	struct val_node *node, *n;

	list_for_each_entry_safe(node, n, list, list)
		kfree(node);

	kfree(list);
}

static struct val_queue *val_queue_create(unsigned int count)
{
	struct val_queue *queue;
	struct list_head *list;

	queue = kzalloc(sizeof(*queue), GFP_KERNEL);
	if (!queue)
		return  ERR_PTR(-ENOMEM);

	list = val_list_create();
	if (!list) {
		kfree(queue);
		return  ERR_PTR(-ENOMEM);
	}

	queue->list = list;
	queue->max_count = count;
	queue->count = 0;

	return queue;
}

static void val_queue_destroy(struct val_queue *queue)
{
	val_list_destroy(queue->list);
	kfree(queue);
}

static int val_list_show(struct seq_file *m, void *v)
{
	struct val_node *node;
	struct list_head *list = PDE_DATA(file_inode(m->file));

	list_for_each_entry(node, list, list) {
		seq_printf(m, "%d ", node->val);
	}
	seq_printf(m, "\n");

	return 0;
}

static ssize_t add_head_store(struct file *file, const char __user *buf,
		size_t size, loff_t *ppos)
{
	int n;
	struct list_head *list = PDE_DATA(file_inode(file));

	if (sscanf(buf, "%d", &n) != 1)
		return -EINVAL;

	if (val_list_add_head(list, n))
		return -ENOMEM;

	return size;
}

static ssize_t add_tail_store(struct file *file, const char __user *buf,
		size_t size, loff_t *ppos)
{
	int n;
	struct list_head *list = PDE_DATA(file_inode(file));

	if (sscanf(buf, "%d", &n) != 1)
		return -EINVAL;

	if (val_list_add_tail(list, n))
		return -ENOMEM;

	return size;
}

static ssize_t del_head_store(struct file *file, const char __user *buf,
		size_t size, loff_t *ppos)
{
	bool val;
	int ret;
	struct list_head *list = PDE_DATA(file_inode(file));

	ret = strtobool(buf, &val);
	if (ret < 0)
		return ret;

	if (val == false)
		return -EINVAL;

	if (list_empty(list))
		return -ENOSPC;

	val_list_del_head(list);

	return size;
}

static ssize_t del_tail_store(struct file *file, const char __user *buf,
		size_t size, loff_t *ppos)
{
	bool val;
	int ret;
	struct list_head *list = PDE_DATA(file_inode(file));

	ret = strtobool(buf, &val);
	if (ret < 0)
		return ret;

	if (val == false)
		return -EINVAL;

	if (list_empty(list))
		return -ENOSPC;

	val_list_del_tail(list);

	return size;
}

static const struct file_operations add_head_fops = {
	.write = add_head_store,
};

static const struct file_operations add_tail_fops = {
	.write = add_tail_store,
};

static const struct file_operations del_head_fops = {
	.write = del_head_store,
};

static const struct file_operations del_tail_fops = {
	.write = del_tail_store,
};

static int show_open(struct inode *inode, struct file *file)
{
	return single_open(file, val_list_show, inode->i_private);
}

static const struct file_operations show_fops = {
	.open = show_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int foo_probe(struct platform_device *pdev)
{
	struct list_head *list = val_list_create();
	if (!list)
		goto err_create_list;

	linked_list_dentry = proc_mkdir("foo", NULL);
	if (!linked_list_dentry)
		goto err_create_linked_list;

	add_head_dentry = proc_create_data("add_head",
			S_IRUSR | S_IRGRP | S_IROTH,
			linked_list_dentry, &add_head_fops, list);
	if (!add_head_dentry)
		goto err_create_add_head;

	add_tail_dentry = proc_create_data("add_tail",
			S_IRUSR | S_IRGRP | S_IROTH,
			linked_list_dentry, &add_tail_fops, list);
	if (!add_tail_dentry)
		goto err_create_add_tail;

	del_head_dentry = proc_create_data("del_head",
			S_IRUSR | S_IRGRP | S_IROTH,
			linked_list_dentry, &del_head_fops, list);
	if (!del_head_dentry)
		goto err_create_del_head;

	del_tail_dentry = proc_create_data("del_tail",
			S_IRUSR | S_IRGRP | S_IROTH,
			linked_list_dentry, &del_tail_fops, list);
	if (!del_tail_dentry)
		goto err_create_del_tail;

	show_dentry = proc_create_data("show",
			S_IRUSR | S_IRGRP | S_IROTH,
			linked_list_dentry, &show_fops, list);
	if (!show_dentry)
		goto err_create_show;

	platform_set_drvdata(pdev, list);

	printk("Andrew: %s %s %d\n", __FILE__, __func__, __LINE__);
	return 0;

err_create_show:
	printk("Andrew: %s %s %d\n", __FILE__, __func__, __LINE__);
	remove_proc_entry("del_tail", linked_list_dentry);

err_create_del_tail:
	printk("Andrew: %s %s %d\n", __FILE__, __func__, __LINE__);
	remove_proc_entry("del_head", linked_list_dentry);

err_create_del_head:
	printk("Andrew: %s %s %d\n", __FILE__, __func__, __LINE__);
	remove_proc_entry("add_tail", linked_list_dentry);

err_create_add_tail:
	printk("Andrew: %s %s %d\n", __FILE__, __func__, __LINE__);
	remove_proc_entry("add_head", linked_list_dentry);

err_create_add_head:
	printk("Andrew: %s %s %d\n", __FILE__, __func__, __LINE__);
	remove_proc_entry("foo", NULL);

err_create_linked_list:
	printk("Andrew: %s %s %d\n", __FILE__, __func__, __LINE__);
	val_list_destroy(list);

err_create_list:
	printk("Andrew: %s %s %d\n", __FILE__, __func__, __LINE__);
	return -ENOMEM;
}

static int foo_remove(struct platform_device *pdev)
{
	struct list_head *list = platform_get_drvdata(pdev);

	remove_proc_entry("show", linked_list_dentry);
	remove_proc_entry("del_tail", linked_list_dentry);
	remove_proc_entry("del_head", linked_list_dentry);
	remove_proc_entry("add_tail", linked_list_dentry);
	remove_proc_entry("add_head", linked_list_dentry);
	remove_proc_entry("foo", NULL);
	val_list_destroy(list);

	return 0;
}

static struct platform_driver foo_drv = {
	.probe	= foo_probe,
	.remove	= foo_remove,
	.driver	= {
		.name = DEVICE_NAME,
	}
};

static int __init foo_init(void)
{
	return platform_driver_register(&foo_drv);
}

static void __exit foo_exit(void)
{
	platform_driver_unregister(&foo_drv);
}

module_init(foo_init);
module_exit(foo_exit);

MODULE_ALIAS("foo-driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);
MODULE_DESCRIPTION("Linux is not Unix");
MODULE_AUTHOR("andrew, mrju.email@gmail.com");
