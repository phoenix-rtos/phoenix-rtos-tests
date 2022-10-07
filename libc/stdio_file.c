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
 * Copyright 2021 Phoenix Systems
 * Author: Mateusz Niewiadomski
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


static FILE *fd, *fd2;
static char stdpath[] = "stdio_file_test";
static char c, buf[20];
static int filedes;
static char toolongpath[PATH_MAX + 16];

/* 
Tets group for:
fopen, fclose,
fdopen, freopen
*/
TEST_GROUP(stdio_fopenfclose);

TEST_SETUP(stdio_fopenfclose)
{
	fd = NULL;
	fd2 = NULL;
}


TEST_TEAR_DOWN(stdio_fopenfclose)
{ /* empty */
}


int checkdescriptor(FILE *fd)
{
	return (fd == NULL) ? 0 : !fclose(fd);
}


void assert_fopen_error(char *path, char *opts, int errnocode)
{
	fd = fopen(path, opts);
	TEST_ASSERT_FALSE(checkdescriptor(fd));
	TEST_ASSERT_EQUAL_INT(errnocode, errno);
}


void assert_fopen_success(char *path, char *opts)
{
	fd = fopen(path, opts);
	TEST_ASSERT_TRUE(checkdescriptor(fd));
}


TEST(stdio_fopenfclose, stdio_fopenfclose_file)
{
	/* not existing file opening without creating */
	assert_fopen_error(stdpath, "r", ENOENT);
	assert_fopen_error(stdpath, "r+", ENOENT);
	/* opening file with creation */
	assert_fopen_success(stdpath, "w");
	assert_fopen_success(stdpath, "a");
	assert_fopen_success(stdpath, "w+");
	assert_fopen_success(stdpath, "a+");
	/* opening existing file for read */
	assert_fopen_success(stdpath, "r");
	assert_fopen_success(stdpath, "r+");
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
	assert_fopen_error("", "r", ENOENT);
	assert_fopen_error(NULL, "r", EINVAL);
	assert_fopen_error("", "w", ENOENT);
	assert_fopen_error(NULL, "w", EINVAL);
}

TEST(stdio_fopenfclose, stdio_fopenfclose_wrongflags)
{
	/* open with no flags/wrong flags/null flags */
	assert_fopen_error(stdpath, "", EINVAL);
	assert_fopen_error(stdpath, "phoenix-rtos", EINVAL);
	// FIXME: invalid test, function argument defined as nonnull
	//assert_fopen_error(stdpath, NULL, EINVAL);
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
	fd = fopen(stdpath, "w");
	TEST_ASSERT_NOT_NULL(fd);
	/* freopen() on opened file */
	fd2 = freopen(stdpath, "w", fd);
	{
		TEST_ASSERT_NOT_NULL(fd);
		TEST_ASSERT_NOT_NULL(fd2);
		TEST_ASSERT_TRUE(fd == fd2);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_fopenfclose, fdopen_file)
{
	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		filedes = -1;
		TEST_ASSERT_NOT_NULL(fd);
		filedes = fileno(fd);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, filedes);
		fd2 = fdopen(filedes, "r");
		TEST_ASSERT_NOT_NULL(fd2);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
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
{ /* empty */
}


TEST_TEAR_DOWN(stdio_getput)
{ /* empty */
}


TEST(stdio_getput, fwritefread_basic)
{
	/* write some data to file using fwrite(), read it using fread(), assert end of file */
	memset(buf, 0, 20);
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_EQUAL_INT(5, fwrite(stdpath, sizeof(char), 5, fd));
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(5, fread(buf, sizeof(char), 5, fd));
		TEST_ASSERT_EQUAL_CHAR_ARRAY(stdpath, buf, 5);
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_getput, getput_basic)
{
	/* Correct write */
	fd = fopen(stdpath, "w");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_EQUAL_INT('a', fputc('a', fd));
		TEST_ASSERT_EQUAL_INT('b', putc('b', fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));

	/* Correct read */
	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_EQUAL_INT('a', fgetc(fd));
		TEST_ASSERT_EQUAL_INT('b', getc(fd));
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
		TEST_ASSERT_EQUAL_INT(EOF, getc(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));

	/* read from file open for writing */
	fd = fopen(stdpath, "w");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_EQUAL_INT('a', fputc('a', fd));
		TEST_ASSERT_EQUAL_INT('b', fputc('b', fd));
		rewind(fd);

		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));

	/* Try to write to file open for reading */
	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_EQUAL_INT(EOF, fputc('a', fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_EQUAL_INT(EOF, fputc('a', fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_getput, getsputs_basic)
{
	/* reading/writing from file */
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		rewind(fd);
		TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));

	/* reading from file not opened for reading */
	fd = fopen(stdpath, "w");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		TEST_ASSERT_NULL(fgets(buf, sizeof(buf), fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_getput, getsputs_readonly)
{
	/* 
	Upon successful completion, fputc() shall return the value it has written. 
	Otherwise, it shall return EOF, the error indicator for the stream shall be set 
	and errno shall be set to indicate the error. 
	
	https://github.com/phoenix-rtos/phoenix-rtos-project/issues/260
	*/
	TEST_IGNORE();

	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_EQUAL_INT(EOF, fputs(stdpath, fd)); /* <posix incompliance> returns 0, should EOF */
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), fd));
		TEST_ASSERT_EQUAL_CHAR_ARRAY(stdpath, buf, sizeof(stdpath));
		TEST_ASSERT_NULL(fgets(buf, sizeof(buf), fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_getput, ungetc_basic)
{
	/* standard usage of ungetc */
	fd = fopen(stdpath, "w");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));

	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		c = fgetc(fd);
		TEST_ASSERT_EQUAL_INT((int)c, ungetc(c, fd));
		TEST_ASSERT_EQUAL_PTR(buf, fgets(buf, sizeof(stdpath), fd));
		TEST_ASSERT_EQUAL_STRING(stdpath, buf);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));

	/*	EOF pushback test
		If the value of c equals that of the macro EOF, 
		the operation shall fail and the input stream shall be left unchanged. 
	*/
	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_EQUAL_INT(EOF, ungetc(EOF, fd));
		TEST_ASSERT_EQUAL_INT(stdpath[0], fgetc(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST_GROUP_RUNNER(stdio_getput)
{
	RUN_TEST_CASE(stdio_getput, fwritefread_basic);
	RUN_TEST_CASE(stdio_getput, getput_basic);
	RUN_TEST_CASE(stdio_getput, getsputs_basic);
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
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		fputs(LINE1, fd);
		fputs(LINE2, fd);
		fputs(LINE3, fd);
		fputs(LINE4, fd);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST_TEAR_DOWN(stdio_line)
{ /* empty */
}


TEST(stdio_line, getline_basic)
{
	char *line = NULL;
	size_t len = 1;

	/* read using getline */
	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		/* gerline with null buffer and misleading size */
		TEST_ASSERT_EQUAL_INT(sizeof(LINE1) - 1, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_STRING(LINE1, line);
		/* new buffer shall be allocated of size at least strlen+1 */
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE1), len);

		/* getline with to small buffer */
		TEST_ASSERT_EQUAL_INT(sizeof(LINE2) - 1, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_STRING(LINE2, line);
		/* buffer shall be reallocated of size at least strlen+1 */
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE2), len);

		/* getline with adequate buffer and character */
		TEST_ASSERT_EQUAL_INT(sizeof(LINE3) - 1, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_STRING(LINE3, line);
		/* buffer shall not be reallocated, and stay at size at least as big as previusly*/
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE3), len);

		/* getline with adequate buffer, but only newline is read */
		TEST_ASSERT_EQUAL_INT(sizeof(LINE4) - 1, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_STRING(LINE4, line);
		/* buffer shall not be reallocated, and stay at size at least as big as previusly*/
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE4), len);

		/* getline reading EOF */
		TEST_ASSERT_EQUAL_INT(-1, getline(&line, &len, fd));
		/* buffer shall not change from previous call */
		TEST_ASSERT_EQUAL_STRING(LINE4, line);
		/* buffer shall not be reallocated, and stay at size at least as big as previusly*/
		TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(LINE4), len);

		free(line);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_line, getline_wronly)
{
	char *line = NULL;
	size_t len = 0;

	/* read using getline from write-only file */
	fd = fopen(stdpath, "a");
	TEST_ASSERT_NOT_NULL(fd);
	{
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(-1, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_NULL(line);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_line, getline_allocated)
{
	char *line = NULL;
	size_t len = 50; /* allocated memory exceeds one demanded for a line that to be read */

	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		line = malloc(len);
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(6, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_INT(50, len);
		TEST_ASSERT_EQUAL_STRING("line1\n", line);
		free(line);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_line, getline_longline)
{
	char *line = NULL;
	size_t len = 0; /* getline() shall be responsible for memory allocation */
	int i;

	/* prepare file with one long line of length 1000 + '\n' */
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		for (i = 0; i < 100; i++) {
			fputs("0123456789", fd);
		}
		fputc('\n', fd);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));

	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(1001, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_INT(1002, len);
		TEST_ASSERT_EQUAL_INT(1001, strlen(line));
		free(line);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
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
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST_TEAR_DOWN(stdio_fileseek)
{ /* empty */
}


TEST(stdio_fileseek, seek_fseek)
{
	/* fseek() to SEEK_SET/CUR/END macros */
	fd = fopen(stdpath, "a+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_NOT_NULL(fd);
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
		/* fallback to absolute beginning */
		TEST_ASSERT_EQUAL_INT(0, fseek(fd, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(stdpath[0], fgetc(fd));
		/* fallback relative -1 */
		TEST_ASSERT_EQUAL_INT(stdpath[1], fgetc(fd));
		TEST_ASSERT_EQUAL_INT(0, fseek(fd, -1, SEEK_CUR));
		TEST_ASSERT_EQUAL_INT(stdpath[1], fgetc(fd));
		/* fallback to end */
		TEST_ASSERT_EQUAL_INT(0, fseek(fd, -1, SEEK_END));
		TEST_ASSERT_EQUAL_INT(stdpath[sizeof(stdpath) - 2], fgetc(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_fileseek, seek_fseeko)
{
	/* fseeko() to SEEK_SET/CUR/END macros */
	fd = fopen(stdpath, "a+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		/* fallback to absolute beginning */
		TEST_ASSERT_NOT_NULL(fd);
		TEST_ASSERT_EQUAL_INT(0, fseeko(fd, (off_t)0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(stdpath[0], fgetc(fd));
		/* fallback relative -1 */
		TEST_ASSERT_EQUAL_INT(stdpath[1], fgetc(fd));
		TEST_ASSERT_EQUAL_INT(0, (off_t)fseeko(fd, -1, SEEK_CUR));
		TEST_ASSERT_EQUAL_INT(stdpath[1], fgetc(fd));
		/* fallback to end */
		TEST_ASSERT_EQUAL_INT(0, (off_t)fseeko(fd, -1, SEEK_END));
		TEST_ASSERT_EQUAL_INT(stdpath[sizeof(stdpath) - 2], fgetc(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_fileseek, seek_fsetpos)
{
	/* <posix incmpliance> function is not implemented in libphoenix */
	/* The fsetpos() function shall set the file position and state indicators for the stream 
	pointed to by stream according to the value of the object pointed to by pos, 
	which the application shall ensure is a value obtained from an earlier call to fgetpos() on the same stream. 
	If a read or write error occurs, the error indicator for the stream shall be set and fsetpos() fails.*/
	TEST_IGNORE();
}


TEST(stdio_fileseek, seek_readonly)
{
	/* EBADF The file descriptor underlying the stream file is not open for writing ... */
	/* <posix incompliant> returns 0, should EOF */
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/263 */
	TEST_IGNORE();

	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_EQUAL_INT(EOF, fseek(fd, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_EQUAL_INT(EOF, fseeko(fd, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_fileseek, seek_rewind)
{
	/* Rewind to beginning of the file */
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(stdpath[0], fgetc(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_fileseek, seek_ftell)
{
	/* tell position in file after fseek() calls */
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		fseek(fd, 0, SEEK_SET);
		TEST_ASSERT_EQUAL_INT(0, ftell(fd));
		fseek(fd, 4, SEEK_SET);
		TEST_ASSERT_EQUAL_INT(4, ftell(fd));
		fgetc(fd);
		TEST_ASSERT_EQUAL_INT(5, ftell(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
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
{ /* empty */
}


TEST_TEAR_DOWN(stdio_fileop)
{ /* empty */
}


TEST(stdio_fileop, fileop_fileno)
{
	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fileno(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_fileop, fileop_feof)
{
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
		TEST_ASSERT_NOT_EQUAL_INT(0, feof(fd));
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(0, feof(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_fileop, fileop_remove)
{
	/* fopen() a file and remove() it */
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
	remove(stdpath);
	fd = fopen(stdpath, "r");
	TEST_ASSERT_NULL(fd);

	/* mkdir() a directory and remove() it */
	TEST_ASSERT_EQUAL_INT(0, mkdir("stdio_file_testdir", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
	TEST_ASSERT_EQUAL_INT(0, access("stdio_file_testdir", F_OK));
	TEST_ASSERT_EQUAL_INT(0, remove("stdio_file_testdir"));
}


TEST(stdio_fileop, fileop_ferror)
{
	fd = fopen(stdpath, "w");
	TEST_ASSERT_NOT_NULL(fd);
	{
		c = fgetc(fd);
		TEST_ASSERT_GREATER_THAN_INT(0, ferror(fd));
		clearerr(fd);
		TEST_ASSERT_EQUAL_INT(0, ferror(fd));
	}
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST_GROUP_RUNNER(stdio_fileop)
{
	RUN_TEST_CASE(stdio_fileop, fileop_fileno);
	RUN_TEST_CASE(stdio_fileop, fileop_feof);
	RUN_TEST_CASE(stdio_fileop, fileop_ferror);
}


/*
Test group for:
setvbuf, setbuf, fflush()
*/
TEST_GROUP(stdio_bufs);

TEST_SETUP(stdio_bufs)
{
	fd = fopen(stdpath, "w+");
	fd2 = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	TEST_ASSERT_NOT_NULL(fd2);
}


TEST_TEAR_DOWN(stdio_bufs)
{
	TEST_ASSERT_EQUAL_INT(0, fclose(fd2));
	TEST_ASSERT_EQUAL_INT(0, fclose(fd));
}


TEST(stdio_bufs, setbuf_basic)
{
	char buf2[BUFSIZ];

	/* after setbuf() read from file before and after flush */
	setbuf(fd, buf2);
	fputc('a', fd);
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd2));
	TEST_ASSERT_EQUAL_INT(0, fflush(fd));
	TEST_ASSERT_EQUAL_INT('a', fgetc(fd2));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd2));
}


TEST(stdio_bufs, setbuf_null)
{
	/* after setbuf() read from file before and after flush */
	setbuf(fd, NULL);
	fputc('a', fd);
	TEST_ASSERT_EQUAL_INT('a', fgetc(fd2));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd2));
}


TEST(stdio_bufs, setvbuf_fullbuffer)
{
	/* after setbuf() read from file before and after flush */
	char buf2[8];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(fd, buf2, _IOFBF, 8));

	TEST_ASSERT_GREATER_THAN_INT(0, fputc('a', fd));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd2));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd2));
	TEST_ASSERT_EQUAL_INT(0, fflush(fd));
	TEST_ASSERT_EQUAL_INT('a', fgetc(fd2));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd2));
}


TEST(stdio_bufs, setvbuf_fullbuffer_overflow)
{
	char buf2[8];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(fd, buf2, _IOFBF, 8));

	TEST_ASSERT_GREATER_THAN_INT(0, fputs("0123456789", fd));
	TEST_ASSERT_NOT_NULL(fgets(buf, 20, fd2));
	TEST_ASSERT_EQUAL_INT(10, strlen(buf));
}


TEST(stdio_bufs, setvbuf_linebuffer)
{
	char buf2[8];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(fd, buf2, _IOLBF, 8));

	TEST_ASSERT_GREATER_THAN_INT(0, fputs("0123", fd));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd2));
	TEST_ASSERT_GREATER_THAN_INT(0, fputc('\n', fd));
	TEST_ASSERT_NOT_NULL(fgets(buf, 10, fd2));
	TEST_ASSERT_EQUAL_INT(5, strlen(buf));
}


TEST(stdio_bufs, setvbuf_nobuffer)
{
	char buf2[8];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(fd, buf2, _IONBF, 8));

	TEST_ASSERT_GREATER_THAN_INT(0, fputs("0123", fd));
	TEST_ASSERT_NOT_NULL(fgets(buf, 10, fd2));
	TEST_ASSERT_EQUAL_INT(4, strlen(buf));
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
