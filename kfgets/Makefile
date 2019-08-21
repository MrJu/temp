# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the kernel modules.
#

KERN_DIR := /lib/modules/$(shell uname -r)/build

detect-objs += fgets.o kernel_io.o
obj-m += detect.o

all:
	$(MAKE) -C $(KERN_DIR) M=`pwd` modules

clean:
	$(MAKE) -C $(KERN_DIR) M=`pwd` modules clean
