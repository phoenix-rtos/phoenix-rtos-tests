/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - sys/stat.h
 *
 * TESTED:
 *    - fstat()
 *
 * Copyright 2023, 2026 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <sys/stat.h>
#include <sys/types.h>
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

static int fd;
static const char *path = "test_fstat.txt";
static const char *symPath = "test_fstat_symlink";
static const char *tempPath = "test_fstat";
static const char *chrPath = "/dev/fstatTest";

/*
=============================================================================
As of July 2023

Shared memory objects are NOT TESTED.

Currently, it's not possible to create shared memory objects in Phoenix-RTOS,
as the implementation for shmat() and related functions is missing.
=============================================================================
*/

TEST_GROUP(fstat_st_mode);
TEST_GROUP(fstat_st_nlink_size_blk_tim);
TEST_GROUP(fstat_errors);

TEST_SETUP(fstat_st_mode)
{
	/* Removing/clearing elements that could affect test case results */
	remove(path);
	remove(symPath);
	remove(tempPath);
}


TEST_TEAR_DOWN(fstat_st_mode)
{
}


TEST(fstat_st_mode, none)
{
	struct stat buffer;

	fd = open(path, O_CREAT, 00000);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(fstat_st_mode, gid)
{
	struct stat buffer;

	fd = open(path, O_CREAT, S_ISGID);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISGID, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(fstat_st_mode, uid)
{
	struct stat buffer;

	fd = open(path, O_CREAT, S_ISUID);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISUID, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(fstat_st_mode, uid_gid)
{
	struct stat buffer;

	fd = open(path, O_CREAT, S_ISUID | S_ISGID);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISUID | S_ISGID, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(fstat_st_mode, vtx)
{
	struct stat buffer;

	fd = open(path, O_CREAT, S_ISVTX);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(NONE_MODE | S_ISVTX, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(fstat_st_mode, permissions_all)
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

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(fstat_st_mode, reg_type)
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

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(fd);
	remove(path);

	fd = open(path, O_CREAT, 0000);

	permissions_set = NONE_MODE & ~mask;

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(fd);
	remove(path);
}


TEST(fstat_st_mode, dir_type)
{
	int dir_fd;
	struct stat buffer;

	TEST_ASSERT_EQUAL_INT(0, mkdir(tempPath, 0777));

	dir_fd = open(tempPath, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(0, fstat(dir_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFDIR);

	close(dir_fd);
	remove(tempPath);

	/* Testing with the lowest possible permissions, have to be 0400 to be accessible by fstat() */
	TEST_ASSERT_EQUAL_INT(0, mkdir(tempPath, 0400));

	dir_fd = open(tempPath, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(0, fstat(dir_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFDIR);

	close(dir_fd);
	remove(tempPath);
}


TEST(fstat_st_mode, symlink_type)
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

	sym_fd = open(symPath, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(0, fstat(sym_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFREG);
	TEST_ASSERT_EQUAL_INT(permissions_set, buffer.st_mode);

	close(sym_fd);
	unlink(symPath);
	close(fd);
	remove(path);
}


TEST(fstat_st_mode, fifo_type)
{
#if defined(__CPU_MCXN94X) || defined(__CPU_ZYNQMP)
	/* Disabled on MCXN947, ZYNQMP because of issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1338 */
	TEST_IGNORE();
#endif
	int fifo_fd;
	struct stat buffer;

	TEST_ASSERT_EQUAL_INT(0, mkfifo(tempPath, 0777));

	fifo_fd = open(tempPath, O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, fstat(fifo_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFIFO);

	close(fifo_fd);
	remove(tempPath);

	TEST_ASSERT_EQUAL_INT(0, mkfifo(tempPath, 0600));

	fifo_fd = open(tempPath, O_RDONLY | O_NONBLOCK);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fifo_fd);
	TEST_ASSERT_EQUAL_INT(0, fstat(fifo_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFIFO);

	close(fifo_fd);
	remove(tempPath);
}


TEST(fstat_st_mode, chr_type)
{
	struct stat buffer;
	int chr_fd;

	/* Disabled on Phoenix-RTOS because of #764 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/764 */
	TEST_IGNORE_MESSAGE("#764 issue");

	chr_fd = open(chrPath, O_RDWR);
	TEST_ASSERT_EQUAL_INT(0, fstat(chr_fd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFCHR);
	close(chr_fd);
}


TEST(fstat_st_mode, sock_type)
{
	int sfd;
	struct stat buffer;
	struct sockaddr_un addr;
	const char *socketPath = "/tmp/test_fstat_socket";

	unlink(socketPath);

	/* Create a new socket. */
	sfd = socket(AF_UNIX, SOCK_STREAM, 0);

	/* Set the address for the socket. */
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

	/* Bind the socket to the address. */
	TEST_ASSERT_EQUAL_INT(0, bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)));

	TEST_ASSERT_EQUAL_INT(0, fstat(sfd, &buffer));
	TEST_ASSERT_TRUE((buffer.st_mode & S_IFMT) == S_IFSOCK);

	close(sfd);
	unlink(socketPath);
}


TEST_SETUP(fstat_st_nlink_size_blk_tim)
{
	fd = open(path, O_CREAT, 0666);
}


TEST_TEAR_DOWN(fstat_st_nlink_size_blk_tim)
{
	close(fd);
	remove(path);
}


TEST(fstat_st_nlink_size_blk_tim, nlink)
{
	struct stat buffer;
	const char *anotherPath = "test_fstat_another_link_path";

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(1, buffer.st_nlink);


	TEST_ASSERT_EQUAL_INT(0, link(path, symPath));
	TEST_ASSERT_EQUAL_INT(0, link(path, tempPath));
	TEST_ASSERT_EQUAL_INT(0, link(path, anotherPath));


	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(4, buffer.st_nlink);

	unlink(symPath);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(3, buffer.st_nlink);

	unlink(tempPath);

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(2, buffer.st_nlink);

	unlink(anotherPath);
}


TEST(fstat_st_nlink_size_blk_tim, nlink_symloop_max)
{
	struct stat buffer;
	char source[16], target[16];
	int i, sym_fd;

	for (i = 1; i < SYMLOOP_MAX; i++) {
		sprintf(target, "link%d", i);

		TEST_ASSERT_EQUAL_INT(0, link(path, target));
	}

	sym_fd = open(path, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(0, fstat(sym_fd, &buffer));
	TEST_ASSERT_EQUAL_INT(SYMLOOP_MAX, buffer.st_nlink);
	close(sym_fd);

	for (i = 1; i < SYMLOOP_MAX; i++) {
		sprintf(source, "link%d", i);

		TEST_ASSERT_EQUAL_INT(0, unlink(source));
	}
}


TEST(fstat_st_nlink_size_blk_tim, size_blk_blocks)
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
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(size, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_GREATER_THAN_INT(1, buffer.st_blocks);
}


TEST(fstat_st_nlink_size_blk_tim, size_blk_blocks_zero)
{
	struct stat buffer;

	/* block size may differ from the target */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(0, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_EQUAL_INT(0, buffer.st_blocks);
}


TEST(fstat_st_nlink_size_blk_tim, size_blk_blocks_big)
{
	struct stat buffer;
	off_t newSize = INT_MAX / 2;

	TEST_ASSERT_EQUAL_INT(0, truncate(path, newSize));

	/* block size may differ from the target */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, buffer.st_size);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_blksize);
	TEST_ASSERT_GREATER_OR_EQUAL(0, buffer.st_blocks);
}


TEST(fstat_st_nlink_size_blk_tim, tim)
{
	struct stat buffer;
	int temp_fd;

	temp_fd = open(tempPath, O_CREAT, 0666);
	time_t currentTime = time(NULL);

	TEST_ASSERT_EQUAL_INT(0, fstat(temp_fd, &buffer));
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_ctim.tv_sec);
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_atim.tv_sec);
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_mtim.tv_sec);

	close(temp_fd);
	remove(tempPath);
}


TEST(fstat_st_nlink_size_blk_tim, a_m_tim_mod)
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

	TEST_ASSERT_EQUAL_INT(0, fstat(temp_fd, &buffer));
	TEST_ASSERT_INT_WITHIN(1, currentTime, buffer.st_ctim.tv_sec);
	TEST_ASSERT_EQUAL_INT(access_time.tv_sec, buffer.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT(modification_time.tv_sec, buffer.st_mtim.tv_sec);

	close(temp_fd);
	remove(tempPath);
}


TEST(fstat_st_nlink_size_blk_tim, st_dev_ino)
{
	struct stat buffer;

	/*
	 * We can't check the exact values of st_dev, st_ino because:
	 * st_dev describes the device on which this file resides
	 * st_ino contains the file's inode number.
	 */

	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &buffer));
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_dev);
	TEST_ASSERT_GREATER_THAN_INT(0, buffer.st_ino);
}


TEST_SETUP(fstat_errors)
{
}


TEST_TEAR_DOWN(fstat_errors)
{
}


TEST(fstat_errors, ebadf)
{
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


TEST_GROUP_RUNNER(fstat_st_mode)
{
	RUN_TEST_CASE(fstat_st_mode, none);
	RUN_TEST_CASE(fstat_st_mode, gid);
	RUN_TEST_CASE(fstat_st_mode, uid);
	RUN_TEST_CASE(fstat_st_mode, uid_gid);
	RUN_TEST_CASE(fstat_st_mode, vtx);
	RUN_TEST_CASE(fstat_st_mode, permissions_all);

	RUN_TEST_CASE(fstat_st_mode, reg_type);
	RUN_TEST_CASE(fstat_st_mode, dir_type);
	RUN_TEST_CASE(fstat_st_mode, symlink_type);
	RUN_TEST_CASE(fstat_st_mode, fifo_type);
	RUN_TEST_CASE(fstat_st_mode, sock_type);

/* Check only on Phoenix-RTOS */
#ifdef __phoenix__
	oid_t dev;
	dev.id = 0;
	create_dev(&dev, chrPath);
	RUN_TEST_CASE(fstat_st_mode, chr_type);
	remove(chrPath);
#endif
}


TEST_GROUP_RUNNER(fstat_st_nlink_size_blk_tim)
{
	RUN_TEST_CASE(fstat_st_nlink_size_blk_tim, nlink);
	RUN_TEST_CASE(fstat_st_nlink_size_blk_tim, nlink_symloop_max);
	RUN_TEST_CASE(fstat_st_nlink_size_blk_tim, size_blk_blocks);
	RUN_TEST_CASE(fstat_st_nlink_size_blk_tim, size_blk_blocks_zero);
	RUN_TEST_CASE(fstat_st_nlink_size_blk_tim, size_blk_blocks_big);
	RUN_TEST_CASE(fstat_st_nlink_size_blk_tim, tim);
	RUN_TEST_CASE(fstat_st_nlink_size_blk_tim, a_m_tim_mod);
	RUN_TEST_CASE(fstat_st_nlink_size_blk_tim, st_dev_ino);
}


TEST_GROUP_RUNNER(fstat_errors)
{
	/*
	 * fstat() only have following errnos: EBADF, EIO, EOVEFLOW
	 * There's not EIO, EOVERFLOW cases because of reproduction difficulties
	 */
	RUN_TEST_CASE(fstat_errors, ebadf);
}
