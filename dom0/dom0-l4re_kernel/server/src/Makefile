PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../l4re-snapshot-2014092821/src/l4


TARGET          = l4re
SRC_CC         := main.cc globals.cc page_alloc.cc region.cc loader.cc \
                  loader_elf.cc mem.cc dispatcher.cc

DEFAULT_RELOC_x86   := 0xb0000000
DEFAULT_RELOC_arm   := 0xb0000000
DEFAULT_RELOC_ppc32 := 0xb0000000
DEFAULT_RELOC_amd64 := 0x70000000

MODE = sigma0

PRIVATE_INCDIR      += $(SRC_DIR)/ARCH-$(ARCH)

CXXFLAGS_amd64-l4f += -mcmodel=medium
CFLAGS_amd64-l4f += -mcmodel=medium
DEFINES += -DL4_CXX_NO_EXCEPTION_BACKTRACE

MMAP_BACKEND   := 

#REQUIRES_LIBS  := libc l4re-util libstdc++ libloader cxx_libc_io libc_be_minimal_log_io #libmount libc_be_l4refile libl4re-vfs libl4revfs-fs-l4fs libl4revfs-fs-tmpfs
REQUIRES_LIBS  :=  libc_be_minimal_log_io  cxx_libc_io libc_minimal l4re \
                  l4re-util libsupc++_minimal libloader libc_minimal_l4re
#                  libmount
#                  libl4revfs-fs-l4fs libmount

include $(L4DIR)/mk/prog.mk

