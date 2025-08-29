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

#define LOREM               "Lorem ipsum dolor sit amet, consectetur adipiscing elit."
#define LOREM_LEN           56
#define LOREM_PATH          "/tmp/lorem"
#define OVERLAPPING_REPEATS 500


#ifdef __phoenix__
/* libphoenix internal functions */
extern ssize_t __safe_write(int fd, const void *buf, size_t size);
extern ssize_t __safe_pwrite(int fd, const void *buf, size_t size, off_t offset);
extern ssize_t __safe_read(int fd, void *buf, size_t size);
extern ssize_t __safe_pread(int fd, void *buf, size_t size, off_t offset);
extern int __safe_open(const char *path, int oflag, mode_t mode);
extern int __safe_close(int fd);
#endif


TEST_GROUP(unistd_pread);


/* Returns first free file descriptor greater or equal to fildes */
static int assert_free_fd(int fildes)
{
	int attempts = 0;

	while ((fcntl(fildes, F_GETFL) != -1) && attempts++ < 20) {
		fildes++;
	}

	if (attempts < 20) {
		return fildes;
	}

	TEST_FAIL_MESSAGE("Could not find a free file descriptor.");
	return -1; /* Unreachable */
}


TEST_SETUP(unistd_pread)
{
#ifdef __phoenix__
	int fd = __safe_open(LOREM_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL(LOREM_LEN, __safe_write(fd, LOREM, LOREM_LEN));
	TEST_ASSERT_EQUAL_INT(0, __safe_close(fd));
#endif
}


TEST_TEAR_DOWN(unistd_pread)
{
#ifdef __phoenix__
	unlink(LOREM_PATH);
#endif
}


#ifdef __phoenix__


/* Test basic reading on different offset */
TEST(unistd_pread, pread_offset)
{
	char buf[15];
	int fd = __safe_open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL(14, __safe_pread(fd, buf, 14, 12));
	buf[14] = '\0';
	TEST_ASSERT_EQUAL_STRING("dolor sit amet", buf);
	TEST_ASSERT_EQUAL(11, __safe_pread(fd, buf, 11, 6));
	buf[11] = '\0';
	TEST_ASSERT_EQUAL_STRING("ipsum dolor", buf);
	TEST_ASSERT_EQUAL_INT(0, __safe_close(fd));
}


/* Test basic writing on different offset */
TEST(unistd_pread, pwrite_offset)
{
	char buf[LOREM_LEN + 1];
	int fd = __safe_open(LOREM_PATH, O_RDWR, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL(9, __safe_pwrite(fd, "OVERWRITE", 9, 12));
	TEST_ASSERT_EQUAL(26, __safe_read(fd, buf, 26));
	buf[26] = '\0';
	TEST_ASSERT_EQUAL_STRING("Lorem ipsum OVERWRITE amet", buf);
	TEST_ASSERT_EQUAL(15, __safe_pwrite(fd, "ipsum dolor sit", 15, 6));
	TEST_ASSERT_EQUAL(LOREM_LEN, __safe_pread(fd, buf, LOREM_LEN, 0));
	buf[LOREM_LEN] = '\0';
	TEST_ASSERT_EQUAL_STRING(LOREM, buf);
	TEST_ASSERT_EQUAL_INT(0, __safe_close(fd));
}

#endif

/* Test closed fd, incompatible open mode and nonexistent fd */
TEST(unistd_pread, pread_ebadf)
{
	char buf[4];

	int fd = open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(0, close(fd));
	TEST_ASSERT_EQUAL_INT(-1, pread(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	fd = open(LOREM_PATH, O_WRONLY, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(-1, pread(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	TEST_ASSERT_EQUAL_INT(0, close(fd));
	fd = assert_free_fd(7);
	TEST_ASSERT_EQUAL_INT(-1, pread(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


/* Test closed fd, incompatible open mode and nonexistent fd */
TEST(unistd_pread, pwrite_ebadf)
{
	char buf[4] = "abc";

	int fd = open(LOREM_PATH, O_WRONLY, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(0, close(fd));
	TEST_ASSERT_EQUAL_INT(-1, pwrite(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	fd = open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(-1, pwrite(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	TEST_ASSERT_EQUAL_INT(0, close(fd));
	fd = assert_free_fd(7);
	TEST_ASSERT_EQUAL_INT(-1, pwrite(fd, buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


/* Test negative offset */
TEST(unistd_pread, pread_einval)
{
	char buf[4];
	int fd = open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(-1, pread(fd, buf, 2, -1));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	TEST_ASSERT_EQUAL_INT(0, close(fd));
}


/* Test negative offset */
TEST(unistd_pread, pwrite_einval)
{
	char buf[4];
	int fd = open(LOREM_PATH, O_WRONLY, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(-1, pwrite(fd, buf, 2, -1));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	TEST_ASSERT_EQUAL_INT(0, close(fd));
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
	TEST_ASSERT_EQUAL_INT(-1, pread(pipe_fds[0], buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);
	TEST_ASSERT_EQUAL_INT(0, close(pipe_fds[0]));
	TEST_ASSERT_EQUAL_INT(0, close(pipe_fds[1]));
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
	TEST_ASSERT_EQUAL_INT(-1, pwrite(pipe_fds[1], buf, 2, 1));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);
	TEST_ASSERT_EQUAL_INT(0, close(pipe_fds[0]));
	TEST_ASSERT_EQUAL_INT(0, close(pipe_fds[1]));
}

#ifdef __phoenix__

typedef struct {
	int ret;
	int fd;
	void *buf;
	size_t nbytes;
	off_t offset;
} args_t;


static void *pread_thread(void *arg)
{
	args_t *args = (args_t *)arg;
	args->ret = __safe_pread(args->fd, args->buf, args->nbytes, args->offset);
	return NULL;
}


static void *pwrite_thread(void *arg)
{
	args_t *args = (args_t *)arg;
	args->ret = __safe_pwrite(args->fd, args->buf, args->nbytes, args->offset);
	return NULL;
}


/* Test concurrent reading on multiple threads from the same fd */
TEST(unistd_pread, pread_multithread)
{
	const int chunk = LOREM_LEN / 4;
	const int remainder = LOREM_LEN % 4;
	int fd, i;
	char buffer[LOREM_LEN + 1];
	args_t args[4];
	pthread_t threads[4];
	fd = __safe_open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	for (i = 0; i < 4; i++) {
		args[i].fd = fd;
		args[i].offset = chunk * i;
		args[i].buf = buffer + args[i].offset;
		args[i].nbytes = args[i].offset + chunk <= LOREM_LEN ? chunk : remainder;
	}
	for (i = 3; i >= 0; i--)
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, pread_thread, &args[i]));
	for (i = 0; i < 4; i++) {
		TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
		TEST_ASSERT_EQUAL(chunk * (i + 1) <= LOREM_LEN ? chunk : remainder, args[i].ret);
	}
	buffer[LOREM_LEN] = '\0';
	TEST_ASSERT_EQUAL_STRING(LOREM, buffer);
	TEST_ASSERT_EQUAL_INT(0, __safe_close(fd));
}


static void *pread_overlapping_thread(void *arg)
{
	args_t *args = (args_t *)arg;
	for (int i = 0; i < OVERLAPPING_REPEATS; i++) {
		args->ret = __safe_pread(args->fd, args->buf, args->nbytes, args->offset);
		if (args->ret != args->nbytes) {
			break;
		}
	}
	return NULL;
}


/* Test concurrent reading overlapping fragments from the same fd */
TEST(unistd_pread, pread_multithread_overlapping)
{
	int fd, i;
	char buffer[4][6];
	args_t args[4];
	pthread_t threads[4];
	fd = __safe_open(LOREM_PATH, O_RDONLY, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	for (i = 0; i < 4; i++) {
		args[i].fd = fd;
		args[i].offset = 6 + i;
		args[i].buf = buffer[i];
		args[i].nbytes = 5;
	}
	for (i = 3; i >= 0; i--)
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, pread_overlapping_thread, &args[i]));
	for (i = 0; i < 4; i++) {
		TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
		TEST_ASSERT_EQUAL(5, args[i].ret);
		buffer[i][5] = '\0';
	}
	TEST_ASSERT_EQUAL_STRING("ipsum", buffer[0]);
	TEST_ASSERT_EQUAL_STRING("psum ", buffer[1]);
	TEST_ASSERT_EQUAL_STRING("sum d", buffer[2]);
	TEST_ASSERT_EQUAL_STRING("um do", buffer[3]);
	TEST_ASSERT_EQUAL_INT(0, __safe_close(fd));
}


/* Test concurrent writing on multiple threads from the same fd */
TEST(unistd_pread, pwrite_multithread)
{
	const int chunk = LOREM_LEN / 4;
	const int remainder = LOREM_LEN % 4;
	int fd, i;
	char buffer[LOREM_LEN + 1] = LOREM;
	char read_buffer[LOREM_LEN + 1];
	args_t args[4];
	pthread_t threads[4];
	fd = __safe_open("/tmp/newlorem", O_RDWR | O_CREAT, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	for (i = 0; i < 4; i++) {
		args[i].fd = fd;
		args[i].offset = chunk * i;
		args[i].buf = buffer + args[i].offset;
		args[i].nbytes = args[i].offset + chunk <= LOREM_LEN ? chunk : remainder;
	}
	for (i = 3; i >= 0; i--)
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, pwrite_thread, &args[i]));
	for (i = 0; i < 4; i++) {
		TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
		TEST_ASSERT_EQUAL(chunk * (i + 1) <= LOREM_LEN ? chunk : remainder, args[i].ret);
	}
	TEST_ASSERT_EQUAL(LOREM_LEN, __safe_pread(fd, read_buffer, LOREM_LEN, 0));
	read_buffer[LOREM_LEN] = '\0';
	TEST_ASSERT_EQUAL_STRING(LOREM, read_buffer);
	TEST_ASSERT_EQUAL_INT(0, __safe_close(fd));
	TEST_ASSERT_EQUAL_INT(0, unlink("/tmp/newlorem"));
}


static void *pwrite_overlapping_thread(void *arg)
{
	args_t *args = (args_t *)arg;
	for (int i = 0; i < OVERLAPPING_REPEATS; i++) {
		args->ret = __safe_pwrite(args->fd, args->buf, args->nbytes, args->offset);
		if (args->ret != args->nbytes) {
			break;
		}
	}
	return NULL;
}


/* Test concurrent writing overlapping fragments from the same fd */
TEST(unistd_pread, pwrite_multithread_overlapping)
{
	int fd, i;
	char buffer[4][6] = { "ipsum", "psum ", "sum d", "um do" };
	char read_buffer[9];
	args_t args[4];
	pthread_t threads[4];
	fd = __safe_open("/tmp/ipsum", O_RDWR | O_CREAT, 0777);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	for (i = 0; i < 4; i++) {
		args[i].fd = fd;
		args[i].offset = i;
		args[i].buf = buffer[i];
		args[i].nbytes = 5;
	}
	for (i = 3; i >= 0; i--)
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, pwrite_overlapping_thread, &args[i]));
	for (i = 0; i < 4; i++) {
		TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
		TEST_ASSERT_EQUAL(5, args[i].ret);
		buffer[i][5] = '\0';
	}
	TEST_ASSERT_EQUAL(8, __safe_pread(fd, read_buffer, 8, 0));
	read_buffer[8] = '\0';
	TEST_ASSERT_EQUAL_STRING("ipsum do", read_buffer);
	TEST_ASSERT_EQUAL_INT(0, __safe_close(fd));
	TEST_ASSERT_EQUAL_INT(0, unlink("/tmp/ipsum"));
}


/* TODO: write more demanding tests for overalpping read and write */
#endif


TEST_GROUP_RUNNER(unistd_pread)
{
#ifdef __phoenix__
	RUN_TEST_CASE(unistd_pread, pread_offset);
	RUN_TEST_CASE(unistd_pread, pwrite_offset);
#endif
	RUN_TEST_CASE(unistd_pread, pread_ebadf);
	RUN_TEST_CASE(unistd_pread, pwrite_ebadf);
	RUN_TEST_CASE(unistd_pread, pread_einval);
	RUN_TEST_CASE(unistd_pread, pwrite_einval);
	RUN_TEST_CASE(unistd_pread, pread_espipe);
	RUN_TEST_CASE(unistd_pread, pwrite_espipe);
#ifdef __phoenix__
	RUN_TEST_CASE(unistd_pread, pread_multithread);
	RUN_TEST_CASE(unistd_pread, pread_multithread_overlapping);
	RUN_TEST_CASE(unistd_pread, pwrite_multithread);
	RUN_TEST_CASE(unistd_pread, pwrite_multithread_overlapping);
#endif
}
