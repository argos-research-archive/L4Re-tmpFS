#pragma once

#include <l4/cxx/ipc_server>
#include <l4/fs/l4fs.h>
#include <l4/re/dataspace>

namespace L4fs
{
  class L4fs_fs
  {
  public:
    virtual int init(L4::Cap<L4Re::Dataspace> *rw_buf_ds);
    virtual int open(const char *name, int flags, mode_t &mode);
    virtual int mkdir(const char *name, mode_t mode);
    virtual int rename(const char *from);
    virtual int link(const char *from);
    virtual int symlink(const char *from);
    virtual int unlink(const char *name);
    virtual int rmdir(const char *name);
    virtual int umount();
    virtual int sync();
  };

  class L4fs_file
  {
  public:
		virtual int     readlink(int fh, size_t size);
    virtual ssize_t pread(int fh, size_t size, off64_t offset);
    virtual ssize_t pwrite(int fh, size_t size, off64_t offset);
    virtual ssize_t pgetdents(int fh, size_t size, off64_t offset);
    virtual int     close(int fh);
    virtual int     fstat64(int fh, struct stat64 *st);
    virtual int     fsync(int fh);
    virtual int     ftruncate64(int fh, off64_t offset);
  };

  class L4fs_ds
  {
  public:
    virtual int dataspace(int fh, L4::Cap<L4Re::Dataspace> *ds);
  };

  class L4fs_svr : public L4::Server_object/*_t<L4fs>*/, public L4fs_fs,
                   public L4fs_file, public L4fs_ds
  {
  public:
    int dispatch(l4_umword_t o, L4::Ipc::Iostream &ios);
  };
}
