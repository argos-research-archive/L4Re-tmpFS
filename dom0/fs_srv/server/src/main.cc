#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <l4/fs/svr.h>
#include <l4/re/util/cap_alloc> 
#include <l4/re/util/object_registry>
#include <l4/re/util/bitmap_cap_alloc>
#include <l4/l4re_vfs/vfs.h>
#include <l4/l4re_vfs/backend>

#define PATH_LEN 255

const char* tmpDir = "/tmp/";

//This class implements all the relevant functions.
//The according function gets automatically called whenever such
//an IPC-call arrives.
//Some of those functions are just forwarding the requests to the
//underlying FS (tmpfs) while others have to do a bit more work,
//like path translation or ds creation.
//Some lesser used functions aren't implemented at all due to
//the underlying tmpfs not having them implemented as well,
//so that they couldn't be tested anyway.

class Server: public L4fs::L4fs_svr
{
public:

	Server()
	{

		strcpy(actualPath, tmpDir);
		shortPath = actualPath + strlen(tmpDir);

		_rw_buf_ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();

		// allocate dataspace
		L4Re::Env::env()->mem_alloc()->alloc((size_t) L4fs::Max_rw_buffer_size,
				_rw_buf_ds, 0);

		// attach dataspace
		L4Re::Env::env()->rm()->attach(&rw_buf, _rw_buf_ds->size(),
				L4Re::Rm::Search_addr, _rw_buf_ds);
	}

	int init(L4::Cap<L4Re::Dataspace>* ds)
	{
		printf("int init (L4::Cap<L4Re::Dataspace>* ds)\n");
		*ds = _rw_buf_ds;
		return L4_EOK;
	}

	int open(const char *name, int flags, mode_t &mode)
	{
		printf("int open(const char *, int, mode_t&)\n"
				"%s\n%s,\n", name, getActualPath(name));
		int fh = ::open(getActualPath(name), flags, mode);

		return fh;
	}

	int mkdir(const char *name, mode_t mode)
	{
		printf("int mkdir(const char *, mode_t)\n");
		return ::mkdir(getActualPath(name), mode);
	}

	int rename(const char *from)
	{
		printf("int rename(const char *)\n");
		printf("from: %s, to: %s, strlen: %i\n", from, (char*) rw_buf,
				strlen((char*) rw_buf));
		char* tmp = (char*) malloc(PATH_LEN);
		strcpy(tmp, tmpDir);
		strcpy(tmp + strlen(tmpDir), (char*) rw_buf);
		printf("Strings. %s and %s\n", getActualPath(from), tmp);
		int result = ::rename(getActualPath(from), tmp);
		free(tmp);
		return result;
	}

	int unlink(const char *name)
	{
		printf("int unlink(const char *)\n");
		return ::unlink(getActualPath(name));
	}

	int rmdir(const char *name)
	{
		printf("int rmdir(const char *)\n");
		return ::rmdir(getActualPath(name));
	}

	int umount()
	{
		printf("int umount()\n");
		return -L4_ENOSYS;
	}

	int sync()
	{
		printf("int sync()\n");
		//::sync();
		return 0;
	}

	int link(const char *from)
	{
		printf("int link(const char *)\n");
		return -L4_ENOSYS;
	}

	int symlink(const char *from)
	{
		printf("int symlink(const char *)\n");
		return -L4_ENOSYS;
	}

	int readlink(int, size_t)
	{
		printf("int readlink(int, size_t)\n");
		return -L4_ENOSYS;
	}

	int close(int fd)
	{
		printf("int close(int)\n");
		return ::close(fd);
	}

	ssize_t pread(int fh, size_t size, off64_t offset)
	{
		printf("ssize_t pread(int, size_t, off64_t)\n");
		return ::pread64(fh, rw_buf, size, offset);
	}

	ssize_t pwrite(int fh, size_t size, off64_t offset)
	{
		printf("ssize_t pwrite(int, size_t, off64_t)\n");
		return ::pwrite64(fh, rw_buf, size, offset);
	}

	ssize_t pgetdents(int fh, size_t size, off64_t offset)
	{
		printf("ssize_t pgetdents(int, size_t, off64_t)\n");
		return -L4_ENOSYS; //::pgetdents(fh, size, offset);
	}

	int fstat64(int fh, struct stat64 *st)
	{
		printf("int fstat64(int, struct stat64 *)\n");
		return ::fstat64(fh, st);
	}

	int fsync(int)
	{
		printf("int fsync(int)\n");
		return -L4_ENOSYS;
	}

	int ftruncate64(int, off64_t)
	{
		printf("int ftruncate64(int, off64_t)\n");
		return -L4_ENOSYS;
	}

	int dataspace(int fh, L4::Cap<L4Re::Dataspace>* ds)
	{
		//If the underlying filesystem supports it,
		//we could just get the dataspace from there
		//and forward it to the requester:
		//cxx::Ref_ptr<L4Re::Vfs::File> fp = L4Re::Vfs::vfs_ops->get_file(fh);
		//*ds = fp->data_space();

		//But since our underlying tmpfs does not support that,
		//we need to copy the file content into a new dataspace.

		printf("int dataspace(int, L4::Cap<L4Re::Dataspace> *)\n");
		char* addr;
		struct stat buf;
		::fstat(fh, &buf);
		int size = buf.st_size;
		*ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
		L4Re::Env::env()->mem_alloc()->alloc(size, *ds, 0);
		L4Re::Env::env()->rm()->attach(&addr, (*ds)->size(),
				L4Re::Rm::Search_addr, *ds);

		size_t read = 0;
		while (read < size)
		{
			read += ::pread64(fh, addr + read, size, read);
		}

		return 0;
	}

private:
	//Buffer used for various purposes
	L4::Cap<L4Re::Dataspace> _rw_buf_ds;
	void* rw_buf;
	//Things used for path translation etc.
	char actualPath[PATH_LEN];
	char* shortPath;
	char* temp[PATH_LEN];
	char* getActualPath(const char* src)
	{
		strcpy(shortPath, src);
		return actualPath;
	}

};

int main(void)
{

	//Mount our underlying tmpfs
	int res = mount("/", tmpDir + 1, "tmpfs", 0, 0);
	if (res)
	{
		printf("MOUNT tmpfs, Error: %u (%s)\n", errno, strerror(errno));
		return 1;
	}
	printf("tmpfs mounted.\n");

	//Register our server
	Server svr;
	L4Re::Util::Registry_server<> reg;
	if (!reg.registry()->register_obj(&svr, "l4fs").is_valid())
	{
		printf("[lib] Error registering l4fs Server\n");
		return 0;
	}

	//Run the loop, so that IPC (FS) requests get handled.
	reg.loop();
	return 0;
}

