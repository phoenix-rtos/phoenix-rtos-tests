/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - sys/stat.h
 *
 * TESTED:
 *    - stat()
 *    - lstat()
 *    - fstat()
 *
 * UNTESTED:
 *    - fstatat(), because its not implemented yet
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <limits.h>

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#include <unity_fixture.h>

#ifdef __phoenix__
#include <posix/utils.h>
#endif

#ifndef __phoenix__
#ifndef SYMLOOP_MAX
#define SYMLOOP_MAX 40
#endif
#endif

#define NONE_MODE 0100000
#define LINK_MODE 0120777

static int fd;
static const char *path = "test_stat.txt";
static const char *symPath = "test_stat_symlink";
static const char *tempPath = "test_stat";
static const char *chrPath = "/dev/statTest";

/*
=============================================================================
As of July 2023

Shared memory objects are NOT TESTED.

Currently, it's not possible to create shared memory objects in Phoenix-RTOS,
as the implementation for shmat() and related functions is missing.
=============================================================================
*/

TEST_GROUP(stat_mode);
TEST_GROUP(stat_nlink_size_blk_tim);
TEST_GROUP(stat_errno);

TEST_SETUP(stat_mode)
{
	/* Removing/clearing elements that could affect test case results */
	remove(path);
	remove(symPath);
	remove(tempPath);
}


TEST_TEAR_DOWN(stat_mode)
{
}


TEST(stat_mode, none)
{
	struct stat buffer;

	fd = open(path, O_CREAT, 00000);

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(stat_mode, gid)
{
	struct stat buffer;

	fd = open(path, O_CREAT, S_ISGID);

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISGID, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISGID, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISGID, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(stat_mode, uid)
{
	struct stat buffer;

	fd = open(path, O_CREAT, S_ISUID);

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISUID, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISUID, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISUID, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(stat_mode, uid_gid)
{
	struct stat buffer;

	fd = open(path, O_CREAT, S_ISUID | S_ISGID);

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISUID | S_ISGID, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISUID | S_ISGID, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISUID | S_ISGID, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(stat_mode, vtx)
{
	struct stat buffer;

	fd = open(path, O_CREAT, S_ISVTX);

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISVTX, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISVTX, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISVTX, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(stat_mode, permissions_all)
{
	struct stat buffer;
	int permissions_set;
	mode_t mask;

	/*
	 * We subtract the umask because, when we are setting in open() any mask, in reality it is set to 'any_mask & ~umask'
	 */
	mask = umask(0);
	permissions_set = (NONE_MODE | 07777) & ~mask;
	umask(mask);

	fd = open(path, O_CREAT, 07777);

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(stat_mode, reg_type)
{
	struct stat buffer;
	int permissions_set;
	mode_t mask;

	/*
	 * We subtract the umask because, when we are setting in open() any mask, in reality it is set to 'any_mask & ~umask'
	 */
	mask = umask(0);
	permissions_set = (NONE_MODE | 0666) & ~mask;
	umask(mask);

	fd = open(path, O_CREAT, 0666);

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(fd);
	remove(path);

	fd = open(path, O_CREAT, 0000);

	permissions_set = NONE_MODE & ~mask;

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(stat_mode, dir_type)
{
	int dir_fd;
	struct stat buffer;

	TEST_ASSERT_EQUAL_INT(0, mkdir(tempPath, 0777));

	TEST_ASSERT_EQUAL_INT(0, stat(tempPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFDIR);

	TEST_ASSERT_EQUAL_INT(0, lstat(tempPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFDIR);

	dir_fd = open(tempPath, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(0, fstat(dir_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFDIR);

	close(dir_fd);
	remove(tempPath);

	/* Testing with the lowest possible permissions, have to be 0400 to be accessible by fstat() */
	TEST_ASSERT_EQUAL_INT(0, mkdir(tempPath, 0400));

	TEST_ASSERT_EQUAL_INT(0, stat(tempPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFDIR);

	TEST_ASSERT_EQUAL_INT(0, lstat(tempPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFDIR);

	dir_fd = open(tempPath, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(0, fstat(dir_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFDIR);

	close(dir_fd);
	remove(tempPath);
}


TEST(stat_mode, symlink_type)
{
	int sym_fd;
	struct stat buffer;
	int permissions_set;
	mode_t mask;

	/*
	 * We subtract the umask because, when we are setting in open() any mask, in reality it is set to 'any_mask & ~umask'
	 */
	mask = umask(0);
	permissions_set = (NONE_MODE | 0666) & ~mask;
	umask(mask);

	fd = open(path, O_CREAT, 0666);

	unlink(symPath);

	TEST_ASSERT_EQUAL_INT(0, symlink(path, symPath));

	TEST_ASSERT_EQUAL_INT(0, stat(symPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	sym_fd = open(symPath, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(0, fstat(sym_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(sym_fd);
	unlink(symPath);
	close(fd);
	remove(path);

	/* Testing with the lowest possible permissions, have to be 0400 to be accessible by fstat() */
	fd = open(path, O_CREAT, 0400);

	permissions_set = (NONE_MODE | 0400) & ~mask;

	TEST_ASSERT_EQUAL_INT(0, symlink(path, symPath));

	TEST_ASSERT_EQUAL_INT(0, stat(symPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	sym_fd = open(symPath, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(0, fstat(sym_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(sym_fd);
	unlink(symPath);
	close(fd);
	remove(path);
}


TEST(stat_mode, symlink_lstat)
{
	struct stat buffer;
	int permissions_set;
	mode_t mask;

	/*
	 * We subtract the umask because, when we are setting in open() any mask, in reality it is set to 'any_mask & ~umask'
	 */
	mask = umask(0);
	permissions_set = (NONE_MODE | 0666) & ~mask;
	umask(mask);

	fd = open(path, O_CREAT, 0666);

	unlink(symPath);
	TEST_ASSERT_EQUAL_INT(0, symlink(path, symPath));

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	permissions_set = LINK_MODE;

	TEST_ASSERT_EQUAL_INT(0, lstat(symPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFLNK);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(fd);
	remove(path);
	unlink(symPath);

	/* Testing with the lowest possible permissions */

	fd = open(path, O_CREAT, 0000);

	permissions_set = (NONE_MODE | 0000) & ~mask;

	TEST_ASSERT_EQUAL_INT(0, symlink(path, symPath));

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	permissions_set = LINK_MODE;

	TEST_ASSERT_EQUAL_INT(0, lstat(symPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFLNK);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(fd);
	remove(path);
	unlink(symPath);
}


TEST(stat_mode, symloop_max)
{
	struct stat buffer;
	char source[16], target[16], link_name[16];
	int i;

	for (i = 0; i < SYMLOOP_MAX; i++) {
		sprintf(link_name, "link%d", i);

		unlink(link_name);
	}

	fd = open(path, O_CREAT, 0666);

	TEST_ASSERT_EQUAL_INT(0, symlink(path, "link0"));

	for (i = 0; i < SYMLOOP_MAX - 1; i++) {
		sprintf(source, "link%d", i);
		sprintf(target, "link%d", i + 1);

		TEST_ASSERT_EQUAL_INT(0, symlink(source, target));
	}

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(0, buffer.st_size);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(0, buffer.st_size);

	TEST_ASSERT_EQUAL_INT(0, lstat(target, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFLNK);
	TEST_ASSERT_EQUAL_INT(strlen(target), buffer.st_size);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(0, buffer.st_size);

	for (i = 0; i < SYMLOOP_MAX; i++) {
		sprintf(link_name, "link%d", i);

		TEST_ASSERT_EQUAL_INT(0, unlink(link_name));
	}

	close(fd);
	remove(path);
}


TEST(stat_mode, fifo_type)
{
/* Disabled on Phoenix-RTOS because of #680 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/680 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#680 issue");
#else
	int fifo_fd;
	struct stat buffer;

	TEST_ASSERT_EQUAL_INT(0, mkfifo(tempPath, 0777));

	TEST_ASSERT_EQUAL_INT(0, stat(tempPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFIFO);

	TEST_ASSERT_EQUAL_INT(0, lstat(tempPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFIFO);

	fifo_fd = open(tempPath, O_RDWR);
	TEST_ASSERT_EQUAL_INT(0, fstat(fifo_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFIFO);

	close(fifo_fd);
	remove(tempPath);

	TEST_ASSERT_EQUAL_INT(0, mkfifo(tempPath, 0000));

	TEST_ASSERT_EQUAL_INT(0, stat(tempPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFIFO);

	TEST_ASSERT_EQUAL_INT(0, lstat(tempPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFIFO);

	close(fifo_fd);
	remove(tempPath);
#endif
}


TEST(stat_mode, chr_type)
{
	struct stat buffer;
	int chr_fd;

	TEST_ASSERT_EQUAL_INT(0, stat(chrPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFCHR);
	TEST_ASSERT_EQUAL_INT(0, lstat(chrPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFCHR);

	/* Disabled on Phoenix-RTOS because of #764 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/764 */
	TEST_IGNORE_MESSAGE("#764 issue");

	chr_fd = open(chrPath, O_RDWR);
	TEST_ASSERT_EQUAL_INT(0, fstat(chr_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFCHR);
	close(chr_fd);
}


TEST(stat_mode, sock_type)
{
	int sfd;
	struct stat buffer;
	struct sockaddr_un addr;
	const char *socketPath = "/tmp/test_stat_socket";

	unlink(socketPath);

	/* Create a new socket. */
	sfd = socket(AF_UNIX, SOCK_STREAM, 0);

	/* Set the address for the socket. */
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

	/* Bind the socket to the address. */
	TEST_ASSERT_EQUAL_INT(0, bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)));

/* Disabled on Phoenix-RTOS because of #749 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/749 */
#ifndef __phoenix__
	TEST_ASSERT_EQUAL_INT(0, stat(socketPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFSOCK);

	TEST_ASSERT_EQUAL_INT(0, lstat(socketPath, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFSOCK);
#endif

	TEST_ASSERT_EQUAL_INT(0, fstat(sfd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFSOCK);

	close(sfd);
	unlink(socketPath);
}


TEST_SETUP(stat_nlink_size_blk_tim)
{
	fd = open(path, O_CREAT, 0666);
}


TEST_TEAR_DOWN(stat_nlink_size_blk_tim)
{
	close(fd);
	remove(path);
}


TEST(stat_nlink_size_blk_tim, nlink)
{
	struct stat buffer;
	const char *anotherPath = "test_stat_another_link_path";

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(1, buffer.st_nlink);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(1, buffer.st_nlink);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(1, buffer.st_nlink);


	TEST_ASSERT_EQUAL_INT(0, link(path, symPath));
	TEST_ASSERT_EQUAL_INT(0, link(path, tempPath));
	TEST_ASSERT_EQUAL_INT(0, link(path, anotherPath));


	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(4, buffer.st_nlink);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(4, buffer.st_nlink);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(4, buffer.st_nlink);

	unlink(symPath);

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(3, buffer.st_nlink);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(3, buffer.st_nlink);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(3, buffer.st_nlink);

	unlink(tempPath);

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(2, buffer.st_nlink);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(2, buffer.st_nlink);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(2, buffer.st_nlink);

	unlink(anotherPath);
}


TEST(stat_nlink_size_blk_tim, nlink_symloop_max)
{
	struct stat buffer;
	char source[16], target[16];
	int i, sym_fd;

	for (i = 1; i < SYMLOOP_MAX; i++) {
		sprintf(target, "link%d", i);

		TEST_ASSERT_EQUAL_INT(0, link(path, target));
	}

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(SYMLOOP_MAX, buffer.st_nlink);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(SYMLOOP_MAX, buffer.st_nlink);

	sym_fd = open(path, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(0, fstat(sym_fd, &buffer));
	TEST_ASSERT_EQUAL_INT(SYMLOOP_MAX, buffer.st_nlink);

	for (i = 1; i < SYMLOOP_MAX; i++) {
		sprintf(source, "link%d", i);

		TEST_ASSERT_EQUAL_INT(0, unlink(source));
	}
}


TEST(stat_nlink_size_blk_tim, size_blk_blocks)
{
	int size = 0, i;
	struct stat buffer;
	FILE *fp = fopen(path, "w");

	for (i = 0; i < 256; i++) {
		for (char ch = '0'; ch < 'z'; ch++) {
			fputc(ch, fp);
			size++;
		}
	}

	TEST_ASSERT_TRUE(size != 0);
	TEST_ASSERT_EQUAL_INT(0, fclose(fp));

	/* block size may differ from the target */
	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(size, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_GREATER_THAN_INT(1, buffer.st_blocks);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(size, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_GREATER_THAN_INT(1, buffer.st_blocks);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(size, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_GREATER_THAN_INT(1, buffer.st_blocks);
}


TEST(stat_nlink_size_blk_tim, size_blk_blocks_zero)
{
	struct stat buffer;

	/* block size may differ from the target */
	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(0, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_EQUAL_INT(0, buffer.st_blocks);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(0, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_EQUAL_INT(0, buffer.st_blocks);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(0, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_EQUAL_INT(0, buffer.st_blocks);
}


TEST(stat_nlink_size_blk_tim, size_blk_blocks_big)
{
	struct stat buffer;
	off_t newSize = INT_MAX / 2;

	TEST_ASSERT_EQUAL_INT(0, truncate(path, newSize));

	/* block size may differ from the target */
	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, buffer.st_blocks);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_GREATER_OR_EQUAL(0, buffer.st_blocks);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_GREATER_OR_EQUAL(0, buffer.st_blocks);
}


TEST(stat_nlink_size_blk_tim, size_symlink_lstat)
{
	struct stat buffer;

	unlink(symPath);

	TEST_ASSERT_EQUAL_INT(0, symlink(path, symPath));

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_EQUAL_INT(0, buffer.st_size);

	TEST_ASSERT_EQUAL_INT(0, lstat(symPath, &buffer));
	TEST_ASSERT_EQUAL_INT(strlen(path), buffer.st_size);

	unlink(symPath);
}


TEST(stat_nlink_size_blk_tim, tim)
{
	struct stat buffer;
	int temp_fd;

	temp_fd = open(tempPath, O_CREAT, 0666);
	time_t currentTime = time(NULL);


	TEST_ASSERT_EQUAL_INT(0, stat(tempPath, &buffer));
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_ctim.tv_sec);
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_atim.tv_sec);
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_mtim.tv_sec);

	TEST_ASSERT_EQUAL_INT(0, lstat(tempPath, &buffer));
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_ctim.tv_sec);
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_atim.tv_sec);
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_mtim.tv_sec);

	TEST_ASSERT_EQUAL_INT(0, fstat(temp_fd, &buffer));
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_ctim.tv_sec);
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_atim.tv_sec);
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_mtim.tv_sec);

	close(temp_fd);
	remove(tempPath);
}


TEST(stat_nlink_size_blk_tim, a_m_tim_mod)
{
	struct timeval access_time, modification_time;
	struct timeval times[2];
	struct stat buffer;
	time_t currentTime;
	int temp_fd;

	/* Setting new times values */
	access_time.tv_sec = CHAR_MAX;
	access_time.tv_usec = 0;
	modification_time.tv_sec = CHAR_MAX;
	modification_time.tv_usec = 0;

	times[0] = access_time;
	times[1] = modification_time;

	/* Creating file and getting default times values */
	temp_fd = open(tempPath, O_CREAT, 0666);
	currentTime = time(NULL);

	/* Changing times values*/
	TEST_ASSERT_EQUAL_INT(0, utimes(tempPath, times));

	TEST_ASSERT_NOT_EQUAL_INT(access_time.tv_sec, currentTime);
	TEST_ASSERT_NOT_EQUAL_INT(modification_time.tv_sec, currentTime);

	TEST_ASSERT_EQUAL_INT(0, stat(tempPath, &buffer));
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_ctim.tv_sec);
	TEST_ASSERT_EQUAL_INT(access_time.tv_sec, buffer.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT(modification_time.tv_sec, buffer.st_mtim.tv_sec);

	TEST_ASSERT_EQUAL_INT(0, lstat(tempPath, &buffer));
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_ctim.tv_sec);
	TEST_ASSERT_EQUAL_INT(access_time.tv_sec, buffer.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT(modification_time.tv_sec, buffer.st_mtim.tv_sec);

	TEST_ASSERT_EQUAL_INT(0, fstat(temp_fd, &buffer));
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_ctim.tv_sec);
	TEST_ASSERT_EQUAL_INT(access_time.tv_sec, buffer.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT(modification_time.tv_sec, buffer.st_mtim.tv_sec);

	close(temp_fd);
	remove(tempPath);
}


TEST(stat_nlink_size_blk_tim, st_dev_ino)
{
	struct stat buffer;

	/*
	 * We can't check the exact values of st_dev, st_ino because:
	 * st_dev describes the device on which this file resides
	 * st_ino contains the file's inode number.
	 */

	TEST_ASSERT_EQUAL_INT(0, stat(path, &buffer));
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_dev);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_ino);

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &buffer));
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_dev);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_ino);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_dev);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_ino);
}


TEST_SETUP(stat_errno)
{
	fd = open(path, O_CREAT | O_RDONLY, 0666);
}


TEST_TEAR_DOWN(stat_errno)
{
	close(fd);
	remove(path);
}


TEST(stat_errno, ebadf)
{
	/* EBADF only occur for fstat() */
	struct stat buffer;
	int fd_invaild = open("never_existed.txt", O_RDONLY);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fstat(INT_MAX, &buffer));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fstat(-1, &buffer));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fstat(fd_invaild, &buffer));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(stat_errno, eloop)
{
	/* Couldn't reproduce ELOOP for lstat */
	struct stat buffer;
	const char *secSymPath = "test_stat_sec_symlink";

	unlink(symPath);
	unlink(secSymPath);

	TEST_ASSERT_EQUAL_INT(0, symlink(secSymPath, symPath));
	TEST_ASSERT_EQUAL_INT(0, symlink(symPath, secSymPath));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, stat(symPath, &buffer));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);

	TEST_ASSERT_EQUAL_INT(0, unlink(symPath));
	TEST_ASSERT_EQUAL_INT(0, unlink(secSymPath));
}


TEST(stat_errno, enametoolong)
{
	struct stat buffer;
	char tooLongPath[PATH_MAX];
	memset(tooLongPath, 'a', PATH_MAX - 1);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, stat(tooLongPath, &buffer));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lstat(tooLongPath, &buffer));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


TEST(stat_errno, enoent)
{
	struct stat buffer;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, stat("\0", &buffer));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lstat("\0", &buffer));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, stat("test_stat_nonexistent_file", &buffer));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lstat("test_stat_nonexistent_file", &buffer));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, stat("test_stat_nonexistent_file/", &buffer));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	TEST_ASSERT_EQUAL_INT(-1, lstat("test_stat_nonexistent_file/", &buffer));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(stat_errno, enotdir)
{
/* Disabled on Phoenix-RTOS because of #682 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/682*/
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#682 issue");
#endif
	struct stat buffer;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, stat("test_stat.txt/", &buffer));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lstat("test_stat.txt/", &buffer));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST_GROUP_RUNNER(stat_mode)
{
	RUN_TEST_CASE(stat_mode, none);
	RUN_TEST_CASE(stat_mode, gid);
	RUN_TEST_CASE(stat_mode, uid);
	RUN_TEST_CASE(stat_mode, uid_gid);
	RUN_TEST_CASE(stat_mode, vtx);
	RUN_TEST_CASE(stat_mode, permissions_all);

	RUN_TEST_CASE(stat_mode, reg_type);
	RUN_TEST_CASE(stat_mode, dir_type);
	RUN_TEST_CASE(stat_mode, symlink_type);
	RUN_TEST_CASE(stat_mode, symlink_lstat);
	RUN_TEST_CASE(stat_mode, symloop_max);
	RUN_TEST_CASE(stat_mode, fifo_type);
	RUN_TEST_CASE(stat_mode, sock_type);

/* Check only on Phoenix-RTOS */
#ifdef __phoenix__
	oid_t dev;
	dev.id = 0;
	create_dev(&dev, chrPath);
	RUN_TEST_CASE(stat_mode, chr_type);
	remove(chrPath);
#endif
}


TEST_GROUP_RUNNER(stat_nlink_size_blk_tim)
{
	RUN_TEST_CASE(stat_nlink_size_blk_tim, nlink);
	RUN_TEST_CASE(stat_nlink_size_blk_tim, nlink_symloop_max);
	RUN_TEST_CASE(stat_nlink_size_blk_tim, size_blk_blocks);
	RUN_TEST_CASE(stat_nlink_size_blk_tim, size_blk_blocks_zero);
	RUN_TEST_CASE(stat_nlink_size_blk_tim, size_blk_blocks_big);
	RUN_TEST_CASE(stat_nlink_size_blk_tim, size_symlink_lstat);
	RUN_TEST_CASE(stat_nlink_size_blk_tim, tim);
	RUN_TEST_CASE(stat_nlink_size_blk_tim, a_m_tim_mod);
	RUN_TEST_CASE(stat_nlink_size_blk_tim, st_dev_ino);
}


TEST_GROUP_RUNNER(stat_errno)
{
	/*
	 * There's not EIO, EACCESS, EOVERFLOW cases because of reproduction difficulties
	 *
	 * fstat() only have following errnos: EBADF, EIO, EOVEFLOW
	 * So fstat() is not tested in other cases
	 *
	 */
	RUN_TEST_CASE(stat_errno, ebadf);
	RUN_TEST_CASE(stat_errno, eloop);
	RUN_TEST_CASE(stat_errno, enametoolong);
	RUN_TEST_CASE(stat_errno, enoent);
	RUN_TEST_CASE(stat_errno, enotdir);
}
