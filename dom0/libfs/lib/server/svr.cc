#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <l4/fs/l4fs-sys.h>
#include <l4/fs/svr.h>

static inline L4::Ipc::Istream &operator >> (L4::Ipc::Istream &s, char * &name)
{
  static char name_buf[L4fs::Max_path_size];
  unsigned long name_len = L4fs::Max_path_size;

  s >> L4::Ipc::buf_cp_in(name_buf, name_len);
  name_buf[sizeof(name_buf)-1] = 0;
  name = name_buf;

  return s;
}

int
L4fs::L4fs_svr::dispatch(l4_umword_t /*obj*/, L4::Ipc::Iostream &ios)
{
  L4::Opcode opcode;

  ios >> opcode;
  switch (opcode)
    {
    case Svr_::Open:
	{
	  unsigned long flags;
	  mode_t mode;
	  char *name;
	  ios >> flags >> mode >> name;
	  int fh = open(name, flags, mode);
	  ios << fh << mode;
	  return 0;
	}
		case Svr_::Readlink:
	{
		int fh;
		size_t size;
		ios >> fh >> size;
		ssize_t ret = readlink(fh, size);
		ios << ret;
		return (ret < 0) ? ret : 0;
	}
    case Svr_::Pread:
	{
	  int fh;
	  size_t size;
          off64_t offset;
	  ios >> fh >> size >> offset;
	  ssize_t ret = pread(fh, size, offset);
	  ios << ret;
	  return (ret < 0) ? ret : 0;
	}
    case Svr_::Pwrite:
	{
	  int fh;
	  size_t size;
          off64_t offset;
	  ios >> fh >> size >> offset;
	  ssize_t ret = pwrite(fh, size, offset);
	  ios << ret;
	  return (ret < 0) ? ret : 0;
	}
    case Svr_::Pgetdents:
	{
	  int fh;
	  size_t size;
          off64_t offset;
	  ios >> fh >> size >> offset;
	  ssize_t ret = pgetdents(fh, size, offset);
	  ios << ret << offset;
	  return (ret < 0) ? ret : 0;
	}
    case Svr_::Close:
	{
	  int fh;
	  ios >> fh;
	  return close(fh);
	}
    case Svr_::Symlink:
	{
	  char *from;
	  ios >> from;
	  return symlink(from);
	}
    case Svr_::Link:
	{
	  char *from;
	  ios >> from;
	  return link(from);
	}
    case Svr_::Fstat64:
	{
	  int fh;
	  struct stat64 st;
	  ios >> fh;
	  int ret = fstat64(fh, &st);
	  ios << L4::Ipc::buf_cp_out(&st, 1);
	  return ret;
	}
    case Svr_::Fsync:
	{
	  int fh;
	  ios >> fh;
	  return fsync(fh);
	}
    case Svr_::Ftruncate64:
	{
	  int fh;
          off64_t offset;
	  ios >> fh >> offset;
	  return ftruncate64(fh, offset);
	}
    case Svr_::Get_dataspace:
	{
          int fh;
          ios >> fh;
          L4::Cap<L4Re::Dataspace> ds;
          int err = dataspace(fh, &ds);
	  ios << ds;
	  return err;
	}
    case Svr_::Rename:
	{
	  char *from;
	  ios >> from;
	  return rename(from);
	}
    case Svr_::Unlink:
	{
	  char *name;
	  ios >> name;
	  return unlink(name);
	}
    case Svr_::Rmdir:
	{
	  char *name;
	  ios >> name;
	  return rmdir(name);
	}
    case Svr_::Mkdir:
	{
	  char *name; mode_t mode;
	  ios >> mode >> name;
	  return mkdir(name, mode);
	}
    case Svr_::Init:
	{
	  L4::Cap<L4Re::Dataspace> cap;
	  init(&cap);
	  ios << cap;
	  return cap.is_valid() ? 0 : -L4_ENOMEM;
	}
    case Svr_::Umount:
      return umount();
    case Svr_::Sync:
      return sync();
    default:
      printf("Unknown file operation %d\n", opcode);
      return -L4_ENOSYS;
    }
}


int L4fs::L4fs_fs::init(L4::Cap<L4Re::Dataspace> *)      { return -L4_ENOSYS; }
int L4fs::L4fs_fs::open(const char *, int, mode_t&)      { return -L4_ENOSYS; }
int L4fs::L4fs_fs::mkdir(const char *, mode_t)           { return -L4_ENOSYS; }
int L4fs::L4fs_fs::rename(const char *)                  { return -L4_ENOSYS; }
int L4fs::L4fs_fs::unlink(const char *)                  { return -L4_ENOSYS; }
int L4fs::L4fs_fs::rmdir(const char *)                   { return -L4_ENOSYS; }
int L4fs::L4fs_fs::umount()                              { return -L4_ENOSYS; }
int L4fs::L4fs_fs::sync()                                { return -L4_ENOSYS; }
int L4fs::L4fs_fs::link(const char *)                    { return -L4_ENOSYS; }
int L4fs::L4fs_fs::symlink(const char *)                 { return -L4_ENOSYS; }

int     L4fs::L4fs_file::readlink(int, size_t)           { return -L4_ENOSYS; }
int     L4fs::L4fs_file::close(int)                      { return -L4_ENOSYS; }
ssize_t L4fs::L4fs_file::pread(int, size_t, off64_t)     { return -L4_ENOSYS; }
ssize_t L4fs::L4fs_file::pwrite(int, size_t, off64_t)    { return -L4_ENOSYS; }
ssize_t L4fs::L4fs_file::pgetdents(int, size_t, off64_t) { return -L4_ENOSYS; }
int     L4fs::L4fs_file::fstat64(int, struct stat64 *)   { return -L4_ENOSYS; }
int     L4fs::L4fs_file::fsync(int)                      { return -L4_ENOSYS; }
int     L4fs::L4fs_file::ftruncate64(int, off64_t)       { return -L4_ENOSYS; }

int L4fs::L4fs_ds::dataspace(int, L4::Cap<L4Re::Dataspace> *) { return -L4_ENOSYS; }
