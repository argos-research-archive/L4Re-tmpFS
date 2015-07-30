PKGDIR		?= ../..
L4DIR	?= $(PKGDIR)/../l4re-snapshot-2014092821/src/l4

TARGET		=	fstest
SRC_C		=
SRC_CC		=	main.cc
REQUIRES_LIBS	+=	libl4revfs-fs-l4fs libl4revfs-fs-tmpfs
include $(L4DIR)/mk/prog.mk
