PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../l4re-snapshot-2014092821/src/l4

TARGET           = dom0
SRC_CC           = main.cc dom0_server.cc lua_ipc_client.cc remote_dom0_instance.cc tcp_socket.cc tcp_client_socket.cc
SYSTEMS          = x86-l4f arm-l4f
REQUIRES_LIBS    = lwip libc_be_socket_lwip liblwip_netif_ankh libc_support_misc libc_be_file_stdin libl4revfs-fs-tmpfs libl4revfs-fs-l4fs libmount


include $(L4DIR)/mk/prog.mk
