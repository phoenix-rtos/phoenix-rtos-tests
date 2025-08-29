/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing unistd.h pread and pwrite functions
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jakub Rak
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#include <unity_fixture.h>

#define LOREM      "Lorem ipsum dolor sit amet, consectetur adipiscing elit."
#define LOREM_LEN  56
#define LOREM_PATH "/tmp/lorem"

static ssize_t safe_pread(int fd, void *buf, size_t count, off_t offset)
{
	ssize_t bytes_read = 0;
	int err;
	do {
		err = pread(fd, (char *)buf + bytes_read, count - bytes_read, offset + bytes_read);
		if (err < 0) {
			if (errno == EINTR)
				continue;
			return err;
		}
		bytes_read += err;
	} while (err > 0);
	return count;
}


static ssize_t safe_pwrite(int fd, void *buf, size_t count, off_t offset)
{
	ssize_t bytes_written = 0;
	int err;
	do {
		err = pwrite(fd, (char *)buf + bytes_written, count - bytes_written, offset + bytes_written);
		if (err < 0) {
			if (errno == EINTR)
				continue;
			return err;
		}
		bytes_written += err;
	} while (err > 0);
	return count;
}


static ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t bytes_read = 0;
	int err;
	do {
		err = read(fd, (char *)buf + bytes_read, count - bytes_read);
		if (err < 0) {
			if (errno == EINTR)
				continue;
			return err;
		}
		bytes_read += err;
	} while (err > 0);
	return count;
}


static ssize_t safe_write(int fd, void *buf, size_t count)
{
	ssize_t bytes_written = 0;
	int err;
	do {
		err = write(fd, (char *)buf + bytes_written, count - bytes_written);
		if (err < 0) {
			if (errno == EINTR)
				continue;
			return err;
		}
		bytes_written += err;
	} while (err > 0);
	return count;
}


static int safe_open(const char *path, int oflag, mode_t mode)
{
	int err;
	do {
		err = open(path, oflag, mode);
	} while ((err < 0) && (errno == EINTR));

	return err;
}


static int safe_close(int fd)
{
	int err;
	do {
		err = close(fd);
	} while ((err < 0) && (errno == EINTR));

	return err;
}


/* Returns first free file descriptor greater or equal to fildes */
static int assert_free_fd(int fildes)
{
	int attempts = 0;

	while ((fcntl(fildes, F_GETFL) != -1) && attempts++ < 20) {
		fildes++;
	}

	return fildes;
}


TEST_GROUP(unistd_pread);


TEST_SETUP(unistd_pread)
{
	int fd = safe_open(LOREM_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	TEST_ASSERT_GREATER_THAN_INT(0, safe_write(fd, LOREM, LOREM_LEN));
	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
}


TEST_TEAR_DOWN(unistd_pread)
{
	unlink(LOREM_PATH);
}


/* Test basic reading on different offset */
TEST(unistd_pread, pread_offset)
{
	char buf[15];
	int fd = safe_open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	TEST_ASSERT_GREATER_THAN_INT(0, safe_pread(fd, buf, 14, 12));
	buf[14] = '\0';
	TEST_ASSERT_EQUAL_STRING("dolor sit amet", buf);
	TEST_ASSERT_GREATER_THAN_INT(0, safe_pread(fd, buf, 11, 6));
	buf[11] = '\0';
	TEST_ASSERT_EQUAL_STRING("ipsum dolor", buf);
	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
}


/* Test basic writing on different offset */
TEST(unistd_pread, pwrite_offset)
{
	char buf[LOREM_LEN + 1];
	int fd = safe_open(LOREM_PATH, O_RDWR, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	TEST_ASSERT_GREATER_THAN_INT(0, safe_pwrite(fd, "OVERWRITE", 9, 12));
	TEST_ASSERT_GREATER_THAN_INT(0, safe_read(fd, buf, 26));
	buf[26] = '\0';
	TEST_ASSERT_EQUAL_STRING("Lorem ipsum OVERWRITE amet", buf);
	TEST_ASSERT_GREATER_THAN_INT(0, safe_pwrite(fd, "ipsum dolor sit", 15, 6));
	TEST_ASSERT_GREATER_THAN_INT(0, safe_pread(fd, buf, LOREM_LEN, 0));
	buf[LOREM_LEN] = '\0';
	TEST_ASSERT_EQUAL_STRING(LOREM, buf);
	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
}


/* Test closed fd, incompatible open mode and nonexistent fd */
TEST(unistd_pread, pread_ebadf)
{
	char buf[4];

	int fd = safe_open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
	TEST_ASSERT_EQUAL_INT(-1, pread(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	fd = safe_open(LOREM_PATH, O_WRONLY, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(-1, pread(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
	fd = assert_free_fd(7);
	TEST_ASSERT_EQUAL_INT(-1, pread(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


/* Test closed fd, incompatible open mode and nonexistent fd */
TEST(unistd_pread, pwrite_ebadf)
{
	char buf[4] = "abc";

	int fd = safe_open(LOREM_PATH, O_WRONLY, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
	TEST_ASSERT_EQUAL_INT(-1, pwrite(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	fd = safe_open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(-1, pwrite(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
	fd = assert_free_fd(7);
	TEST_ASSERT_EQUAL_INT(-1, pwrite(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


/* Test negative offset */
TEST(unistd_pread, pread_einval)
{
	char buf[4];
	int fd = safe_open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(-1, pread(fd, buf, 2, -1));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
}


/* Test negative offset */
TEST(unistd_pread, pwrite_einval)
{
	char buf[4];
	int fd = safe_open(LOREM_PATH, O_WRONLY, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(-1, pwrite(fd, buf, 2, -1));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
}


/* Test trying to read on non-seekable file */
TEST(unistd_pread, pread_espipe)
{
	char buf[4];
	int pipe_fds[2];
	if (pipe(pipe_fds) != 0) {
		/* ignore test if not pipe() not supported (errno == ENOSYS) */
		TEST_ASSERT_EQUAL_INT(ENOSYS, errno);
		TEST_IGNORE();
	}
	TEST_ASSERT_EQUAL_INT(-1, pread(pipe_fds[1], buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(0, safe_close(pipe_fds[0]));
	TEST_ASSERT_EQUAL_INT(0, safe_close(pipe_fds[1]));
}


/* Test trying to read on non-seekable file */
TEST(unistd_pread, pwrite_espipe)
{
	char buf[4] = "abc";
	int pipe_fds[2];
	if (pipe(pipe_fds) != 0) {
		/* ignore test if not pipe() not supported (errno == ENOSYS) */
		TEST_ASSERT_EQUAL_INT(ENOSYS, errno);
		TEST_IGNORE();
	}
	TEST_ASSERT_EQUAL_INT(-1, pwrite(pipe_fds[0], buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(0, safe_close(pipe_fds[0]));
	TEST_ASSERT_EQUAL_INT(0, safe_close(pipe_fds[1]));
}


typedef struct {
	int ret;
	int fd;
	void *buf;
	int nbytes;
	off_t offset;
} args_t;


static void *pread_thread(void *arg)
{
	args_t *args = (args_t *)arg;
	args->ret = safe_pread(args->fd, args->buf, args->nbytes, args->offset);
	return NULL;
}


static void *pwrite_thread(void *arg)
{
	args_t *args = (args_t *)arg;
	args->ret = safe_pwrite(args->fd, args->buf, args->nbytes, args->offset);
	return NULL;
}


/* Test concurrent reading on multiple threads from the same fd */
TEST(unistd_pread, pread_multithread)
{
	int fd, i;
	char buffer[LOREM_LEN + 1];
	args_t args[4];
	pthread_t threads[4];
	fd = safe_open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	for (i = 0; i < 4; i++) {
		args[i].fd = fd;
		args[i].offset = 15 * i;
		args[i].buf = buffer + args[i].offset;
		args[i].nbytes = args[i].offset + 15 <= LOREM_LEN ? 15 : 11;
	}
	for (i = 3; i >= 0; i--)
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, pread_thread, &args[i]));
	for (i = 0; i < 4; i++) {
		TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
		TEST_ASSERT_NOT_EQUAL_INT(-1, args[i].ret);
	}
	buffer[LOREM_LEN] = '\0';
	TEST_ASSERT_EQUAL_STRING(LOREM, buffer);
	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
}


/* Test concurrent writing on multiple threads from the same fd */
TEST(unistd_pread, pwrite_multithread)
{
	int fd, i;
	char buffer[LOREM_LEN + 1] = LOREM;
	char read_buffer[LOREM_LEN + 1];
	args_t args[4];
	pthread_t threads[4];
	fd = safe_open("/tmp/newlorem", O_RDWR | O_CREAT, 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	for (i = 0; i < 4; i++) {
		args[i].fd = fd;
		args[i].offset = 15 * i;
		args[i].buf = buffer + args[i].offset;
		args[i].nbytes = args[i].offset + 15 <= LOREM_LEN ? 15 : 11;
	}
	for (i = 3; i >= 0; i--)
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, pwrite_thread, &args[i]));
	for (i = 0; i < 4; i++) {
		TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
		TEST_ASSERT_NOT_EQUAL_INT(-1, args[i].ret);
	}
	TEST_ASSERT_NOT_EQUAL(-1, safe_pread(fd, read_buffer, LOREM_LEN, 0));
	read_buffer[LOREM_LEN] = '\0';
	TEST_ASSERT_EQUAL_STRING(LOREM, read_buffer);
	TEST_ASSERT_EQUAL_INT(0, safe_close(fd));
	TEST_ASSERT_EQUAL_INT(0, unlink("/tmp/newlorem"));
}


TEST_GROUP_RUNNER(unistd_pread)
{
	RUN_TEST_CASE(unistd_pread, pread_offset);
	RUN_TEST_CASE(unistd_pread, pwrite_offset);
	RUN_TEST_CASE(unistd_pread, pread_ebadf);
	RUN_TEST_CASE(unistd_pread, pwrite_ebadf);
	RUN_TEST_CASE(unistd_pread, pread_einval);
	RUN_TEST_CASE(unistd_pread, pwrite_einval);
	RUN_TEST_CASE(unistd_pread, pread_espipe);
	RUN_TEST_CASE(unistd_pread, pwrite_espipe);
	RUN_TEST_CASE(unistd_pread, pread_multithread);
	RUN_TEST_CASE(unistd_pread, pwrite_multithread);
}
