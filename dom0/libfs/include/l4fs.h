#pragma once

#include <sys/stat.h>
#include <unistd.h>

#include <l4/re/dataspace>
#include <l4/sys/kernel_object.h>
#include <l4/sys/types.h>

namespace L4fs
{
  enum
    {
      Max_path_size = (L4_UTCB_GENERIC_DATA_SIZE - 5) * sizeof(l4_umword_t),
      Max_rw_buffer_size = 1024*1024,
    };

  class L4fs : public L4::Kobject_t<L4fs, L4::Kobject>
  {
  public:
    int init(L4::Cap<L4Re::Dataspace> rw_buf_ds);
    int open(const char *name, int flags, mode_t &mode);
    int mkdir(const char* name, mode_t mode);
    int rmdir(const char *name);
    int rename(const char *from);
    int unlink(const char *name);
    int umount();
    int sync();

    int pread(int fh, size_t &size, off64_t offset);
    int pwrite(int fh, size_t &size, off64_t offset);
    int pgetdents(int fh, size_t &size, off64_t offset);
    int close(int fh);
    int symlink(const char *from);
    int link(const char *from);
		int readlink(int fh, size_t &size);
    int fstat64(int fh, struct stat64 *st);
    int fsync(int fh);
    int ftruncate64(int fh, off64_t offset);
    int dataspace(int fh, L4::Cap<L4Re::Dataspace> ds);
  };
}
