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
	const char* myteststring1 = "FOOBAR FOOBAR\n";

	sleep(10);

	printf("Reading from %s\n", mytestfile);
	fd = open(mytestfile, O_RDWR | O_TRUNC);
	while ((r = read(fd, rbuf, sizeof(rbuf) - 1)) > 0)
	{
		rbuf[sizeof(rbuf) - 1] = 0;
		printf("%s", rbuf);
	}
	printf("\n");
	close(fd);
	assert(errno == 0);

	sleep(4);

	printf("Attempt to write %s to %s\n", myteststring1, mytestfile);
	fd = open(mytestfile, O_RDWR | O_TRUNC);
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

