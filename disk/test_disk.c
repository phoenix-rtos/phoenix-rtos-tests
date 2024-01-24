/*
 * Phoenix-RTOS
 *
 * Disk benchmark - based on lmbench (https://github.com/intel/lmbench/blob/master/src/disk.c)
 *
 * Copyright 2020, 2021 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "errno.h"
#include "fcntl.h"
#include "inttypes.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"

#include "sys/mman.h"
#include "sys/time.h"


/* Common definitions */
#define BLOCK_SIZE         512     /* Disk block size */

/* Seek test definitions */
#define SEEK_POINTS        2000    /* Number of seeks to perform */
#define SEEK_MIN_STRIDE    512     /* Min seek stride */
#define SEEK_MIN_TIME      1000    /* Min valid seek time in usec */
#define SEEK_MAX_TIME      1000000 /* Max valid seek time in usec */

/* Zone test definitions */
#define ZONE_POINTS        150     /* Number of zones to test */
#define ZONE_MIN_STRIDE    512     /* Min zone stride */

/* Pattern test definitions */
#define PATTERN_POINTS     10      /* Number of pattern zones to test */
#define PATTERN_MAX_BLOCKS 512     /* Max blocks to read/write per single pattern test */

/* Performance test definitions */
#define PERF_BLOCKS        0x8000  /* Blocks to read/write per single performance test */

/* Misc definitions */
#define BP_OFFS            0       /* Offset of 0 exponent entry in binary prefix table */
#define BP_EXP_OFFS        10      /* Offset between consecutive entries exponents in binary prefix table */
#define SI_OFFS            8       /* Offset of 0 exponent entry in SI prefix table */
#define SI_EXP_OFFS        3       /* Offset between consecutive entries exponents in SI prefix table */


/* Binary (base 2) prefixes */
static const char *bp[] = {
	"",  /* 2^0         */
	"K", /* 2^10   kibi */
	"M", /* 2^20   mebi */
	"G", /* 2^30   gibi */
	"T", /* 2^40   tebi */
	"P", /* 2^50   pebi */
	"E", /* 2^60   exbi */
	"Z", /* 2^70   zebi */
	"Y"  /* 2^80   yobi */
};


/* SI (base 10) prefixes */
static const char* si[] = {
	"y", /* 10^-24 yocto */
	"z", /* 10^-21 zepto */
	"a", /* 10^-18 atto  */
	"f", /* 10^-15 femto */
	"p", /* 10^-12 pico  */
	"n", /* 10^-9  nano  */
	"u", /* 10^-6  micro */
	"m", /* 10^-3  milli */
	"",  /* 10^0         */
	"k", /* 10^3   kilo  */
	"M", /* 10^6   mega  */
	"G", /* 10^9   giga  */
	"T", /* 10^12  tera  */
	"P", /* 10^15  peta  */
	"E", /* 10^18  exa   */
	"Z", /* 10^21  zetta */
	"Y", /* 10^24  yotta */
};


typedef struct timeval timeval_t;


static int test_disk_mod(int x, int y)
{
	int ret = x % y;

	if (ret < 0)
		ret += abs(y);

	return ret;
}


static int test_disk_div(int x, int y)
{
	return (x - test_disk_mod(x, y)) / y;
}


static int test_disk_log(unsigned int base, unsigned int x)
{
	int ret = 0;

	while (x /= base)
		ret++;

	return ret;
}


static int test_disk_pow(int x, unsigned int y)
{
	int ret = 1;

	while (y) {
		if (y & 1)
			ret *= x;
		y >>= 1;
		if (!y)
			break;
		x *= x;
	}

	return ret;
}


static const char *test_disk_bp(int exp)
{
	exp = test_disk_div(exp, BP_EXP_OFFS) + BP_OFFS;

	if ((exp < 0) || (exp >= sizeof(bp) / sizeof(bp[0])))
		return NULL;

	return bp[exp];
}


static const char *test_disk_si(int exp)
{
	exp = test_disk_div(exp, SI_EXP_OFFS) + SI_OFFS;

	if ((exp < 0) || (exp >= sizeof(si) / sizeof(si[0])))
		return NULL;

	return si[exp];
}


/* Converts n = x * base ^ y to a short binary(base 2) or SI(base 10) prefix notation */
static char *test_disk_prefix(unsigned int base, int x, int y, unsigned int prec, char *buff)
{
	int div = test_disk_log(base, abs(x)), exp = div + y;
	int offs, ipart, fpart;
	const char *(*fp)(int);
	const char *prefix;

	/* Support precision for up to 8 decimal places */
	if (prec > 8)
		return NULL;

	switch (base) {
	/* Binary prefix */
	case 2:
		fp = test_disk_bp;
		offs = BP_EXP_OFFS;
		break;

	/* SI prefix */
	case 10:
		fp = test_disk_si;
		offs = SI_EXP_OFFS;
		break;

	default:
		return NULL;
	}

	/* div < 0 => accumulate extra exponents in x */
	if ((div -= test_disk_mod(exp, offs)) < 0) {
		x *= test_disk_pow(base, -div);
		div = 0;
	}
	div = test_disk_pow(base, div);

	/* Save integer part and fractional part as percentage */
	ipart = abs(x) / div;
	fpart = (int)((uint64_t)test_disk_pow(10, prec + 1) * (abs(x) % div) / div);

	/* Round the result */
	if ((fpart = (fpart + 5) / 10) == test_disk_pow(10, prec)) {
		ipart++;
		fpart = 0;
		if (ipart == test_disk_pow(base, offs)) {
			ipart = 1;
			exp += offs;
		}
	}

	/* Remove trailing zeros */
	while (fpart && !(fpart % 10)) {
		fpart /= 10;
		prec--;
	}

	/* Get the prefix */
	if ((prefix = fp((!ipart && !fpart) ? y : exp)) == NULL)
		return NULL;

	if (x < 0)
		*buff++ = '-';

	if (fpart)
		sprintf(buff, "%d.%0*d%s", ipart, prec, fpart, prefix);
	else
		sprintf(buff, "%d%s", ipart, prefix);

	return buff;
}


/* Calculates time delta in usec */
static uint64_t test_disk_time(timeval_t *start, timeval_t *end)
{
	timeval_t diff;

	diff.tv_sec = end->tv_sec - start->tv_sec;
	diff.tv_usec = end->tv_usec - start->tv_usec;

	if ((diff.tv_usec < 0) && (diff.tv_sec > 0)) {
		diff.tv_sec--;
		diff.tv_usec += 1000000;
	}

	if ((diff.tv_usec < 0) || (end->tv_sec < start->tv_sec)) {
		diff.tv_sec = 0;
		diff.tv_usec = 0;
	}

	return diff.tv_usec + 1000000 * diff.tv_sec;
}


/* Performs lseek to 64-bit offset */
static int test_disk_lseek(int fd, uint64_t offs)
{
	uint64_t i, step = 1 << 30;

	lseek(fd, 0, 0);

	/* Perform safe seek in 1GB steps */
	for (i = 0; offs - i > step; i += step) {
		if (lseek(fd, (off_t)step, SEEK_CUR) < 0)
			break;
	}

	return lseek(fd, (off_t)(offs - i), SEEK_CUR);
}


/* Estimates disk size with 1MB accuracy */
static uint64_t test_disk_size(int fd)
{
	uint64_t offs = 0, step = 1 << 30;
	char buff[BLOCK_SIZE];

	/* Move forward in 1GB steps */
	for (offs += step; !((test_disk_lseek(fd, offs) < 0) || (read(fd, buff, sizeof(buff)) != sizeof(buff))); offs += step);
	offs -= step;
	step = 1 << 25;

	/* Move forward in 32MB steps */
	for (offs += step; !((test_disk_lseek(fd, offs) < 0) || (read(fd, buff, sizeof(buff)) != sizeof(buff))); offs += step);
	offs -= step;
	step = 1 << 20;

	/* Move forward in 1MB steps */
	for (offs += step; !((test_disk_lseek(fd, offs) < 0) || (read(fd, buff, sizeof(buff)) != sizeof(buff))); offs += step);
	offs -= step;

	return offs;
}


/* Measures seek + 1 block read */
static ssize_t test_disk_seektime(int fd, uint64_t offs)
{
	timeval_t start, end;
	char buff[BLOCK_SIZE];

	gettimeofday(&start, NULL);

	if (test_disk_lseek(fd, offs) < 0) {
		fprintf(stderr, "test_disk: bad lseek at offs=%" PRIu64 "\n", offs);
		return -EINVAL;
	}

	if (read(fd, buff, sizeof(buff)) != sizeof(buff)) {
		fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs);
		return -EIO;
	}

	gettimeofday(&end, NULL);

	return test_disk_time(&start, &end);
}


/* Measures len bytes read */
static ssize_t test_disk_zonetime(int fd, uint64_t offs, char *buff, uint64_t len)
{
	timeval_t start, end;
	uint64_t l = len;
	ssize_t ret;

	if (test_disk_lseek(fd, offs) < 0) {
		fprintf(stderr, "test_disk: bad lseek at offs=%" PRIu64 "\n", offs);
		return -EINVAL;
	}

	if (read(fd, buff, 1024) != 1024) {
		fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs);
		return -EIO;
	}

	gettimeofday(&start, NULL);

	while (l > 0) {
		if ((ret = read(fd, buff, l)) < 0) {
			fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs + (len - l) + 1024);
			return -EIO;
		}
		l -= ret;
	}

	gettimeofday(&end, NULL);

	return test_disk_time(&start, &end);
}


/* Measures n len byte blocks reads */
static ssize_t test_disk_patternrtime(int fd, uint64_t offs, uint8_t *buff, uint64_t len, uint64_t n, uint8_t (*gen)(uint64_t))
{
	uint64_t i, j, time = 0;
	timeval_t start, end;

	for (i = 0; i < n; i++) {
		gettimeofday(&start, NULL);

		if (read(fd, buff, len) != len) {
			fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs + i * len);
			return -EIO;
		}

		gettimeofday(&end, NULL);
		time += test_disk_time(&start, &end);

		if (gen != NULL) {
			for (j = 0; j < len; j++) {
				if (buff[j] != gen(i * len + j)) {
					fprintf(stderr, "test_disk: bad pattern at offs=%" PRIu64 ". Expected %#x, got %#x\n", offs + i * len + j, gen(i * len + j), buff[j]);
					return -EFAULT;
				}
			}
		}
	}

	return time;
}


/* Measures n len byte blocks writes */
static ssize_t test_disk_patternwtime(int fd, uint64_t offs, uint8_t *buff, uint64_t len, uint64_t n, uint8_t (*gen)(uint64_t))
{
	uint64_t i, j, time = 0;
	timeval_t start, end;

	for (i = 0; i < n; i++) {
		if (gen != NULL) {
			for (j = 0; j < len; j++)
				buff[j] = gen(i * len + j);
		}

		gettimeofday(&start, NULL);

		if (write(fd, buff, len) != len) {
			fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs + i * len);
			return -EIO;
		}

		gettimeofday(&end, NULL);
		time += test_disk_time(&start, &end);
	}

	return time;
}


/* Measures n blocks pattern write and read */
static ssize_t test_disk_patterntime(int fd, uint64_t offs, uint64_t blocksz, uint64_t n, uint8_t (*gen)(uint64_t))
{
	ssize_t wret, rret;
	uint8_t *buff;

	if ((buff = malloc(blocksz)) == NULL)
		return -ENOMEM;

	if (test_disk_lseek(fd, offs) < 0) {
		fprintf(stderr, "test_disk: bad lseek at offs=%" PRIu64 "\n", offs);
		free(buff);
		return -EINVAL;
	}

	if ((wret = test_disk_patternwtime(fd, offs, buff, blocksz, n, gen)) < 0) {
		free(buff);
		return wret;
	}

	if (test_disk_lseek(fd, offs) < 0) {
		fprintf(stderr, "test_disk: bad lseek at offs=%" PRIu64 "\n", offs);
		free(buff);
		return -EINVAL;
	}

	if ((rret = test_disk_patternrtime(fd, offs, buff, blocksz, n, gen)) < 0) {
		free(buff);
		return rret;
	}
	free(buff);

	return (uint64_t)wret + (uint64_t)rret;
}


/* Runs seek test */
static int test_disk_seek(int fd, uint64_t disksz)
{
	uint64_t i, j, t, time = 0, stride = (disksz / SEEK_POINTS / SEEK_MIN_STRIDE + 1) * SEEK_MIN_STRIDE;
	unsigned int nseeks = 0;
	char prefix[8];

	for (i = 0, j = (disksz > stride) ? disksz - stride : 0; i < j; i += stride, j -= stride) {
		if ((t = test_disk_seektime(fd, i)) < 0)
			return t;

		/* Seek with time outside this range is either cached or a weirdo */
		if ((t > SEEK_MIN_TIME) && (t < SEEK_MAX_TIME)) {
			time += t;
			nseeks++;
		}

		if ((t = test_disk_seektime(fd, j)) < 0)
			return t;

		/* Seek with time outside this range is either cached or a weirdo */
		if ((t > SEEK_MIN_TIME) && (t < SEEK_MAX_TIME)) {
			time += t;
			nseeks++;
		}
	}

	if (nseeks)
		printf("test_disk: average seek time: %ss\n", test_disk_prefix(10, time / nseeks, -6, 1, prefix));
	else
		fprintf(stderr, "test_disk: no seeks measured\n");

	return EOK;
}


/* Runs zone test */
static int test_disk_zone(int fd, uint64_t disksz, uint64_t blocksz)
{
	uint64_t stride = (disksz / ZONE_POINTS / ZONE_MIN_STRIDE + 1) * ZONE_MIN_STRIDE;
	uint64_t offs, t, time = 0, len = blocksz / _PAGE_SIZE * _PAGE_SIZE;
	unsigned int nzones = 0; 
	char *buff, prefix[8];

	if ((buff = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		fprintf(stderr, "test_disk: failed to allocate memory\n");
		return -ENOMEM;
	}

	/* Move to disk start */
	offs = 0;
	if (test_disk_lseek(fd, offs) < 0) {
		fprintf(stderr, "test_disk: bad lseek at offs=%" PRIu64 "\n", offs);
		munmap(buff, len);
		return -EINVAL;
	}

	/* Catch permissions */
	if (read(fd, buff, 512) != 512) {
		fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs);
		munmap(buff, len);
		return -EIO;
	}

	for (offs = 0; offs < disksz - len - 1024; offs += stride) {
		if ((t = test_disk_zonetime(fd, offs, buff, len)) < 0) {
			munmap(buff, len);
			return t;
		}
		time += t;
		nzones++;
	}
	munmap(buff, len);

	if (nzones)
		printf("test_disk: average zone read time: %ss\n", test_disk_prefix(10, time / nzones, -6, 1, prefix));
	else
		fprintf(stderr, "test_disk: no zone reads measured\n");

	return EOK;
}


/* Generates 0x00 (00000000...) pattern */
static uint8_t test_disk_pattern00(uint64_t idx)
{
	return 0x00;
}


/* Generates 0xff (11111111...) pattern */
static uint8_t test_disk_patternFF(uint64_t idx)
{
	return 0xff;
}


/* Generates 0x55 (01010101...) pattern */
static uint8_t test_disk_pattern55(uint64_t idx)
{
	return 0x55;
}


/* Generates 0xaa (10101010...) pattern */
static uint8_t test_disk_patternAA(uint64_t idx)
{
	return 0xaa;
}


/* Runs one pattern test */
static int test_disk_patternone(int fd, uint64_t disksz, uint8_t (*gen)(uint64_t))
{
	uint64_t offs, n, blocks = disksz / BLOCK_SIZE;
	unsigned int i;

	for (i = 0; i < PATTERN_POINTS; i++) {
		offs = rand() % blocks;
		n = rand() % PATTERN_MAX_BLOCKS + 1;

		if (offs + n > blocks)
			n = blocks - offs;

		if (test_disk_patterntime(fd, offs * BLOCK_SIZE, BLOCK_SIZE, n, gen) < 0)
			return -EFAULT;
	}

	return EOK;
}


/* Runs one performance test */
static int test_disk_perfone(int fd, uint64_t offs, uint64_t blocksz, uint64_t n)
{
	ssize_t srtime, swtime;
	uint8_t *buff;
	char bprefix[8], srprefix[8], swprefix[8];

	if ((buff = malloc(blocksz)) == NULL)
		return -ENOMEM;

	if (test_disk_lseek(fd, offs) < 0) {
		fprintf(stderr, "test_disk: bad lseek at offs=%" PRIu64 "\n", offs);
		free(buff);
		return -EFAULT;
	}

	if ((swtime = test_disk_patternwtime(fd, offs, buff, blocksz, n, NULL)) < 0) {
		free(buff);
		return swtime;
	}

	if (test_disk_lseek(fd, offs) < 0) {
		fprintf(stderr, "test_disk: bad lseek at offs=%" PRIu64 "\n", offs);
		free(buff);
		return -EFAULT;
	}

	if ((srtime = test_disk_patternrtime(fd, offs, buff, blocksz, n, NULL)) < 0) {
		free(buff);
		return srtime;
	}
	free(buff);

	printf("| %5sB  | %-5" PRIu64 "  | %6sB/s  | %7sB/s  |\n",
		test_disk_prefix(2, blocksz, 0, 0, bprefix),
		UINT64_C(2000000) * n / ((uint64_t)srtime + (uint64_t)swtime),
		test_disk_prefix(2, 1000000ULL * n * blocksz / (uint64_t)srtime, 0, 1, srprefix),
		test_disk_prefix(2, 1000000ULL * n * blocksz / (uint64_t)swtime, 0, 1, swprefix));

	return EOK;
}


/* Runs pattern test */
static int test_disk_pattern(int fd, uint64_t disksz)
{
	int err;

	srand(time(NULL));

	printf("test_disk: testing pattern 0x00...\n");
	if ((err = test_disk_patternone(fd, disksz, test_disk_pattern00)) < 0)
		return err;

	printf("test_disk: testing pattern 0xff...\n");
	if ((err = test_disk_patternone(fd, disksz, test_disk_patternFF)) < 0)
		return err;

	printf("test_disk: testing pattern 0x55...\n");
	if ((err = test_disk_patternone(fd, disksz, test_disk_pattern55)) < 0)
		return err;

	printf("test_disk: testing pattern 0xaa...\n");
	if ((err = test_disk_patternone(fd, disksz, test_disk_patternAA)) < 0)
		return err;

	printf("test_disk: pattern test finished successfully\n");
	return EOK;
}


/* Runs performance test */
static int test_disk_perf(int fd, uint64_t disksz)
{
	uint64_t i, len = (PERF_BLOCKS * BLOCK_SIZE > disksz) ? disksz : PERF_BLOCKS * BLOCK_SIZE;
	int err;

	printf("|  BLOCK  |  IOPS  |  SEQ READ  |  SEQ WRITE  |\n");
	for (i = BLOCK_SIZE; i <= len / 4; i <<= 1) {
		if ((err = test_disk_perfone(fd, 0, i, len / i)) < 0)
			return err;
	}

	return EOK;
}


int main(int argc, char *argv[])
{
	uint64_t size;
	int fd;

	if (argc != 2) {
		printf("Usage: %s <disk device>\n", argv[0]);
		return EOK;
	}

	printf("test_disk: starting, main is at %p\n", main);

	if ((fd = open(argv[1], 0)) < 0) {
		fprintf(stderr, "test_disk: failed to open disk %s\n", argv[1]);
		return -EINVAL;
	}

	if (!(size = test_disk_size(fd))) {
		fprintf(stderr, "test_disk: disk %s has less than 1MB of storage capacity required for the tests to run. Exiting...\n", argv[1]);
		return EOK;
	}
	printf("test_disk: disk %s has %" PRIu64 "MB of storage capacity\n", argv[1], size / (1 << 20));

	printf("********************************\n");
	printf("test_disk: starting seek test...\n");
	test_disk_seek(fd, size);

	printf("********************************\n");
	printf("test_disk: starting zone test...\n");
	test_disk_zone(fd, size, 1 << 20);

	/* Warning: destructive test, overwrites disk data */
	printf("***********************************\n");
	printf("test_disk: starting pattern test...\n");
	test_disk_pattern(fd, size);

	/* Warning: destructive test, overwrites disk data */
	printf("***************************************\n");
	printf("test_disk: starting performance test...\n");
	test_disk_perf(fd, size);

	return EOK;
}
