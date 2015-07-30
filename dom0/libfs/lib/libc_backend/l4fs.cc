#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <l4/re/env>
#include <l4/util/util.h>
#include <l4/cxx/ipc_stream>
#include <l4/cxx/exceptions>
#include <l4/re/error_helper>
#include <l4/re/dataspace>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/env_ns>

#include <l4/fs/l4fs.h>
#include <l4/l4re_vfs/backend>


namespace L4fs {

using namespace L4Re::Vfs;
using cxx::Ref_ptr;

class Svr {

  public:
    Svr(char const *fs_name);

    L4::Cap<L4fs> svr() const { return _svr.get(); }
    void *rw_buffer()   const { return _rw_buf.get(); }

    inline void add_ref() throw() { _ref++; }
    inline unsigned long remove_ref() throw() { return _ref--; }

  private:
    L4Re::Util::Auto_cap<L4fs>::Cap _svr;
    L4Re::Util::Auto_cap<L4Re::Dataspace>::Cap _rw_buf_ds;
    L4Re::Rm::Auto_region<void *> _rw_buf;
    unsigned long _ref;
};


template < typename BE_FILE_CLASS >
class L4fs_file_base : public BE_FILE_CLASS
{
  public:
    explicit L4fs_file_base(Ref_ptr<Svr> const &svr, int fh)
      : _fh(fh), _svr(svr) { }
    ~L4fs_file_base() throw() { close(); }

    off64_t size() const throw(); // needed for lseek64 implementation in the base class
    int fstat64(struct stat64 *buf) const throw();
    int fdatasync() throw();
    int fsync() throw();
    int close() throw();
		int link(const char *src_path, const char *dst_path) throw();
		int symlink(const char *src_path, const char *dst_path) throw();
		ssize_t readlink(char *, size_t) throw();

  protected:
    int _fh;
    Ref_ptr<Svr> _svr;
};


class L4fs_dir : public L4fs_file_base<Be_file>
{
  public:
    explicit L4fs_dir(Ref_ptr<Svr> const &svr, int fh)
    : L4fs_file_base<Be_file>(svr, fh), _pos(0) { }

    off64_t lseek64(off64_t offset, int whence) throw();
    ssize_t getdents(char *, size_t) throw();
    int get_entry(const char *, int, mode_t, Ref_ptr<File> *) L4_NOTHROW;
    int mkdir(const char *, mode_t) throw();
    int rmdir(const char *) throw();
    int rename(const char *, const char *) throw();
    int unlink(const char *) throw();
    int stat64(const char *, struct stat64 *) throw();

  private:
    off64_t _pos; // FIXME: better use Be_file_pos instead?
};


class L4fs_file : public L4fs_file_base<Be_file_pos>
{
  public:
    explicit L4fs_file(Ref_ptr<Svr> const &svr, int fh) throw()
    : L4fs_file_base<Be_file_pos>(svr, fh) { }

    ssize_t preadv(const struct iovec *iov, int cnt, off64_t p) throw();
    ssize_t pwritev(const struct iovec *iov, int cnt, off64_t p) throw();
    int ftruncate64(off64_t offset) throw();
    L4::Cap<L4Re::Dataspace> data_space() const throw();
};


class L4fs_file_system : public Be_file_system {

  public:
    L4fs_file_system() : Be_file_system("l4fs") { }

    int mount(char const *source, unsigned long mountflags,
              void const *data, cxx::Ref_ptr<File> *dir) throw();
};

/*
 * ****************************************************************
 */

Svr::Svr(char const *fs_name) : _ref(0)
{
    using L4Re::chkcap;
    using L4Re::chksys;
    using L4Re::Util::cap_alloc;
    L4Re::Util::Env_ns ns;

    L4Re::Env const *e = L4Re::Env::env();

    _ref = 0;

    _svr = chkcap(ns.query<L4fs>(fs_name), "getting service capability", 0);

    _rw_buf_ds = chkcap(cap_alloc.alloc<L4Re::Dataspace>(),
                        "allocate dataspace capability");

    // do init call to server and get dataspace capability
    chksys(_svr->init(_rw_buf_ds.get()), "init call to l4fs service");

    chksys(e->rm()->attach(&_rw_buf, _rw_buf_ds->size(), L4Re::Rm::Search_addr,
                           _rw_buf_ds.get()),
           "attach buffer dataspace");
}

template < typename BE_FILE_CLASS >
int L4fs_file_base<BE_FILE_CLASS>::link(const char *src_path, const char *dst_path) throw() {
  size_t dst_size = strlen(dst_path) + 1;
  if (dst_size > Max_path_size)
      return -L4_ENAMETOOLONG;
  memcpy(_svr->rw_buffer(), dst_path, dst_size);

	int err = _svr->svr()->link(src_path);

	return err;
}

template < typename BE_FILE_CLASS >
int L4fs_file_base<BE_FILE_CLASS>::symlink(const char *src_path, const char *dst_path) throw() {
  size_t dst_size = strlen(dst_path) + 1;
  if (dst_size > Max_path_size)
      return -L4_ENAMETOOLONG;
  memcpy(_svr->rw_buffer(), dst_path, dst_size);

	int err = _svr->svr()->symlink(src_path);

	return err;
}

template < typename BE_FILE_CLASS >
int L4fs_file_base<BE_FILE_CLASS>::readlink(char *buf, size_t size) throw() {
	size_t client_size = size;
	int res = _svr->svr()->readlink(_fh, size);
	if(res < 0) {
		return res;
	}
 
	memcpy(buf, _svr->rw_buffer(), client_size);

	return size;
}

/*
 * **************************************************************** 
 */

template < typename BE_FILE_CLASS >
off64_t L4fs_file_base<BE_FILE_CLASS>::size() const throw() {

    struct stat64 st;
    int err = fstat64(&st);
    return (err == 0) ? st.st_size : err;
}

template < typename BE_FILE_CLASS >
int L4fs_file_base<BE_FILE_CLASS>::close() throw() {

    int err = (_fh >= 0) ? _svr->svr()->close(_fh) : -EBADF;
    _fh = -1;
    return err;
}

template < typename BE_FILE_CLASS >
int L4fs_file_base<BE_FILE_CLASS>::fstat64(struct stat64 *buf) const throw() {

    return _svr->svr()->fstat64(_fh, buf);
}

template < typename BE_FILE_CLASS >
int L4fs_file_base<BE_FILE_CLASS>::fdatasync() throw() {

    return fsync();
}


template < typename BE_FILE_CLASS >
int L4fs_file_base<BE_FILE_CLASS>::fsync() throw() {

    return _svr->svr()->fsync(_fh);
}


/*
 * **************************************************************** 
 */

// stolen from l4re_vfs/include/backend
off64_t L4fs_dir::lseek64(off64_t offset, int whence) throw() {

    off64_t r;
    switch (whence) {
        case SEEK_SET: r = offset; break;
        case SEEK_CUR: r = _pos + offset; break;
        case SEEK_END: r = size() + offset; break;
        default: return -EINVAL;
    };

    if (r < 0)
        return -EINVAL;

    _pos = r;
    return _pos;
}

ssize_t L4fs_dir::getdents(char *buf, size_t size) throw() {

    size_t max_size = size;
    int err = _svr->svr()->pgetdents(_fh, size, _pos);
    if (err == 0 && size > 0 && size <= max_size) {
        memcpy(buf, _svr->rw_buffer(), size);
        _pos += size;
        return size;
    }

    return err;
}

int L4fs_dir::get_entry(const char *name, int flags, mode_t mode,
                        Ref_ptr<File> *file) throw() {

    //printf("get_entry: '%s'\n", name);
    // VPFS has strict checking of flags parameter, make it happy
    // when L4Re VFS tries to open the root directory
    if (name[0] == 0)
        flags |= O_DIRECTORY;
    int fh = _svr->svr()->open(name, flags, mode);
    if (fh < 0)
        return fh;

    //printf("mode=%x,flags=%x\n", mode, flags);
    if (S_ISDIR(mode)) {
        //printf("open '%s' as directory\n", name);
        *file = new L4fs_dir(_svr, fh);
    } else  {
        //printf("open '%s' as file\n", name);
        *file = new L4fs_file(_svr, fh);
    }

    return (*file) ? 0 : -ENOMEM;
}

/*
 * **************************************************************** 
 */

L4::Cap<L4Re::Dataspace> L4fs_file::data_space() const throw() {

    using L4Re::Util::cap_alloc;
    L4Re::Util::Auto_cap<L4Re::Dataspace>::Cap ds = chkcap(cap_alloc.alloc<L4Re::Dataspace>());
    int err = _svr->svr()->dataspace(_fh, ds.get());
    return (err == 0) ? ds.release() : L4::Cap<L4Re::Dataspace>();
}

int L4fs_dir::mkdir(const char *name, mode_t mode) throw() {

    return _svr->svr()->mkdir(name, mode);
}

int L4fs_dir::rmdir(const char *name) throw() {

    return _svr->svr()->rmdir(name);
}

int L4fs_dir::rename(const char *from, const char *to) throw() {

  size_t to_size = strlen(to) + 1;
  if (to_size > Max_path_size)
      return -L4_ENAMETOOLONG;
  memcpy(_svr->rw_buffer(), to, to_size);
  return _svr->svr()->rename(from);
}

int L4fs_dir::unlink(const char *name) throw() {

    return _svr->svr()->unlink(name);
}

ssize_t L4fs_file::preadv(const struct iovec *iov, int cnt, off64_t p) throw() {

    ssize_t read_bytes = 0;

    for (int i = 0; i < cnt; ++i) {
        char  *buf = reinterpret_cast<char *>(iov[i].iov_base);
        size_t len = iov[i].iov_len;

        int err = _svr->svr()->pread(_fh, len, p);
        if (err < 0)
            return err;
        if (!err && len)
            memcpy(buf, _svr->rw_buffer(), len);
        read_bytes += len;
        p += len;
    }
    return read_bytes;
}

ssize_t L4fs_file::pwritev(const struct iovec *iov, int cnt, off64_t p) throw() {

    ssize_t written_bytes = 0;
    for (int i = 0; i < cnt; ++i) {
        char  *buf = reinterpret_cast<char *>(iov[i].iov_base);
        size_t len = iov[i].iov_len;

        memcpy(_svr->rw_buffer(), buf, len);
        int err = _svr->svr()->pwrite(_fh, len, p);
        if (err < 0)
            return err;
        written_bytes += len;
        p += len;
        if (len != iov[i].iov_len)
            break;
    }
    return written_bytes;
}

int L4fs_file::ftruncate64(off64_t offset) throw() {

    return _svr->svr()->ftruncate64(_fh, offset);
}

/*
 * **************************************************************** 
 */

int L4fs_file_system::mount(char const *source, unsigned long mountflags,
                            void const *data, cxx::Ref_ptr<File> *dir) throw() {

    try {
        (void)mountflags;
        (void)data;

        Ref_ptr<Svr> svr(new Svr("l4fs_svr"));

        mode_t mode = 0;
        int fh = svr->svr()->open(source, O_DIRECTORY, mode);
        if (fh < 0)
            return fh;

        *dir = new L4fs_dir(svr, fh);
        if (!dir)
            return -ENOMEM;

        return 0;
    } catch (L4::Runtime_error const &e) {
        return e.err_no();
    } catch (...) {
        return -EINVAL;
    }

}

} // namespace L4fs

/*
 * **************************************************************** 
 */

static L4fs::L4fs_file_system _l4fs_fs L4RE_VFS_FILE_SYSTEM_ATTRIBUTE;
