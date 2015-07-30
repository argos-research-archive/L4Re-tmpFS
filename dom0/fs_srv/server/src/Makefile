PKGDIR		?= ../..
L4DIR	?= $(PKGDIR)/../l4re-snapshot-2014092821/src/l4

TARGET		=	fs_srv
SRC_CC		=	main.cc
REQUIRES_LIBS	+=	libl4revfs-fs-tmpfs libfs-svr
include $(L4DIR)/mk/prog.mk
