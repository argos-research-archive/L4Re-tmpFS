#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <l4/re/env>
#include <l4/cxx/ipc_stream>
#include <l4/cxx/exceptions>
#include <l4/re/error_helper>
#include <l4/re/dataspace>
#include <l4/re/util/cap_alloc>
#include <l4/fs/l4fs.h>
#include <l4/fs/l4fs-sys.h>

int L4fs::L4fs::init(L4::Cap<L4Re::Dataspace> rw_buf_ds)
{
  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Init);
  ios << L4::Ipc::Small_buf(rw_buf_ds.cap());
  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::open(const char *name, int flags, mode_t &mode)
{
  if (strlen(name) + 1 > Max_path_size)
    return -L4_ENAMETOOLONG;

  L4::Ipc::Iostream ios(l4_utcb());

  ios << L4::Opcode(Svr_::Open) << flags << mode << name;

  int ret = l4_error(ios.call(cap()));
  if (ret)
    return ret;
  int fh;
  ios >> fh >> mode;
  return fh;
}

int L4fs::L4fs::mkdir(const char *name, mode_t mode)
{
  if (strlen(name) + 1 > Max_path_size)
    return -L4_ENAMETOOLONG;

  L4::Ipc::Iostream ios(l4_utcb());

  ios << L4::Opcode(Svr_::Mkdir) << mode << name;

  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::rmdir(const char *name)
{
  if (strlen(name) + 1 > Max_path_size)
    return -L4_ENAMETOOLONG;

  L4::Ipc::Iostream ios(l4_utcb());

  ios << L4::Opcode(Svr_::Rmdir) << name;

  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::rename(const char *from)
{
  if (strlen(from) + 1 > Max_path_size)
    return -L4_ENAMETOOLONG;

  L4::Ipc::Iostream ios(l4_utcb());

  ios << L4::Opcode(Svr_::Rename) << from;

  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::unlink(const char *name)
{
  if (strlen(name) + 1 > Max_path_size)
    return -L4_ENAMETOOLONG;

  L4::Ipc::Iostream ios(l4_utcb());

  ios << L4::Opcode(Svr_::Unlink) << name;

  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::umount()
{
  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Umount);
  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::sync()
{
  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Sync);
  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::pread(int fh, size_t &size, off64_t offset)
{
  if (size > Max_rw_buffer_size)
    return -L4_ENOMEM;

  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Pread) << fh << size << offset;
  int ret = l4_error(ios.call(cap()));
  ios >> size;
  return ret;
}

int L4fs::L4fs::pwrite(int fh, size_t &size, off64_t offset)
{
  if (size > Max_rw_buffer_size)
    return -L4_ENOMEM;

  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Pwrite) << fh << size << offset;
  int ret = l4_error(ios.call(cap()));
  ios >> size;
  return ret;
}

int L4fs::L4fs::pgetdents(int fh, size_t &size, off64_t offset)
{
  if (size > Max_rw_buffer_size)
    return -L4_ENOMEM;

  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Pgetdents) << fh << size << offset;
  int ret = l4_error(ios.call(cap()));
  ios >> size;
  return ret;
}

int L4fs::L4fs::close(int fh)
{
  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Close) << fh;
  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::link(const char *from)
{
  if (strlen(from) + 1 > Max_path_size)
    return -L4_ENAMETOOLONG;

  L4::Ipc::Iostream ios(l4_utcb());

  ios << L4::Opcode(Svr_::Link);
  ios << L4::Ipc::Buf_cp_out<const char>(from, strlen(from) + 1);

  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::readlink(int fh, size_t &bufsize)
{

  L4::Ipc::Iostream ios(l4_utcb());

  ios << L4::Opcode(Svr_::Readlink) << fh << bufsize;
	int ret = l4_error(ios.call(cap()));

	ios >> bufsize;
  return ret;
}

int L4fs::L4fs::symlink(const char *from)
{
  if (strlen(from) + 1 > Max_path_size)
    return -L4_ENAMETOOLONG;

  L4::Ipc::Iostream ios(l4_utcb());

  ios << L4::Opcode(Svr_::Symlink);
  ios << L4::Ipc::Buf_cp_out<const char>(from, strlen(from) + 1);

  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::fstat64(int fh, struct stat64 *st)
{
  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Fstat64) << fh;
  int ret = l4_error(ios.call(cap()));
  unsigned long size = 1;
  ios >> L4::Ipc::buf_cp_in(st, size);
  return ret;
}

int L4fs::L4fs::fsync(int fh)
{
  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Fsync) << fh;
  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::ftruncate64(int fh, off64_t offset)
{
  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Ftruncate64) << fh << offset;
  return l4_error(ios.call(cap()));
}

int L4fs::L4fs::dataspace(int fh, L4::Cap<L4Re::Dataspace> ds)
{
  L4::Ipc::Iostream ios(l4_utcb());
  ios << L4::Opcode(Svr_::Get_dataspace) << fh;
  ios << L4::Small_buf(ds.cap());
  return l4_error(ios.call(cap()));
}
