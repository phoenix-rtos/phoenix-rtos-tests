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


FILE *fd, *fd2;
char stdpath[] = "stdio_file_test";
char c, buf[20];
int filedes;
char toolongpath[PATH_MAX + 16];

/* 
Tets group for:
fopen, fclose,
fdopen, freopen
*/
TEST_GROUP(fopenfclose);

TEST_SETUP(fopenfclose)
{
	memset(toolongpath, 'a', PATH_MAX + 15);
	toolongpath[PATH_MAX + 15] = '\0';
}


TEST_TEAR_DOWN(fopenfclose)
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


TEST(fopenfclose, fopenfclose_file)
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


TEST(fopenfclose, fopenfclose_specs)
{
	/* open directory */
	assert_fopen_success("/dev/", "r");
	assert_fopen_error("/dev/", "w", EISDIR);
	/* open null or zero path */
	assert_fopen_error("", "r", EINVAL);   /* <POSIX incompliant>, errno should be ENOENT */
	assert_fopen_error(NULL, "r", EINVAL); /* <POSIX incompliant>, errno should be ENOENT */
	assert_fopen_error("", "w", EINVAL);   /* <POSIX incompliant>, errno should be ENOENT */
	assert_fopen_error(NULL, "w", EINVAL); /* <POSIX incompliant>, errno should be ENOENT */
	/* open with no flags/wrong flags/null flags */
	assert_fopen_error(stdpath, "", EINVAL);
	assert_fopen_error(stdpath, "phoenix-rtos", EINVAL);
	assert_fopen_error(stdpath, NULL, EINVAL);
	/* open file with too long name */
	assert_fopen_error(toolongpath, "w", ENAMETOOLONG);
}


TEST(fopenfclose, freopen_file)
{
	fd = fopen(stdpath, "w");
	/* freopen() on opened file */
	fd2 = freopen(stdpath, "w", fd);
	{
		TEST_ASSERT_NOT_NULL(fd);
		TEST_ASSERT_NOT_NULL(fd2);
		TEST_ASSERT_TRUE(fd == fd2);
	}
	fclose(fd);
}


TEST(fopenfclose, fdopen_file)
{
	fd = fopen(stdpath, "r");
	{
		TEST_ASSERT_NOT_NULL(fd);
		filedes = fileno(fd);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, filedes);
		fd2 = fdopen(filedes, "r");
		TEST_ASSERT_NOT_NULL(fd2);
	}
	fclose(fd);
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
TEST_GROUP(getput);

TEST_SETUP(getput)
{ /* empty */
}


TEST_TEAR_DOWN(getput)
{ /* empty */
}


TEST(getput, fwritefread_basic)
{
	/* write some data to file using fwrite(), read it using fread(), assert end of file */
	memset(buf, 0, 20);
	fd = fopen(stdpath, "w+");
	{
		TEST_ASSERT_EQUAL_INT(5, fwrite(stdpath, sizeof(char), 5, fd));
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(5, fread(buf, sizeof(char), 5, fd));
		TEST_ASSERT_EQUAL_CHAR_ARRAY(stdpath, buf, 5);
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
	}
	fclose(fd);
}


TEST(getput, getput_basic)
{
	/* Correct write */
	fd = fopen(stdpath, "w");
	{
		TEST_ASSERT_EQUAL_INT('a', fputc('a', fd));
		TEST_ASSERT_EQUAL_INT('b', putc('b', fd));
	}
	fclose(fd);

	/* Correct read */
	fd = fopen(stdpath, "r");
	{
		TEST_ASSERT_EQUAL_INT('a', fgetc(fd));
		TEST_ASSERT_EQUAL_INT('b', getc(fd));
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
		TEST_ASSERT_EQUAL_INT(EOF, getc(fd));
	}
	fclose(fd);

	/* read from file open for writing */
	fd = fopen(stdpath, "w");
	{
		TEST_ASSERT_EQUAL_INT('a', fputc('a', fd));
		TEST_ASSERT_EQUAL_INT('b', fputc('b', fd));
		rewind(fd);

		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	fclose(fd);

	/* Try to write to file open for reading */
	fd = fopen(stdpath, "r");
	{
		TEST_ASSERT_EQUAL_INT(EOF, fputc('a', fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_EQUAL_INT(EOF, fputc('a', fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	fclose(fd);
}


TEST(getput, getsputs_basic)
{
	/* reading/writing from file */
	fd = fopen(stdpath, "w+");
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		rewind(fd);
		TEST_ASSERT_NOT_NULL(fgets(buf, 16, fd));
	}
	fclose(fd);

	/* reading from file not opened for reading */
	fd = fopen(stdpath, "w");
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		TEST_ASSERT_NULL(fgets(buf, 16, fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	fclose(fd);

	fd = fopen(stdpath, "w+");
}


TEST(getput, getsputs_readonly)
{
	/* 
	Upon successful completion, fputc() shall return the value it has written. 
	Otherwise, it shall return EOF, the error indicator for the stream shall be set 
	and errno shall be set to indicate the error. 
	
	https://github.com/phoenix-rtos/phoenix-rtos-project/issues/260
	*/
	TEST_IGNORE();

	fd = fopen(stdpath, "r");
	{
		TEST_ASSERT_EQUAL_INT(EOF, fputs(stdpath, fd)); /* <posix incompliance> returns 0, should EOF */
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_NOT_NULL(fgets(buf, 16, fd));
		TEST_ASSERT_EQUAL_CHAR_ARRAY(stdpath, buf, sizeof(stdpath));
		TEST_ASSERT_NULL(fgets(buf, 16, fd));
	}
	fclose(fd);
}


TEST(getput, ungetc_basic)
{
	/* standard usage of ungetc */
	fd = fopen(stdpath, "w");
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
	fclose(fd);
	fd = fopen(stdpath, "r");
	{
		c = fgetc(fd);
		TEST_ASSERT_EQUAL_INT((int)c, ungetc(c, fd));
		TEST_ASSERT_EQUAL_PTR(buf, fgets(buf, sizeof(stdpath), fd));
		TEST_ASSERT_EQUAL_STRING(stdpath, buf);
	}
	fclose(fd);

	/*	EOF pushback test
		If the value of c equals that of the macro EOF, 
		the operation shall fail and the input stream shall be left unchanged. 
	*/
	fd = fopen(stdpath, "r");
	{
		TEST_ASSERT_EQUAL_INT(EOF, ungetc(EOF, fd));
		TEST_ASSERT_EQUAL_INT(stdpath[0], fgetc(fd));
	}
	fclose(fd);
}

/*
test group for:
getline,
*/
TEST_GROUP(line);

TEST_SETUP(line)
{
	/* file preparation */
	fd = fopen(stdpath, "w+");
	{
		fputs("line1\nlineline2\nline3", fd);
	}
	fclose(fd);
}


TEST_TEAR_DOWN(line)
{ /* empty */
}


TEST(line, getline_basic)
{
	char *line = NULL;
	size_t len = 0;

	/* read using getline */
	fd = fopen(stdpath, "r");
	{
		TEST_ASSERT_EQUAL_INT(6, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_STRING("line1\n", line);

		TEST_ASSERT_EQUAL_INT(10, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_STRING("lineline2\n", line);

		TEST_ASSERT_EQUAL_INT(5, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_STRING("line3", line);

		free(line);
	}
	fclose(fd);
}


TEST(line, getline_wronly)
{
	char *line = NULL;
	size_t len = 0;

	/* read using getline from write-only file */
	fd = fopen(stdpath, "a");
	{
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(-1, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_NULL(line);
	}
	fclose(fd);
}


TEST(line, getline_allocated)
{
	/* If *lineptr is a null pointer or if the object pointed to by *lineptr is of insufficient size, 
	an object shall be allocated as if by malloc() or the object shall be reallocated as if by realloc(), 
	respectively, such that the object is large enough to hold the characters to be written to it, including the terminating NUL, 
	and *n shall be set to the new size */
	/* <posix incompliant> - value pointed to by *len argument should not change but is changed and equals strlen(line) */
	TEST_IGNORE();

	char *line = NULL;
	size_t len = 50; /* allocated memory exceeds one demanded for a line that to be read */

	fd = fopen(stdpath, "r");
	{
		line = malloc(len);
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(6, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_INT(50, len);
		TEST_ASSERT_EQUAL_STRING("line1\n", line);
		free(line);
	}
	fclose(fd);
}


TEST(line, getline_longline)
{
	char *line = NULL;
	size_t len = 50; /* allocated memory exceeds one demanded for a line that to be read */
	int i;

	/* prepare file with one long line */
	fd = fopen(stdpath, "w+");
	{
		for (i = 0; i < 100; i++) {
			fputs("0123456789", fd);
		}
		fputc('\n', fd);
	}
	fclose(fd);

	fd = fopen(stdpath, "r");
	{
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(1001, getline(&line, &len, fd));
		TEST_ASSERT_EQUAL_INT(1001, len);
		TEST_ASSERT_EQUAL_INT(1001, strlen(line));
		free(line);
	}
	fclose(fd);
}

/* 
Test group for:
fseek, fseeko, fsetpos(), rewind()
ftell, ftello
*/
TEST_GROUP(seek);

TEST_SETUP(seek)
{
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
	fclose(fd);
}


TEST_TEAR_DOWN(seek)
{ /* empty */
}


TEST(seek, seek_fseek)
{
	/* fseek() to SEEK_SET/CUR/END macros */
	fd = fopen(stdpath, "a+");
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
	fclose(fd);
}


TEST(seek, seek_fseeko)
{
	/* fseeko() to SEEK_SET/CUR/END macros */
	fd = fopen(stdpath, "a+");
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
	fclose(fd);
}


TEST(seek, seek_fsetpos)
{
	/* <posix incmpliance> function is not implemented in lubphoenix */
	/* The fsetpos() function shall set the file position and state indicators for the stream 
	pointed to by stream according to the value of the object pointed to by pos, 
	which the application shall ensure is a value obtained from an earlier call to fgetpos() on the same stream. 
	If a read or write error occurs, the error indicator for the stream shall be set and fsetpos() fails.*/
	TEST_IGNORE();
}


TEST(seek, seek_readonly)
{
	/* EBADF The file descriptor underlying the stream file is not open for writing ... */
	/* <posix incompliant> returns 0, should EOF */
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/263 */
	TEST_IGNORE();

	fd = fopen(stdpath, "r");
	{
		TEST_ASSERT_EQUAL_INT(EOF, fseek(fd, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		TEST_ASSERT_EQUAL_INT(EOF, fseeko(fd, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
	}
	fclose(fd);
}


TEST(seek, seek_rewind)
{
	/* Rewind to beggining of the file */
	fd = fopen(stdpath, "w+");
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(stdpath[0], fgetc(fd));
	}
	fclose(fd);
}


TEST(seek, seek_ftell)
{
	/* tell position in file after fseek() calls */
	fd = fopen(stdpath, "w+");
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		fseek(fd, 0, SEEK_SET);
		TEST_ASSERT_EQUAL_INT(0, ftell(fd));
		fseek(fd, 4, SEEK_SET);
		TEST_ASSERT_EQUAL_INT(4, ftell(fd));
		fgetc(fd);
		TEST_ASSERT_EQUAL_INT(5, ftell(fd));
	}
	fclose(fd);
}

/*
Tets group for:
fileno, feof, remove
ferror, clearerr
*/
TEST_GROUP(fileop);

TEST_SETUP(fileop)
{ /* empty */
}


TEST_TEAR_DOWN(fileop)
{ /* empty */
}


TEST(fileop, fileop_fileno)
{
	fd = fopen(stdpath, "r");
	TEST_ASSERT_NOT_NULL(fd);
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fileno(fd));
	}
	fclose(fd);
}


TEST(fileop, fileop_feof)
{
	fd = fopen(stdpath, "w+");
	{
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(stdpath, fd));
		TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd));
		TEST_ASSERT_NOT_EQUAL_INT(0, feof(fd));
		rewind(fd);
		TEST_ASSERT_EQUAL_INT(0, feof(fd));
	}
	fclose(fd);
}


TEST(fileop, fileop_remove)
{
	/* fopen() a file and remove() it */
	fd = fopen(stdpath, "w+");
	TEST_ASSERT_NOT_NULL(fd);
	fclose(fd);
	remove(stdpath);
	fd = fopen(stdpath, "r");
	TEST_ASSERT_NULL(fd);

	/* mkdir() a directory and remove() it */
	TEST_ASSERT_EQUAL_INT(0, mkdir("stdio_file_testdir", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
	TEST_ASSERT_EQUAL_INT(0, access("stdio_file_testdir", F_OK));
	TEST_ASSERT_EQUAL_INT(0, remove("stdio_file_testdir"));
}


TEST(fileop, fileop_ferror)
{
	fd = fopen(stdpath, "w");
	{
		c = fgetc(fd);
		TEST_ASSERT_GREATER_THAN_INT(0, ferror(fd));
		clearerr(fd);
		TEST_ASSERT_EQUAL_INT(0, ferror(fd));
	}
	fclose(fd);
}

/*
Test group for:
setvbuf, setbuf, fflush()
*/
TEST_GROUP(bufs);

TEST_SETUP(bufs)
{
	fd = fopen(stdpath, "w+");
	fd2 = fopen(stdpath, "r");
}


TEST_TEAR_DOWN(bufs)
{
	fclose(fd2);
	fclose(fd);
}


TEST(bufs, setbuf_basic)
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


TEST(bufs, setbuf_null)
{
	/* after setbuf() read from file before and after flush */
	setbuf(fd, NULL);
	fputc('a', fd);
	TEST_ASSERT_EQUAL_INT('a', fgetc(fd2));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd2));
}


TEST(bufs, setvbuf_fullbuffer)
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


TEST(bufs, setvbuf_fullbuffer_overflow)
{
	char buf2[8];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(fd, buf2, _IOFBF, 8));

	TEST_ASSERT_GREATER_THAN_INT(0, fputs("0123456789", fd));
	TEST_ASSERT_NOT_NULL(fgets(buf, 20, fd2));
	TEST_ASSERT_EQUAL_INT(10, strlen(buf));
}


TEST(bufs, setvbuf_linebuffer)
{
	char buf2[8];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(fd, buf2, _IOLBF, 8));

	TEST_ASSERT_GREATER_THAN_INT(0, fputs("0123", fd));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fd2));
	TEST_ASSERT_GREATER_THAN_INT(0, fputc('\n', fd));
	TEST_ASSERT_NOT_NULL(fgets(buf, 10, fd2));
	TEST_ASSERT_EQUAL_INT(5, strlen(buf));
}


TEST(bufs, setvbuf_nobuffer)
{
	char buf2[8];

	TEST_ASSERT_EQUAL_INT(0, setvbuf(fd, buf2, _IONBF, 8));

	TEST_ASSERT_GREATER_THAN_INT(0, fputs("0123", fd));
	TEST_ASSERT_NOT_NULL(fgets(buf, 10, fd2));
	TEST_ASSERT_EQUAL_INT(4, strlen(buf));
}


TEST_GROUP_RUNNER(readwrite)
{
	RUN_TEST_CASE(fopenfclose, fopenfclose_file);
	RUN_TEST_CASE(fopenfclose, fopenfclose_specs);
	RUN_TEST_CASE(fopenfclose, freopen_file);
	RUN_TEST_CASE(fopenfclose, fdopen_file)

	RUN_TEST_CASE(line, getline_basic);
	RUN_TEST_CASE(line, getline_wronly);
	RUN_TEST_CASE(line, getline_allocated);
	RUN_TEST_CASE(line, getline_longline)

	RUN_TEST_CASE(getput, fwritefread_basic);
	RUN_TEST_CASE(getput, getput_basic);
	RUN_TEST_CASE(getput, getsputs_basic);
	RUN_TEST_CASE(getput, getsputs_readonly);
	RUN_TEST_CASE(getput, ungetc_basic);

	RUN_TEST_CASE(seek, seek_fseek);
	RUN_TEST_CASE(seek, seek_fseeko);
	RUN_TEST_CASE(seek, seek_fsetpos)
	RUN_TEST_CASE(seek, seek_readonly);
	RUN_TEST_CASE(seek, seek_rewind);
	RUN_TEST_CASE(seek, seek_ftell);

	RUN_TEST_CASE(fileop, fileop_fileno);
	RUN_TEST_CASE(fileop, fileop_feof);
	RUN_TEST_CASE(fileop, fileop_ferror);

	RUN_TEST_CASE(bufs, setbuf_basic);
	RUN_TEST_CASE(bufs, setbuf_null);
	RUN_TEST_CASE(bufs, setvbuf_fullbuffer);
	RUN_TEST_CASE(bufs, setvbuf_fullbuffer_overflow);
	RUN_TEST_CASE(bufs, setvbuf_linebuffer);
	RUN_TEST_CASE(bufs, setvbuf_nobuffer);
}
