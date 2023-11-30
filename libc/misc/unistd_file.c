/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing unistd.h file related functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Mateusz Niewiadomski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <unity_fixture.h>

#define LINE1 "line1\n"
#define LINE2 "lineline2\n" /* longer than LINE1 */
#define LINE3 "line3\n"     /* same length as LINE1 */
#define LINE4 "\n"

#define FNAME "unistd_file_testfile"


static int fd;
static char buf[50];


/* Assert that string pointed to by str has been correctly written to a specified file */
static void assert_write(const int fildes, const char *str)
{
	int toWrite;
	int temp;
	int cnt = 0;
	int written = 0;

	/* include NULL termination character */
	toWrite = strlen(str) + 1;
	/* Assuming that strlen attempts would be sufficient */
	while (written < toWrite && cnt++ <= toWrite) {
		temp = write(fildes, &str[written], toWrite - written);
		if (((temp) == -1) && (errno == EINTR)) {
			continue;
		}
		TEST_ASSERT_GREATER_OR_EQUAL(0, temp);
		written += temp;
	}

	TEST_ASSERT_EQUAL(toWrite, written);
}


/* Assert that n characters have been read to dest from the specified file */
static void assert_read(const int fildes, char *dest, const int n)
{
	int temp;
	int cnt = 0;
	int nread = 0;

	/* Assuming that n attempts would be sufficient */
	while (nread < n && cnt++ < n) {
		temp = read(fildes, dest, n - nread);
		if (((temp) == -1) && (errno == EINTR)) {
			continue;
		}
		TEST_ASSERT_GREATER_OR_EQUAL(0, temp);
		nread += temp;
	}

	TEST_ASSERT_EQUAL(n, nread);
}


/* Assert that only n characters have been read to dest from the specified file.
   Trying to read 2*n bytes in every attempt */
static void assert_read_more(const int fildes, const char *dest, const int n)
{
	int temp;
	int cnt = 0;
	int nread = 0;

	/* Assuming that n attempts would be sufficient */
	while (nread < n && cnt++ < n) {
		/* Always try to read 2 times more than it's possible */
		temp = read(fildes, buf, 2 * n);
		if (((temp) == -1) && (errno == EINTR)) {
			continue;
		}
		TEST_ASSERT_GREATER_OR_EQUAL(0, temp);
		nread += temp;
	}
	TEST_ASSERT_EQUAL(n, nread);
}


/* Returns first free file descriptor greater or equal to fildes */
static int assert_free_fd(int fildes)
{
	int attempts = 0;

	while ((fcntl(fildes, F_GETFL) != -1 ) && attempts++ < 20) {
		fildes++;
	}

	return fildes;
}


TEST_GROUP(unistd_file);


TEST_SETUP(unistd_file)
{
	/* open a file */
	fd = open(FNAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
	memset(buf, '\0', sizeof(buf));
}


TEST_TEAR_DOWN(unistd_file)
{
	memset(buf, '\0', sizeof(buf));
	if (fd >= 0) {
		TEST_ASSERT_EQUAL_INT(0, close(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, remove(FNAME));
}


TEST(unistd_file, file_close)
{
	/* close an opened descriptor */
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	/* try to close a closed descriptor */
	TEST_ASSERT_EQUAL_INT(-1, close(fd));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	/* this test closes fd by itself, invalidate it */
	fd = -1;
}


/* Simple usage of read() and write() */
TEST(unistd_file, file_readwrite_nbytes)
{
	assert_write(fd, LINE1);

	/* repoen file readonly */
	TEST_ASSERT_EQUAL_INT(0, close(fd));
	fd = open(FNAME, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
	{
		/* try to read LINE1 from file */
		assert_read(fd, buf, sizeof(LINE1));
		TEST_ASSERT_EQUAL_STRING(LINE1, buf);

		/* assert EOF */
		TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
	}
}


TEST(unistd_file, file_write_zero)
{
	/* test writing no bytes */
	TEST_ASSERT_EQUAL_INT(0, write(fd, NULL, 0));
}


/* simultaneous write with two open descriptors */
TEST(unistd_file, file_write_reopened)
{
	int fdr;
	int fd2;

	/* reopen file descriptor and open another for reading */
	fd2 = open(FNAME, O_WRONLY);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd2);
	fdr = open(FNAME, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fdr);
	{
		/* write to the same file consecutively from two different file descriptors */
		assert_write(fd, LINE1);
		assert_write(fd2, LINE2);

		/* try to read too much for simple error detection */
		assert_read_more(fdr, buf, sizeof(LINE2));
		TEST_ASSERT_EQUAL_STRING(LINE2, buf);

		/* test for EOF */
		TEST_ASSERT_EQUAL_INT(0, read(fdr, buf, 2 * (sizeof(LINE2))));
	}
	TEST_ASSERT_EQUAL_INT(0, close(fdr));
	TEST_ASSERT_EQUAL_INT(0, close(fd2));
}


/* test simultaneous write() on duplicated file descriptor */
TEST(unistd_file, file_write_dup)
{
	int sum;
	int fdr;
	int fd2;

	/* duplicate file descriptor and open another for reading */
	fd2 = dup(fd);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd2);
	fdr = open(FNAME, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fdr);
	{
		/* write to the same file consecutively from descriptor and its copy */
		assert_write(fd, LINE1);
		assert_write(fd2, LINE2);

		sum = (sizeof(LINE1)) + (sizeof(LINE2));
		assert_read(fdr, buf, sum);
		TEST_ASSERT_EQUAL_STRING_LEN(LINE1, &buf[0], sizeof(LINE1));
		TEST_ASSERT_EQUAL_STRING(LINE2, &buf[sizeof(LINE1)]);
	}
	TEST_ASSERT_EQUAL_INT(0, close(fdr));
	TEST_ASSERT_EQUAL_INT(0, close(fd2));
}


/* Test write() to closed descriptor */
TEST(unistd_file, file_readwrite_badfd)
{
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	TEST_ASSERT_EQUAL_INT(-1, write(fd, LINE1, sizeof(LINE1)));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
	TEST_ASSERT_EQUAL_INT(-1, read(fd, buf, sizeof(LINE1)));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	/* this test closes fd by itself, invalidate it */
	fd = -1;
}


/* test write() outside of the file */
TEST(unistd_file, file_write_incrlength)
{
	/* set offset outside of the file */
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, lseek(fd, 1, SEEK_SET));
	/* The file's length should be increased */
	assert_write(fd, LINE2);
	/* Check whether we can read written data */
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, lseek(fd, 1, SEEK_SET));
	assert_read(fd, buf, sizeof(LINE2));
	TEST_ASSERT_EQUAL_STRING(LINE2, buf);
}


/* test write on readonly file descriptor */
TEST(unistd_file, file_write_readonly)
{
	int fd2;

	fd2 = open(FNAME, O_RDONLY, S_IRUSR);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd2);
	{
		TEST_ASSERT_EQUAL_INT(-1, write(fd2, LINE1, sizeof(LINE1)));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	TEST_ASSERT_EQUAL_INT(0, close(fd2));
}


/* test simple pipe() usage */
TEST(unistd_file, file_readwrite_pipe)
{
	int p[2];

	if (pipe(p) != 0) {
		/* ignore test if pipe() not supported (errno == ENOSYS) */
		TEST_ASSERT_EQUAL_INT(ENOSYS, errno);
		TEST_IGNORE();
	}
	else {
		assert_write(p[1], LINE1);
		assert_read(p[0], buf, sizeof(LINE1));

		TEST_ASSERT_EQUAL_INT(0, close(p[0]));
		TEST_ASSERT_EQUAL_INT(0, close(p[1]));
	}
}


/* test simple use of lseek() */
TEST(unistd_file, file_lseek)
{
	/* fill file with content */
	assert_write(fd, LINE1);

	/* fallback to absolute beginning */
	TEST_ASSERT_EQUAL_INT(0, lseek(fd, (off_t)0, SEEK_SET));
	/* read line */
	assert_read(fd, buf, sizeof(LINE1));
	TEST_ASSERT_EQUAL_STRING(LINE1, buf);
	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));

	/* fallback relative to current */
	TEST_ASSERT_EQUAL_INT(0, (off_t)lseek(fd, -(off_t)sizeof(LINE1), SEEK_CUR));
	/* read line */
	assert_read(fd, buf, sizeof(LINE1));
	TEST_ASSERT_EQUAL_STRING(LINE1, buf);
	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));

	/* fallback relative to file end */
	TEST_ASSERT_EQUAL_INT(0, (off_t)lseek(fd, -(off_t)sizeof(LINE1), SEEK_END));
	/* read line */
	assert_read(fd, buf, sizeof(LINE1));
	TEST_ASSERT_EQUAL_STRING(LINE1, buf);
	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
}


/* test lseek() outside of the file */
TEST(unistd_file, file_lseek_pastfile)
{
	int fd2;
	/* offsets for SEEK_SET, SEEK_CUR and SEEK_END tests of past-the-file lseek() */
	off_t setoff = 2 * (sizeof(LINE1));
	off_t curoff = setoff + sizeof(LINE1);
	off_t endoff = curoff + sizeof(LINE1);

	/* fill file with content */
	assert_write(fd, LINE1);

	/* reopen file */
	fd2 = open(FNAME, O_RDONLY, S_IRUSR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd2);
	{
		/* set fd2 past current EOF position using SEEK_SET */
		TEST_ASSERT_EQUAL_INT(setoff, lseek(fd2, setoff, SEEK_SET));
		/* fill empty gap between fd and fd2 and add new data */
		assert_write(fd, LINE1);
		assert_write(fd, LINE2);
		/* assert correct read from fd2 */
		assert_read(fd2, buf, sizeof(LINE2));
		TEST_ASSERT_EQUAL_STRING(LINE2, buf);

		/* set fd2 past current EOF position using SEEK_CUR */
		TEST_ASSERT_GREATER_OR_EQUAL(curoff, lseek(fd2, sizeof(LINE1), SEEK_CUR));
		/* fill empty gap between fd and fd2 and add new data */
		assert_write(fd, LINE1);
		assert_write(fd, LINE2);
		/* assert correct read from fd2 */
		assert_read(fd2, buf, sizeof(LINE2));
		TEST_ASSERT_EQUAL_STRING(LINE2, buf);

		/* set fd2 past current EOF position using SEEK_END */
		TEST_ASSERT_GREATER_OR_EQUAL(endoff, lseek(fd2, sizeof(LINE1), SEEK_END));
		/* fill empty gap between fd and fd2 and add new data */
		assert_write(fd, LINE1);
		assert_write(fd, LINE2);
		/* assert correct read from fd2 */
		assert_read(fd2, buf, sizeof(LINE2));
		TEST_ASSERT_EQUAL_STRING(LINE2, buf);
	}
	TEST_ASSERT_EQUAL_INT(0, close(fd2));
}


/* test lseek() with offsets that result in overall negative offset in file */
TEST(unistd_file, file_lseek_negative)
{
	assert_write(fd, LINE1);

	TEST_ASSERT_EQUAL_INT(-1, lseek(fd, (off_t)-1 * sizeof(LINE1), SEEK_SET));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	TEST_ASSERT_EQUAL_INT(-1, lseek(fd, (off_t)-2 * sizeof(LINE1), SEEK_CUR));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	TEST_ASSERT_EQUAL_INT(-1, lseek(fd, (off_t)-2 * sizeof(LINE1), SEEK_END));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* test lseek() with invalid file descriptor */
TEST(unistd_file, file_lseek_ebadf)
{
	assert_write(fd, LINE1);
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	TEST_ASSERT_EQUAL_INT(-1, lseek(fd, 1, SEEK_SET));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	TEST_ASSERT_EQUAL_INT(-1, lseek(fd, 1, SEEK_CUR));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	TEST_ASSERT_EQUAL_INT(-1, lseek(fd, 1, SEEK_END));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	/* this test closes fd by itself, invalidate it */
	fd = -1;
}


/* test lseek() on pipe */
TEST(unistd_file, file_lseek_espipe)
{
	int p[2];

	if (pipe(p) != 0) {
		/* ignore test if not pipe() not supported (errno == ENOSYS) */
		TEST_ASSERT_EQUAL_INT(ENOSYS, errno);
		TEST_IGNORE();
	}
	else {
		TEST_ASSERT_EQUAL_INT(-1, lseek(p[0], 1, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

		TEST_ASSERT_EQUAL_INT(-1, lseek(p[0], 1, SEEK_CUR));
		TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

		TEST_ASSERT_EQUAL_INT(-1, lseek(p[0], 1, SEEK_END));
		TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

		close(p[0]);
		close(p[1]);
	}
}


/* test truncate()-ing a file to smaller size than it already is */
TEST(unistd_file, file_truncate_down)
{
	struct stat st;

	/* write LINE1, then LINE2 to the file */
	assert_write(fd, LINE1);
	assert_write(fd, LINE2);
	/* assert file size with fstat */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &st));
	TEST_ASSERT_EQUAL_INT(sizeof(LINE1) + sizeof(LINE2), st.st_size);
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	/* truncate file length to sizeof(LINE1) */
	TEST_ASSERT_EQUAL_INT(0, truncate(FNAME, sizeof(LINE1)));

	/* reopen file and assert data length of sizeof(LINE1) by trying to read too much */
	fd = open(FNAME, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	assert_read_more(fd, buf, sizeof(LINE1));
	TEST_ASSERT_EQUAL_STRING(LINE1, buf);

	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
	/* assert new file size with fstat */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &st));
	TEST_ASSERT_EQUAL_INT(sizeof(LINE1), st.st_size);
}


/* test truncate()-ing a file to larger size than it already is */
TEST(unistd_file, file_truncate_up)
{
	struct stat st;
	char testbuf[sizeof(buf)];
	int datalen = 2 * sizeof(LINE1);

	/* alter buf with arbitrary data, because '\0' are going to be stored */
	memset(buf, ' ', sizeof(buf));
	/* write expected data to testbuf to be compared with buf */
	memset(testbuf, ' ', sizeof(testbuf));
	memset(testbuf, '\0', datalen);
	memcpy(testbuf, LINE1, sizeof(LINE1));

	/* write LINE1 to the file */
	assert_write(fd, LINE1);
	/* assert file size with fstat */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &st));
	TEST_ASSERT_EQUAL_INT(sizeof(LINE1), st.st_size);
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	/* truncate file length to higher than it already is */
	TEST_ASSERT_EQUAL_INT(0, truncate(FNAME, datalen));

	/* reopen file and read everything that's inside */
	fd = open(FNAME, O_RDONLY, S_IRUSR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	assert_read(fd, buf, datalen);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testbuf, buf, sizeof(testbuf));
	TEST_ASSERT_EQUAL_STRING(LINE1, buf);
	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
	/* assert new file size with fstat */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &st));
	TEST_ASSERT_EQUAL_INT(datalen, st.st_size);
}

/* truncate()-ing down file with opened descriptor so that it is in not truncated part */
TEST(unistd_file, file_truncate_opened)
{
	/* write LINE1, then LINE2 and then LINE3 to the file  with fd */
	assert_write(fd, LINE1);
	assert_write(fd, LINE2);
	assert_write(fd, LINE3);
	/* using lseek go back with fd to the end of LINE1  */
	TEST_ASSERT_EQUAL_INT(sizeof(LINE1), lseek(fd, (off_t)(sizeof(LINE1)), SEEK_SET));

	/* truncate file to exclude LINE3 */
	TEST_ASSERT_EQUAL_INT(0, truncate(FNAME, sizeof(LINE1) + sizeof(LINE2)));

	/* try to read from fd, should read only LINE2 */
	assert_read(fd, buf, sizeof(LINE2));
	TEST_ASSERT_EQUAL_STRING(LINE2, buf);

	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
}


/* truncate()-ing down file with opened descriptor so that it in truncated part */
TEST(unistd_file, file_truncate_opened_eof)
{
	/* write LINE1, then LINE2 to the file with fd */
	assert_write(fd, LINE1);
	assert_write(fd, LINE2);

	/* truncate file to exclude LINE2 */
	TEST_ASSERT_EQUAL_INT(0, truncate(FNAME, sizeof(LINE1)));

	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
}

/* truncate()-ing file to negative length*/
TEST(unistd_file, file_truncate_einval)
{
	TEST_ASSERT_EQUAL_INT(-1, truncate(FNAME, -1));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}

/* truncate()-ing with invalid or nonexistent filepath */
TEST(unistd_file, file_truncate_enoent)
{
	TEST_ASSERT_EQUAL_INT(-1, truncate("", 0));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	TEST_ASSERT_EQUAL_INT(-1, truncate("nonexistent_file", 0));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}

/* truncate()-ing on directory */
TEST(unistd_file, file_truncate_eisdir)
{
	/* <posix incmpliance> truncate() wrong errno returned

	truncate() called on directory returns errno 22 (EINVAL) - ext2 or errno 13 (EACCESS)
	instead of errno 21 (EISDIR)

	Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/573
	*/
#ifdef __phoenix__
	TEST_IGNORE();
#endif

	TEST_ASSERT_EQUAL_INT(-1, truncate("/dev", 0));
	TEST_ASSERT_EQUAL_INT(EISDIR, errno);
}

/* test ftruncate()-ing a file to smaller size than it already is */
TEST(unistd_file, file_ftruncate_down)
{
	struct stat st;

	/* write LINE1, then LINE2 to the file */
	assert_write(fd, LINE1);
	assert_write(fd, LINE2);
	/* assert file size with fstat */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &st));
	TEST_ASSERT_EQUAL_INT(sizeof(LINE1) + sizeof(LINE2), st.st_size);

	/* truncate file length to sizeof(LINE1) */
	TEST_ASSERT_EQUAL_INT(0, ftruncate(fd, sizeof(LINE1)));

	/* reopen file */
	TEST_ASSERT_EQUAL_INT(0, close(fd));
	fd = open(FNAME, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	/* assert data length of sizeof(LINE1) by trying to read too much */
	assert_read_more(fd, buf, sizeof(LINE1));
	TEST_ASSERT_EQUAL_STRING(LINE1, buf);
	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
	/* assert new file size with fstat */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &st));
	TEST_ASSERT_EQUAL_INT(sizeof(LINE1), st.st_size);
}

/* test ftruncate()-ing a file to larger size than it already is */
TEST(unistd_file, file_ftruncate_up)
{
	struct stat st;
	char testbuf[sizeof(buf)];
	int datalen = 2 * sizeof(LINE1);

	/* alter buf with arbitrary data, because '\0' are going to be stored */
	memset(buf, ' ', sizeof(buf));
	/* write expected data to testbuf to be compared with buf */
	memset(testbuf, ' ', sizeof(testbuf));
	memset(testbuf, '\0', datalen);
	memcpy(testbuf, LINE1, sizeof(LINE1));

	/* write LINE1 to the file */
	assert_write(fd, LINE1);
	/* assert file size with fstat */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &st));
	TEST_ASSERT_EQUAL_INT(sizeof(LINE1), st.st_size);

	/* truncate file length to higher than it already is */
	TEST_ASSERT_EQUAL_INT(0, ftruncate(fd, datalen));
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	/* reopen file */
	fd = open(FNAME, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	/* read everything that`s inside the file */
	assert_read(fd, buf, datalen);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testbuf, buf, sizeof(testbuf));
	TEST_ASSERT_EQUAL_STRING(LINE1, buf);
	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
	/* assert new file size with fstat */
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &st));
	TEST_ASSERT_EQUAL_INT(datalen, st.st_size);
}

/* ftruncate()-ing down file with opened descriptor so that it is in not truncated part */
TEST(unistd_file, file_ftruncate_opened)
{
	/* write LINE1, then LINE2 and then LINE3 to the file with fd */
	assert_write(fd, LINE1);
	assert_write(fd, LINE2);
	assert_write(fd, LINE3);
	/* using lseek go back with fd to the end of LINE1  */
	TEST_ASSERT_EQUAL_INT(sizeof(LINE1), lseek(fd, (off_t)(sizeof(LINE1)), SEEK_SET));

	/* truncate file to exclude LINE3 */
	TEST_ASSERT_EQUAL_INT(0, ftruncate(fd, sizeof(LINE1) + sizeof(LINE2)));

	/* try to read from fd, should read only LINE2 */
	assert_read(fd, buf, sizeof(LINE2));
	TEST_ASSERT_EQUAL_STRING(LINE2, buf);

	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
}

/* ftruncate()-ing down file with opened descriptor so that it is in truncated part */
TEST(unistd_file, file_ftruncate_opened_eof)
{
	/* write LINE1, then LINE2 to the file  with fd */
	assert_write(fd, LINE1);
	assert_write(fd, LINE2);

	/* truncate file to exclude LINE2 leacing fd outside the file */
	TEST_ASSERT_EQUAL_INT(0, ftruncate(fd, sizeof(LINE1)));

	/* assert EOF */
	TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(LINE1)));
	TEST_ASSERT_EMPTY(buf);
}

/* ftruncate()-ing file to negative length */
TEST(unistd_file, file_ftruncate_einval)
{
	TEST_ASSERT_EQUAL_INT(-1, ftruncate(fd, -1));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}

/* ftruncate()-ing file with invalid file descriptors */
TEST(unistd_file, file_ftruncate_ebadf)
{
	/* try to truncate on closed descriptor */
	TEST_ASSERT_EQUAL_INT(0, close(fd));
	TEST_ASSERT_EQUAL_INT(-1, ftruncate(fd, 0));
	TEST_ASSERT(errno == EBADF || errno == EINVAL);

	/* try to truncate on readonly file */
	fd = open(FNAME, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
	TEST_ASSERT_EQUAL_INT(-1, ftruncate(fd, 0));
	TEST_ASSERT(errno == EBADF || errno == EINVAL);
}

/* ftruncate()-ing directory */
TEST(unistd_file, file_ftruncate_eisdir)
{
	TEST_ASSERT_EQUAL_INT(-1, truncate("bin", 0));
	TEST_ASSERT_EQUAL_INT(EISDIR, errno);
}

/* test if descriptor from dup() refers to the same file */
TEST(unistd_file, file_dup)
{
	int fd2;

	fd2 = dup(fd);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd2);
	{
		assert_write(fd, LINE1);
		TEST_ASSERT_EQUAL_INT(0, lseek(fd2, 0, SEEK_SET));
		assert_read(fd, buf, sizeof(LINE1));

		TEST_ASSERT_EQUAL_STRING(LINE1, buf);
	}
	close(fd2);
}

/* test for invalid descriptors used in dup() */
TEST(unistd_file, file_dup_closed)
{
	int fd2 = 0;

	TEST_ASSERT_EQUAL_INT(-1, dup2(fd, -1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	close(fd);
	TEST_ASSERT_EQUAL_INT(-1, dup2(fd, fd2));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	/* this test closes fd by itself, invalidate it */
	fd = -1;
}

/* test if descriptor from dup2() refers to the same file */
TEST(unistd_file, file_dup2)
{
	/* some value different than 0, 1, 2 (stdin, stdout, stderr) */
	int fd2 = 7;

	fd2 = assert_free_fd(fd2);

	fd2 = dup2(fd, fd2);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd2);
	{
		assert_write(fd, LINE1);
		TEST_ASSERT_EQUAL_INT(0, lseek(fd2, 0, SEEK_SET));
		assert_read(fd, buf, sizeof(LINE1));
		TEST_ASSERT_EQUAL_STRING(LINE1, buf);
	}
	close(fd2);
}

/* dup2() on file descriptor that refers to file opened by another descriptor */
TEST(unistd_file, file_dup2_opened)
{
	const char filename2[] = "unistd_dup_file";
	int fd2;
	/* some value different than 0, 1, 2 (stdin, stdout, stderr) */
	int fdr = 7;

	fdr = assert_free_fd(fdr);

	fd2 = open(filename2, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd2);
	{
		/* fdr shall reference the same file as fd2 */
		fdr = dup2(fd2, fdr);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fdr);
		/* change fd2 to reference the same file as fd leaving fdr on the other file*/
		fd2 = dup2(fd, fd2);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd2);

		/* write different data to both fdr and fd2, and read it back */
		assert_write(fdr, LINE2);
		assert_write(fd2, LINE1);
		TEST_ASSERT_EQUAL_INT(0, lseek(fd2, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(0, lseek(fdr, 0, SEEK_SET));
		assert_read(fd2, buf, sizeof(LINE1));
		TEST_ASSERT_EQUAL_STRING(LINE1, buf);
		assert_read(fdr, buf, sizeof(LINE2));
		TEST_ASSERT_EQUAL_STRING(LINE2, buf);
	}
	TEST_ASSERT_EQUAL_INT(0, close(fd2));
	TEST_ASSERT_EQUAL_INT(0, close(fdr));
	TEST_ASSERT_EQUAL_INT(0, remove(filename2));
}

/* test for not valid descriptors used in dup2() */
TEST(unistd_file, file_dup2_closed)
{
	/* some value different than 0, 1, 2 (stdin, stdout, stderr) */
	int fd2 = 7;

	fd2 = assert_free_fd(fd2);

	TEST_ASSERT_EQUAL_INT(-1, dup2(fd, -1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	close(fd);
	TEST_ASSERT_EQUAL_INT(-1, dup2(fd, fd2));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	/* this test closes fd by itself, invalidate it */
	fd = -1;
}


TEST_GROUP_RUNNER(unistd_file)
{
	RUN_TEST_CASE(unistd_file, file_close);

	RUN_TEST_CASE(unistd_file, file_readwrite_nbytes);
	RUN_TEST_CASE(unistd_file, file_write_zero);
	RUN_TEST_CASE(unistd_file, file_write_reopened);
	RUN_TEST_CASE(unistd_file, file_write_dup);
	RUN_TEST_CASE(unistd_file, file_readwrite_badfd);
	RUN_TEST_CASE(unistd_file, file_write_incrlength);

	RUN_TEST_CASE(unistd_file, file_write_readonly);
	RUN_TEST_CASE(unistd_file, file_readwrite_pipe);

	RUN_TEST_CASE(unistd_file, file_lseek);
	RUN_TEST_CASE(unistd_file, file_lseek_pastfile);
	RUN_TEST_CASE(unistd_file, file_lseek_negative);
	RUN_TEST_CASE(unistd_file, file_lseek_ebadf);
	RUN_TEST_CASE(unistd_file, file_lseek_espipe);

	RUN_TEST_CASE(unistd_file, file_truncate_down);
	RUN_TEST_CASE(unistd_file, file_truncate_up);
	RUN_TEST_CASE(unistd_file, file_truncate_opened);
	RUN_TEST_CASE(unistd_file, file_truncate_opened_eof);
	RUN_TEST_CASE(unistd_file, file_truncate_einval);
	RUN_TEST_CASE(unistd_file, file_truncate_eisdir);
	RUN_TEST_CASE(unistd_file, file_truncate_enoent);

	RUN_TEST_CASE(unistd_file, file_ftruncate_down);
	RUN_TEST_CASE(unistd_file, file_ftruncate_up);
	RUN_TEST_CASE(unistd_file, file_ftruncate_opened);
	RUN_TEST_CASE(unistd_file, file_ftruncate_opened_eof);
	RUN_TEST_CASE(unistd_file, file_ftruncate_einval);
	RUN_TEST_CASE(unistd_file, file_ftruncate_ebadf);

	RUN_TEST_CASE(unistd_file, file_dup);
	RUN_TEST_CASE(unistd_file, file_dup_closed);
	RUN_TEST_CASE(unistd_file, file_dup2);
	RUN_TEST_CASE(unistd_file, file_dup2_opened);
	RUN_TEST_CASE(unistd_file, file_dup2_closed);
}
