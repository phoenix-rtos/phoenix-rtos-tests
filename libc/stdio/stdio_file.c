/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * libphoenix functions tests from stdio/file.c
 *
 * TESTED:
 * fopen, fclose, fdopen, freopen,
 * fwrite, fread,
 * putc, fputc, fputs,
 * getc, fgetc, fgets,
 * ungetc,
 * getline,
 * fseek, fseeko, rewind,
 * ftell,
 * fileno, feof, remove,
 * ferror, clearerr,
 * setvbuf, setbuf, fflush,
 *
 * UNTESTED:
 * puts, gets < needs writing to stdin/unimplemented
 * popen, pclose, tmpfile < not usable on all targets
 *
 * Copyright 2021, 2022 Phoenix Systems
 * Author: Mateusz Niewiadomski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <unity_fixture.h>


#define STDIO_TEST_FILENAME "stdio_file_test"
#define BUF_SIZE            20
#define BUF2_SIZE           8


/* these variables are global to close opened files in case of test failure */
static FILE *filep, *filep2;
static const char teststr[] = "test_string_123";
static char toolongpath[PATH_MAX + 16];

/*
Tets group for:
fopen, fclose,
fdopen, freopen
*/
TEST_GROUP(stdio_fopenfclose);

TEST_SETUP(stdio_fopenfclose)
{
	filep = NULL;
	filep2 = NULL;
}


TEST_TEAR_DOWN(stdio_fopenfclose)
{
	/* if the filep is NULL it means that file does not exist or is closed */
	if (filep != NULL) {
		fclose(filep);
	}

	if (filep2 != NULL) {
		fclose(filep2);
	}

	/* remove the testfile even if some tet cases failed */
	remove(STDIO_TEST_FILENAME);
}


static void assert_fopen_error(const char *path, const char *opts, int errnocode)
{
	FILE *filepLocal;

	filepLocal = fopen(path, opts);
	if (errnocode != -1) {
		TEST_ASSERT_EQUAL_INT(errnocode, errno);
	}
	TEST_ASSERT_NULL(filepLocal);
}


static void assert_fopen_success(const char *path, const char *opts)
{
	FILE *filepLocal;

	filepLocal = fopen(path, opts);
	TEST_ASSERT_NOT_NULL(filepLocal);
	TEST_ASSERT_EQUAL_INT(0, fclose(filepLocal));
}


TEST(stdio_fopenfclose, stdio_fopenfclose_file)
{
	/* not existing file opening without creating */
	assert_fopen_error(STDIO_TEST_FILENAME, "r", ENOENT);
	assert_fopen_error(STDIO_TEST_FILENAME, "r+", ENOENT);
	/* opening file with creation */
	assert_fopen_success(STDIO_TEST_FILENAME, "w");
	assert_fopen_success(STDIO_TEST_FILENAME, "a");
	assert_fopen_success(STDIO_TEST_FILENAME, "w+");
	assert_fopen_success(STDIO_TEST_FILENAME, "a+");
	/* opening existing file for read */
	assert_fopen_success(STDIO_TEST_FILENAME, "r");
	assert_fopen_success(STDIO_TEST_FILENAME, "r+");
}


TEST(stdio_fopenfclose, stdio_fopenfclose_opendir)
{
	/* open directory */
	assert_fopen_success("/dev/", "r");
	assert_fopen_error("/dev/", "w", EISDIR);
}

TEST(stdio_fopenfclose, stdio_fopenfclose_zeropath)
{
	/* open null or zero path */
	/* we do not check errno, because it's not standardized in POSIX for NULL path case.
	On Phoenix-RTOS it returns EINVAL, on Ubuntu host it's EFAULT */
	assert_fopen_error("", "r", ENOENT);
	assert_fopen_error(NULL, "r", -1);
	assert_fopen_error("", "w", ENOENT);
	assert_fopen_error(NULL, "w", -1);
}

TEST(stdio_fopenfclose, stdio_fopenfclose_wrongflags)
{
	/* open with no flags/wrong flags/null flags */
	assert_fopen_error(STDIO_TEST_FILENAME, "", EINVAL);
	assert_fopen_error(STDIO_TEST_FILENAME, "phoenix-rtos", EINVAL);
	// FIXME: invalid test, function argument defined as nonnull
	// assert_fopen_error(STDIO_TEST_FILENAME, NULL, EINVAL);
}

TEST(stdio_fopenfclose, stdio_fopenfclose_toolongname)
{
	/* open file with too long name */
	memset(toolongpath, 'a', sizeof(toolongpath));
	toolongpath[sizeof(toolongpath) - 1] = '\0';

	assert_fopen_error(toolongpath, "w", ENAMETOOLONG);
}


TEST(stdio_fopenfclose, freopen_file)
{
	int ret;
	filep = fopen(STDIO_TEST_FILENAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	/* freopen() on opened file */
	filep2 = freopen(STDIO_TEST_FILENAME, "w", filep);
	{
		TEST_ASSERT_NOT_NULL(filep);
		TEST_ASSERT_NOT_NULL(filep2);
		TEST_ASSERT_TRUE(filep == filep2);
	}
	ret = fclose(filep);
	filep = NULL;
	filep2 = NULL;

	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(stdio_fopenfclose, fdopen_file)
{
	int fd1, fd2, ret;

	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		fd1 = -1;
		TEST_ASSERT_NOT_NULL(filep);
		fd1 = fileno(filep);
		fd2 = dup(fd1);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd2);
		filep2 = fdopen(fd2, "r");
		TEST_ASSERT_NOT_NULL(filep2);
		ret = fclose(filep2);
		filep2 = NULL;
		TEST_ASSERT_EQUAL_INT(0, ret);
	}
	ret = fclose(filep);
	filep = NULL;

	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP_RUNNER(stdio_fopenfclose)
{
	RUN_TEST_CASE(stdio_fopenfclose, stdio_fopenfclose_file);
	RUN_TEST_CASE(stdio_fopenfclose, stdio_fopenfclose_opendir);
	RUN_TEST_CASE(stdio_fopenfclose, stdio_fopenfclose_zeropath);
	RUN_TEST_CASE(stdio_fopenfclose, stdio_fopenfclose_wrongflags);
	RUN_TEST_CASE(stdio_fopenfclose, stdio_fopenfclose_toolongname);
	RUN_TEST_CASE(stdio_fopenfclose, freopen_file);
	RUN_TEST_CASE(stdio_fopenfclose, fdopen_file)
}


/*
Tets group for:
- fwrite, fread
- putc, fputc,
- getc, fgetc,
- putchar, getchar, < need threads
- ungetc,
- puts, fputs, fgets
*/
TEST_GROUP(stdio_getput);

TEST_SETUP(stdio_getput)
{
	filep = NULL;
}


TEST_TEAR_DOWN(stdio_getput)
{
	if (filep != NULL) {
		fclose(filep);
	}

	/* remove the testfile even if some tet cases failed */
	remove(STDIO_TEST_FILENAME);
}


static void assert_fclosed(FILE **fp)
{
	int ret = fclose(*fp);
	*fp = NULL;
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(stdio_getput, fwritefread_basic)
{
	char buf[BUF_SIZE];

	/* write some data to file using fwrite(), read it using fread(), assert end of file */
	memset(buf, 0, 20);
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_EQUAL_INT(5, fwrite(teststr, sizeof(char), 5, filep));
		rewind(filep);
		TEST_ASSERT_EQUAL_INT(5, fread(buf, sizeof(char), 5, filep));
		TEST_ASSERT_EQUAL_CHAR_ARRAY(teststr, buf, 5);
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep));
	}
	assert_fclosed(&filep);
}


TEST(stdio_getput, getput_basic)
{
	/* Correct write */
	filep = fopen(STDIO_TEST_FILENAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_EQUAL_INT('a', fputc('a', filep));
		TEST_ASSERT_EQUAL_INT('b', putc('b', filep));
	}
	assert_fclosed(&filep);

	/* Correct read */
	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_EQUAL_INT('a', fgetc(filep));
		TEST_ASSERT_EQUAL_INT('b', getc(filep));
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep));
		TEST_ASSERT_EQUAL_INT(EOF, getc(filep));
	}
	assert_fclosed(&filep);

	/* read from file open for writing */
	filep = fopen(STDIO_TEST_FILENAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_EQUAL_INT('a', fputc('a', filep));
		TEST_ASSERT_EQUAL_INT('b', fputc('b', filep));
		rewind(filep);

		TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	assert_fclosed(&filep);

	/* Try to write to file open for reading */
	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_EQUAL_INT(EOF, fputc('a', filep));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_EQUAL_INT(EOF, fputc('a', filep));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	assert_fclosed(&filep);
}


TEST(stdio_getput, fgetc_eof)
{
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_EQUAL_INT(0, feof(filep));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep));
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(filep));

	TEST_ASSERT_EQUAL_INT('a', fputc('a', filep));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep));
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(filep));
}


TEST(stdio_getput, fgets_eof)
{
	char buf[BUF_SIZE];

	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_EQUAL_INT(0, feof(filep));
	TEST_ASSERT_NULL(fgets(buf, sizeof(buf), filep));
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(filep));

	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs("test_str", filep));
	TEST_ASSERT_NULL(fgets(buf, sizeof(buf), filep));
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(filep));
}


TEST(stdio_getput, getsputs_basic)
{
	char buf[BUF_SIZE];

	/* reading/writing from file */
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(teststr, filep));
		rewind(filep);
		TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), filep));
	}
	assert_fclosed(&filep);

	/* reading from file not opened for reading */
	filep = fopen(STDIO_TEST_FILENAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(teststr, filep));
		TEST_ASSERT_NULL(fgets(buf, sizeof(buf), filep));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	assert_fclosed(&filep);
}


IGNORE_TEST(stdio_getput, getsputs_readonly)
{
	/*
	Upon successful completion, fputc() shall return the value it has written.
	Otherwise, it shall return EOF, the error indicator for the stream shall be set
	and errno shall be set to indicate the error.

	https://github.com/phoenix-rtos/phoenix-rtos-project/issues/260
	*/

	char buf[BUF_SIZE];

	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_EQUAL_INT(EOF, fputs(teststr, filep)); /* <posix incompliance> returns 0, should EOF */
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), filep));
		TEST_ASSERT_EQUAL_CHAR_ARRAY(teststr, buf, sizeof(teststr));
		TEST_ASSERT_NULL(fgets(buf, sizeof(buf), filep));
	}
	assert_fclosed(&filep);
}


TEST(stdio_getput, ungetc_basic)
{
	char c, buf[BUF_SIZE];

	/* standard usage of ungetc */
	filep = fopen(STDIO_TEST_FILENAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(teststr, filep));
	}
	assert_fclosed(&filep);

	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	{
		c = fgetc(filep);
		TEST_ASSERT_EQUAL_INT((int)c, ungetc(c, filep));
		TEST_ASSERT_EQUAL_PTR(buf, fgets(buf, sizeof(teststr), filep));
		TEST_ASSERT_EQUAL_STRING(teststr, buf);
	}
	assert_fclosed(&filep);

	/*	EOF pushback test
		If the value of c equals that of the macro EOF,
		the operation shall fail and the input stream shall be left unchanged.
	*/
	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_EQUAL_INT(EOF, ungetc(EOF, filep));
		TEST_ASSERT_EQUAL_INT(teststr[0], fgetc(filep));
	}
	assert_fclosed(&filep);
}


TEST_GROUP_RUNNER(stdio_getput)
{
	RUN_TEST_CASE(stdio_getput, fwritefread_basic);
	RUN_TEST_CASE(stdio_getput, getput_basic);
	RUN_TEST_CASE(stdio_getput, fgetc_eof);
	RUN_TEST_CASE(stdio_getput, getsputs_basic);
	RUN_TEST_CASE(stdio_getput, fgets_eof);
	RUN_TEST_CASE(stdio_getput, getsputs_readonly);
	RUN_TEST_CASE(stdio_getput, ungetc_basic);
}


/*
test group for:
getline,
*/
#define LINE1 "line1\n"
#define LINE2 "lineline2\n"
#define LINE3 "line3\n"
#define LINE4 "\n"


TEST_GROUP(stdio_line);

TEST_SETUP(stdio_line)
{
	/* file preparation */
	filep = fopen(STDIO_TEST_FILENAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	{
		fputs(LINE1, filep);
		fputs(LINE2, filep);
		fputs(LINE3, filep);
		fputs(LINE4, filep);
	}
	assert_fclosed(&filep);
}


TEST_TEAR_DOWN(stdio_line)
{
	if (filep != NULL) {
		fclose(filep);
	}

	/* remove the testfile even if some tet cases failed */
	remove(STDIO_TEST_FILENAME);
}


TEST(stdio_line, getline_basic)
{
	char *line = NULL;
	size_t len = 1;

	/* read using getline */
	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	{
		/* gerline with null buffer and misleading size */
		TEST_ASSERT_EQUAL_INT(sizeof(LINE1) - 1, getline(&line, &len, filep));
		TEST_ASSERT_EQUAL_STRING(LINE1, line);
		/* new buffer shall be allocated of size at least strlen+1 */
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE1), len);

		/* getline with to small buffer */
		TEST_ASSERT_EQUAL_INT(sizeof(LINE2) - 1, getline(&line, &len, filep));
		TEST_ASSERT_EQUAL_STRING(LINE2, line);
		/* buffer shall be reallocated of size at least strlen+1 */
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE2), len);

		/* getline with adequate buffer and character */
		TEST_ASSERT_EQUAL_INT(sizeof(LINE3) - 1, getline(&line, &len, filep));
		TEST_ASSERT_EQUAL_STRING(LINE3, line);
		/* buffer shall not be reallocated, and stay at size at least as big as previusly*/
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE3), len);

		/* getline with adequate buffer, but only newline is read */
		TEST_ASSERT_EQUAL_INT(sizeof(LINE4) - 1, getline(&line, &len, filep));
		TEST_ASSERT_EQUAL_STRING(LINE4, line);
		/* buffer shall not be reallocated, and stay at size at least as big as previusly*/
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE4), len);

		/* getline reading EOF */
		TEST_ASSERT_EQUAL_INT(-1, getline(&line, &len, filep));
		/* buffer shall not change from previous call */
		TEST_ASSERT_EQUAL_STRING(LINE4, line);
		/* buffer shall not be reallocated, and stay at size at least as big as previusly*/
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE4), len);

		free(line);
	}
	assert_fclosed(&filep);
}


TEST(stdio_line, getline_wronly)
{
	char *line = NULL;
	size_t len = 0;

	/* read using getline from write-only file */
	filep = fopen(STDIO_TEST_FILENAME, "a");
	TEST_ASSERT_NOT_NULL(filep);
	{
		rewind(filep);
		TEST_ASSERT_EQUAL_INT(-1, getline(&line, &len, filep));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		/* even if line is a NULL pointer and there is nothing to read, it shall allocate even a byte for NUL termination char */
		TEST_ASSERT_NOT_NULL(line);
		free(line);
	}
	assert_fclosed(&filep);
}


TEST(stdio_line, getline_allocated)
{
	char *line = NULL;
	size_t len = 50; /* allocated memory exceeds one demanded for a line that to be read */

	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	{
		line = malloc(len);
		TEST_ASSERT_NOT_NULL(line);
		rewind(filep);
		TEST_ASSERT_EQUAL_INT(6, getline(&line, &len, filep));
		TEST_ASSERT_EQUAL_INT(50, len);
		TEST_ASSERT_EQUAL_STRING("line1\n", line);
		free(line);
	}
	assert_fclosed(&filep);
}


TEST(stdio_line, getline_longline)
{
	char *line = NULL;
	size_t len = 0; /* getline() shall be responsible for memory allocation */
	int i;

	/* prepare file with one long line of length 1000 + '\n' */
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		for (i = 0; i < 100; i++) {
			fputs("0123456789", filep);
		}
		fputc('\n', filep);
	}
	assert_fclosed(&filep);

	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	{
		rewind(filep);
		TEST_ASSERT_EQUAL_INT(1001, getline(&line, &len, filep));
		/* the len can be set to a bigger value than it's required */
		TEST_ASSERT_GREATER_THAN_INT(1001, len);
		TEST_ASSERT_EQUAL_INT(1001, strlen(line));
		free(line);
	}
	assert_fclosed(&filep);
}


TEST_GROUP_RUNNER(stdio_line)
{
	RUN_TEST_CASE(stdio_line, getline_basic);
	RUN_TEST_CASE(stdio_line, getline_wronly);
	RUN_TEST_CASE(stdio_line, getline_allocated);
	RUN_TEST_CASE(stdio_line, getline_longline)
}


/*
Test group for:
fseek, fseeko, fsetpos(), rewind()
ftell, ftello
*/
TEST_GROUP(stdio_fileseek);

TEST_SETUP(stdio_fileseek)
{
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(teststr, filep));
	}
	assert_fclosed(&filep);
}


TEST_TEAR_DOWN(stdio_fileseek)
{
	if (filep != NULL) {
		fclose(filep);
	}

	/* remove the testfile even if some tet cases failed */
	remove(STDIO_TEST_FILENAME);
}


TEST(stdio_fileseek, seek_fseek)
{
	/* fseek() to SEEK_SET/CUR/END macros */
	filep = fopen(STDIO_TEST_FILENAME, "a+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_NOT_NULL(filep);
		/* POSIX does not specify whether the indicator shall be changed, when we are only reading in append mode.
		On Ubuntu host it will remain at the beginning of a file, on Phoenix-RTOS it will indicate EOF
		That's why we want to write sth before testing fseek, this case is standardized - the indicator is set to EOF prior to each write*/
		TEST_ASSERT_EQUAL_INT('.', fputc('.', filep));
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep));
		/* fallback to absolute beginning */
		TEST_ASSERT_EQUAL_INT(0, fseek(filep, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(teststr[0], fgetc(filep));
		/* fallback relative -1 */
		TEST_ASSERT_EQUAL_INT(teststr[1], fgetc(filep));
		TEST_ASSERT_EQUAL_INT(0, fseek(filep, -1, SEEK_CUR));
		TEST_ASSERT_EQUAL_INT(teststr[1], fgetc(filep));
		/* fallback to end */
		TEST_ASSERT_EQUAL_INT(0, fseek(filep, -1, SEEK_END));
		TEST_ASSERT_EQUAL_INT('.', fgetc(filep));
	}
	assert_fclosed(&filep);
}


TEST(stdio_fileseek, seek_fseeko)
{
	/* fseeko() to SEEK_SET/CUR/END macros */
	filep = fopen(STDIO_TEST_FILENAME, "a+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		/* fallback to absolute beginning */
		TEST_ASSERT_NOT_NULL(filep);
		TEST_ASSERT_EQUAL_INT(0, fseeko(filep, (off_t)0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(teststr[0], fgetc(filep));
		/* fallback relative -1 */
		TEST_ASSERT_EQUAL_INT(teststr[1], fgetc(filep));
		TEST_ASSERT_EQUAL_INT(0, (off_t)fseeko(filep, -1, SEEK_CUR));
		TEST_ASSERT_EQUAL_INT(teststr[1], fgetc(filep));
		/* fallback to end */
		TEST_ASSERT_EQUAL_INT(0, (off_t)fseeko(filep, -1, SEEK_END));
		TEST_ASSERT_EQUAL_INT(teststr[sizeof(teststr) - 2], fgetc(filep));
	}
	assert_fclosed(&filep);
}


/* TODO: add more test cases to cover all requirements from documentation (errno, clearing EOF etc.) */
TEST(stdio_fileseek, seek_fsetpos)
{
	/* The fsetpos() function shall set the file position and state indicators for the stream
	pointed to by stream according to the value of the object pointed to by pos,
	which the application shall ensure is a value obtained from an earlier call to fgetpos() on the same stream.
	If a read or write error occurs, the error indicator for the stream shall be set and fsetpos() fails.*/

	fpos_t pos0, pos1;
	filep = fopen(STDIO_TEST_FILENAME, "a+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		/* Ensure to start from the beginning */
		(void)rewind(filep); /* not tested */
		/* Save location of the file beginning */
		TEST_ASSERT_EQUAL_INT(0, fgetpos(filep, &pos0));
		TEST_ASSERT_EQUAL_INT(teststr[0], fgetc(filep));
		/* Save location at second byte */
		TEST_ASSERT_EQUAL_INT(0, fgetpos(filep, &pos1));
		TEST_ASSERT_EQUAL_INT(teststr[1], fgetc(filep));
		TEST_ASSERT_EQUAL_INT(teststr[2], fgetc(filep));
		/* Restore location #1 */
		TEST_ASSERT_EQUAL_INT(0, fsetpos(filep, &pos1));
		TEST_ASSERT_EQUAL_INT(teststr[1], fgetc(filep));
		TEST_ASSERT_EQUAL_INT(teststr[2], fgetc(filep));
		/* Restore location #0 */
		TEST_ASSERT_EQUAL_INT(0, fsetpos(filep, &pos0));
		TEST_ASSERT_EQUAL_INT(teststr[0], fgetc(filep));
	}
	assert_fclosed(&filep);
}


IGNORE_TEST(stdio_fileseek, seek_readonly)
{
	/* EBADF The file descriptor underlying the stream file is not open for writing ... */
	/* <posix incompliant> returns 0, should EOF */
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/263 */

	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_EQUAL_INT(EOF, fseek(filep, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_EQUAL_INT(EOF, fseeko(filep, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	assert_fclosed(&filep);
}


TEST(stdio_fileseek, seek_rewind)
{
	/* Rewind to beginning of the file */
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(teststr, filep));
		rewind(filep);
		TEST_ASSERT_EQUAL_INT(teststr[0], fgetc(filep));
	}
	assert_fclosed(&filep);
}


TEST(stdio_fileseek, seek_ftell)
{
	/* tell position in file after fseek() calls */
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(teststr, filep));
		fseek(filep, 0, SEEK_SET);
		TEST_ASSERT_EQUAL_INT(0, ftell(filep));
		fseek(filep, 4, SEEK_SET);
		TEST_ASSERT_EQUAL_INT(4, ftell(filep));
		fgetc(filep);
		TEST_ASSERT_EQUAL_INT(5, ftell(filep));
	}
	assert_fclosed(&filep);
}


TEST_GROUP_RUNNER(stdio_fileseek)
{
	RUN_TEST_CASE(stdio_fileseek, seek_fseek);
	RUN_TEST_CASE(stdio_fileseek, seek_fseeko);
	RUN_TEST_CASE(stdio_fileseek, seek_fsetpos)
	RUN_TEST_CASE(stdio_fileseek, seek_readonly);
	RUN_TEST_CASE(stdio_fileseek, seek_rewind);
	RUN_TEST_CASE(stdio_fileseek, seek_ftell);
}


/*
Tets group for:
fileno, feof, remove
ferror, clearerr
*/
TEST_GROUP(stdio_fileop);

TEST_SETUP(stdio_fileop)
{
	filep = NULL;
}


TEST_TEAR_DOWN(stdio_fileop)
{
	if (filep != NULL) {
		fclose(filep);
		filep = NULL;
	}

	/* remove the testfile even if some tet cases failed */
	remove(STDIO_TEST_FILENAME);
}


TEST(stdio_fileop, fileop_fileno)
{
	filep = fopen(STDIO_TEST_FILENAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fileno(filep));
	}
	assert_fclosed(&filep);
}


TEST(stdio_fileop, fileop_feof)
{
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(teststr, filep));
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep));
		TEST_ASSERT_NOT_EQUAL_INT(0, feof(filep));
		rewind(filep);
		TEST_ASSERT_EQUAL_INT(0, feof(filep));
	}
	assert_fclosed(&filep);
}


TEST(stdio_fileop, fileop_remove)
{
	/* fopen() a file and remove() it */
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	TEST_ASSERT_EQUAL_INT(0, fclose(filep));
	remove(STDIO_TEST_FILENAME);
	filep = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NULL(filep);

	/* mkdir() a directory and remove() it */
	TEST_ASSERT_EQUAL_INT(0, mkdir("stdio_file_testdir", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
	TEST_ASSERT_EQUAL_INT(0, access("stdio_file_testdir", F_OK));
	TEST_ASSERT_EQUAL_INT(0, remove("stdio_file_testdir"));
}


TEST(stdio_fileop, fileop_ferror)
{
	filep = fopen(STDIO_TEST_FILENAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	{
		fgetc(filep);
		TEST_ASSERT_GREATER_THAN_INT(0, ferror(filep));
		clearerr(filep);
		TEST_ASSERT_EQUAL_INT(0, ferror(filep));
	}
	assert_fclosed(&filep);
}


TEST(stdio_fileop, fileop_clearerr)
{
	filep = fopen(STDIO_TEST_FILENAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	{
		fgetc(filep);
		TEST_ASSERT_NOT_EQUAL_INT(0, ferror(filep));
		clearerr(filep);
		TEST_ASSERT_EQUAL_INT(0, ferror(filep));
	}
	assert_fclosed(&filep);

	filep = fopen(STDIO_TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(filep);
	{
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep));
		TEST_ASSERT_NOT_EQUAL_INT(0, feof(filep));
		clearerr(filep);
		TEST_ASSERT_EQUAL_INT(0, feof(filep));
	}
	assert_fclosed(&filep);
}


TEST_GROUP_RUNNER(stdio_fileop)
{
	RUN_TEST_CASE(stdio_fileop, fileop_fileno);
	RUN_TEST_CASE(stdio_fileop, fileop_feof);
	RUN_TEST_CASE(stdio_fileop, fileop_ferror);
	RUN_TEST_CASE(stdio_fileop, fileop_clearerr);
}


/*
Test group for:
setvbuf, setbuf, fflush()
*/
TEST_GROUP(stdio_bufs);

TEST_SETUP(stdio_bufs)
{
	filep = fopen(STDIO_TEST_FILENAME, "w+");
	filep2 = fopen(STDIO_TEST_FILENAME, "r");
	TEST_ASSERT_NOT_NULL(filep);
	TEST_ASSERT_NOT_NULL(filep2);
}


TEST_TEAR_DOWN(stdio_bufs)
{
	if (filep2 != NULL) {
		fclose(filep2);
		filep2 = NULL;
	}
	if (filep != NULL) {
		fclose(filep);
		filep = NULL;
	}

	/* remove the testfile even if some tet cases failed */
	remove(STDIO_TEST_FILENAME);
}


TEST(stdio_bufs, setbuf_basic)
{
	char buf2[BUFSIZ];

	/* after setbuf() read from file before and after flush */
	setbuf(filep, buf2);
	fputc('a', filep);
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep2));
	/* clear the EOF indicator */
	clearerr(filep2);
	TEST_ASSERT_EQUAL_INT(0, fflush(filep));
	TEST_ASSERT_EQUAL_INT('a', fgetc(filep2));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep2));
}


TEST(stdio_bufs, setbuf_null)
{
	/* after setbuf() read from file before and after flush */
	setbuf(filep, NULL);
	fputc('a', filep);
	TEST_ASSERT_EQUAL_INT('a', fgetc(filep2));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep2));
}


TEST(stdio_bufs, setvbuf_fullbuffer)
{
	/* after setbuf() read from file before and after flush */
	char buf2[BUF2_SIZE];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(filep, buf2, _IOFBF, sizeof(buf2)));

	TEST_ASSERT_GREATER_THAN_INT(0, fputc('a', filep));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep2));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep2));
	/* clear the EOF indicator */
	clearerr(filep2);
	TEST_ASSERT_EQUAL_INT(0, fflush(filep));
	TEST_ASSERT_EQUAL_INT('a', fgetc(filep2));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(filep2));
}


TEST(stdio_bufs, setvbuf_fullbuffer_overflow)
{
	const char data[] = "0123456789";
	char buf[BUF_SIZE];
	char buf2[BUF2_SIZE];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(filep, buf2, _IOFBF, sizeof(buf2)));

	TEST_ASSERT_GREATER_THAN_INT(0, fputs(data, filep));
	TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), filep2));
	TEST_ASSERT_EQUAL_INT(strlen(data), strlen(buf));
}


TEST(stdio_bufs, setvbuf_linebuffer)
{
	const char data[] = "0123";
	char buf[BUF_SIZE];
	char buf2[BUF2_SIZE];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(filep, buf2, _IOLBF, sizeof(buf2)));

	TEST_ASSERT_GREATER_THAN_INT(0, fputs(data, filep));
	/* On host data can be already flushed before sending newline */
	TEST_ASSERT_GREATER_THAN_INT(0, fputc('\n', filep));
	TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), filep2));
	TEST_ASSERT_EQUAL_INT(strlen(data) + 1, strlen(buf));
}


TEST(stdio_bufs, setvbuf_nobuffer)
{
	const char data[] = "0123";
	char buf[BUF_SIZE];
	char buf2[BUF2_SIZE];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(filep, buf2, _IONBF, sizeof(buf2)));

	TEST_ASSERT_GREATER_THAN_INT(0, fputs(data, filep));
	TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), filep2));
	TEST_ASSERT_EQUAL_INT(strlen(data), strlen(buf));
}


TEST_GROUP_RUNNER(stdio_bufs)
{
	RUN_TEST_CASE(stdio_bufs, setbuf_basic);
	RUN_TEST_CASE(stdio_bufs, setbuf_null);
	RUN_TEST_CASE(stdio_bufs, setvbuf_fullbuffer);
	RUN_TEST_CASE(stdio_bufs, setvbuf_fullbuffer_overflow);
	RUN_TEST_CASE(stdio_bufs, setvbuf_linebuffer);
	RUN_TEST_CASE(stdio_bufs, setvbuf_nobuffer);
}
