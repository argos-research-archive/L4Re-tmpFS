/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Lukas Grützmacher <lg2@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <l4/l4re_vfs/vfs.h>
#include <l4/l4re_vfs/backend>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <l4/cxx/ipc_stream>
#include <l4/fs/l4fs.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/sys/err.h>
#include <l4/sys/types.h>

int list_dir(const char *);
int create_file(const char *);
int create_directory(const char *);

int main(void)
{
	int res = 0;
	int fd = 0;
	int r = 0;
	char rbuf[256];
	const char *mount_source = "/";
	const char *mount_point = "/hellofs";
	//Files
	const char *hello_file = "/hellofs/hello";
	const char *myfile = "/hellofs/mydir/myfile";
	const char *myhello = "/hellofs/mydir/my_hello";
	const char *tmp_file = "/hellofs/tmp";
	//Directories
	const char *create_dir = "/hellofs/mydir";
	const char *subdir = "/hellofs/mydir/subdir";
	//other strs
	char *hello_str = "Hello FUSE-exfat on L4";

	sleep(1);

	printf("Mount the libfs with the fuse app as server\n");
	res = mount(mount_source, mount_point + 1, "l4fs", 0, 0);
	printf("MOUNT result: %d Code: %d\n", res, errno);
	if (res)
	{
		printf("\tError Msg: %s\n", strerror(errno));
		return 1;
	}
	assert(errno == 0);
	printf("-------- mount done -------\n\n");


	const char* mytestfile = "/hellofs/mytestfile";
	const char* myteststring1 = "HELLO HELLO HELLO HELLO\n";
	const char* myteststring2 = "TEST TEST TEST TEST TEST\n";


	printf("Creating the file %s\n", mytestfile);
	res = create_file(mytestfile);
	if (res)
	{
		return 1;
	}

	sleep(4);

	printf("Opening the file %s\n", mytestfile);

	fd = open(mytestfile, O_RDWR | O_TRUNC);
	if (!fd)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}

	sleep(4);

	printf("Attempt to write %s to %s\n", myteststring1, mytestfile);
	ssize_t s = write(fd, myteststring1, strlen(myteststring1) + 1);
	if (s < 0)
	{
		printf("\tError Msg: %s\n", strerror(errno));
		return 1;
	}
	close(fd);

	sleep(4);

	printf("Reading from %s\n", mytestfile);
	fd = open(mytestfile, O_RDONLY);
	while ((r = read(fd, rbuf, sizeof(rbuf) - 1)) > 0)
	{
		rbuf[sizeof(rbuf) - 1] = 0;
		printf("%s", rbuf);
	}
	printf("\n");
	close(fd);
	assert(errno == 0);

	sleep(4);

	printf("Reading from %s\n", mytestfile);
	fd = open(mytestfile, O_RDONLY);
	while ((r = read(fd, rbuf, sizeof(rbuf) - 1)) > 0)
	{
		rbuf[sizeof(rbuf) - 1] = 0;
		printf("%s", rbuf);
	}
	printf("\n");
	close(fd);
	assert(errno == 0);

	sleep(4);

	
	printf("Tests finished. If you want to conduct further tests, "
			"remove the return-statement in the next line. "
			"Note that some tests may fail since not all "
			"features are implemented in this or the underlying "
			"filesystem.\n");
	return 0;
	
	printf("\n\nStarting CREATION Phase\n");

	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);

	printf("Creating Directories\n");
	res = create_directory(create_dir);
	if (res)
	{
		return 1;
	}
	res = create_directory(subdir);
	if (res)
	{
		return 1;
	}

	printf("Creating Files\n");
	res = create_file(hello_file);
	if (res)
	{
		return 1;
	}
	res = create_file(tmp_file);
	if (res)
	{
		return 1;
	}
	res = create_file(myfile);
	if (res)
	{
		return 1;
	}
	res = create_file(myhello);
	if (res)
	{
		return 1;
	}

	printf("Verify that everything has been created\n");
	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);

	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);

	printf("\n\nCreation Phase done\n\n");

	printf("\n\nStart MESSING around with files\n\n");

	fd = open(hello_file, O_RDWR);
	if (!fd)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}

	printf("Attempt to write %s to %s\n", hello_str, hello_file);
	s = write(fd, hello_str, strlen(hello_str) + 1);
	if (s < 0)
	{
		printf("\tError Msg: %s\n", strerror(errno));
		return 1;
	}

	assert(errno == 0);
	printf("-------- write done --------\n\n");

	printf("Attempt to fstat64 %s with fd %i\n", hello_file, fd);
	struct stat64 buf;
	memset(&buf, 0, sizeof(struct stat64));
	res = fstat64(fd, &buf);
	if (res < 0)
	{
		printf("\tError No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);
	printf("-------- fstat64 done --------\n\n");

	sleep(4);

	printf("Closing File\n");
	res = close(fd);
	if (res < 0)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);
	printf("-------- close done -------\n\n");

	printf("Attempt to open %s\n", hello_file);
	fd = open(hello_file, O_RDONLY, S_IRWXU);
	if (!fd)
	{
		printf("\tError Msg: %s\n", strerror(errno));
		return 1;
	}
	assert(errno == 0);
	printf("-------- open done, fd: %i --------\n\n", fd);

	printf("Reading from %s\n", hello_file);
	while ((r = read(fd, rbuf, sizeof(rbuf) - 1)) > 0)
	{
		rbuf[sizeof(rbuf) - 1] = 0;
		printf("%s", rbuf);
	}
	assert(errno == 0);
	printf("-------- read done --------\n\n");
	printf("Closing File\n");
	res = close(fd);
	if (res < 0)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);
	printf("------- done --------------\n\n");


	char *tmp = "/hellofs/my_hello";

	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);
	hello_file = tmp;

	tmp = "/hellofs/hello";

	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);
	myhello = tmp;


	printf("Try to rename %s\n", myfile);
	tmp = "/hellofs/myfile";
	res = rename(myfile, tmp);
	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);
	myfile = tmp;

	printf("Try to rename %s\n", subdir);
	tmp = "/hellofs/subdir";
	res = rename(subdir, tmp);
	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);
	subdir = tmp;
	printf("-------- rename done --------\n\n");

	printf("Verify that files have been renamed\n");

	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);


	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);

	printf("\n\nMessing Phase done\n\n");

	printf("\n\nStarting DELETION Phase\n\n");

	printf("Delete %s\n", tmp_file);
	res = unlink(tmp_file);
	if (res != 0)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);
	tmp_file = "";

	printf("Attempt to rmdir %s\n", subdir);
	res = rmdir(subdir);
	if (res < 0)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	printf("-------- rmdir done ---------\n\n");
	subdir = "";

	printf("Verify that files are gone\n");
	res = list_dir(mount_point);
	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);
	res = list_dir(create_dir);
	if (res)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	assert(errno == 0);

	printf("\n\nDeletion Phase done\n\n");

	printf("-------- all done   ---------\n\n");
	return 0;
}


int list_dir(const char *path)
{
	assert(errno == 0);

	int pathlen = strlen(path);
	size_t total = 0;

	printf("Attempt to opendir %s\n", path);
	DIR *dp = opendir(path);
	if (!dp)
	{
		printf("\tError No: %u Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	printf("-------- opendir done -------\n\n");

	struct dirent *entry;
	printf("Listing: %s\n", path);
	while ((entry = readdir(dp)))
	{
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
		{
			//account for / and \0 char when mallocing, because strlen gives us length _without_ \0 char
			char *full_path = (char *) malloc(
					pathlen + strlen(entry->d_name) + 2);
			sprintf(full_path, "%s/%s", path, entry->d_name);

			struct stat64 stbuf;
			int res = stat64(full_path, &stbuf);
			if (res != 0)
			{
				printf("stat64(%s) ERROR No: %i\n", full_path, errno);
				printf("Msg: %s\n", strerror(errno));
				free(full_path);
				return 1;
			}
			free(full_path);

			total += stbuf.st_size;

			if (S_ISDIR(stbuf.st_mode))
			{
				printf("d");
			}
			else
			{
				printf("-");
			}
			if (stbuf.st_mode & S_IREAD)
			{
				printf("r");
			}
			else
			{
				printf("-");
			}
			if (stbuf.st_mode & S_IWRITE)
			{
				printf("w");
			}
			else
			{
				printf("-");
			}
			if (stbuf.st_mode & S_IEXEC)
			{
				printf("x");
			}
			else
			{
				printf("-");
			}
			printf(" %i %i %i", stbuf.st_nlink, stbuf.st_uid, stbuf.st_gid);
			if (stbuf.st_size < 1024)
			{
				printf(" %lli", stbuf.st_size);
			}
			else if (stbuf.st_size < 1024 * 1024)
			{
				printf(" %lliK", stbuf.st_size / 1024);
			}
			else
			{
				printf(" %lliM", stbuf.st_size / (1024 * 1024));
			}
			printf(" %li %s\n", stbuf.st_mtime, entry->d_name);
		}
		else
		{
			printf("%s\n", entry->d_name);
		}
	}
	if (errno)
	{
		printf("\treaddir Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}

	printf("total: %iK\n", total / 1024);
	printf("-------- readdir done --------\n\n");

	printf("Close %s\n", path);
	closedir(dp);
	printf("-------- closedir done -------\n\n");

	return 0;
}

int create_file(const char *filename)
{
	printf("Attempt to create %s\n", filename);
	int fd = open(filename, O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
	if (!fd)
	{
		printf("\tError No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	int res = close(fd);
	printf("-------- create done --------\n\n");
	return res;
}

int create_directory(const char *dirname)
{
	printf("Attempt to mkdir %s\n", dirname);
	int res = mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (res < 0)
	{
		printf("Error No: %i Msg: %s\n", errno, strerror(errno));
		return 1;
	}
	printf("-------- mkdir done ---------\n\n");
	return 0;
}

