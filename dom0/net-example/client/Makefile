PKGDIR          ?= ../
L4DIR	?= $(PKGDIR)/../l4re-snapshot-2014092821/src/l4

TARGET           = client
SRC_C            = main.c 
SYSTEMS          = x86-l4f arm-l4f
REQUIRES_LIBS    = lwip libc_be_socket_lwip liblwip_netif_ankh libc_support_misc

include $(L4DIR)/mk/prog.mk

