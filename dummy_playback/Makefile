# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the kernel modules.
#

KERN_DIR := /lib/modules/$(shell uname -r)/build

dummy-objs := track.o playback.o
obj-m := dummy.o

all:
	$(MAKE) -C $(KERN_DIR) M=`pwd` modules

clean:
	$(MAKE) -C $(KERN_DIR) M=`pwd` modules clean
