/*
 * Phoenix-RTOS
 *
 * Hello World
 *
 * Example of user application
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unity_fixture.h>
TEST_GROUP(extest);

TEST_SETUP(extest)
{
}


TEST_TEAR_DOWN(extest)
{
}

/*
	dev_t     st_dev;
	ino_t     st_ino;
	mode_t    st_mode;
	nlink_t   st_nlink;

	uid_t     st_uid;
	gid_t     st_gid;
	dev_t     st_rdev;
	off_t     st_size;
*/

TEST(extest, test1)
{
	struct stat buf;
	stat("syspage/psh", &buf);
	printf("%d %d %d %d\n %d %d %d %d\n", buf.st_dev, 
	buf.st_ino, buf.st_mode, buf.st_nlink, buf.st_uid, buf.st_gid, buf.st_rdev, buf.st_size);

	return 0;
}

TEST_GROUP_RUNNER(extest)
{
	RUN_TEST_CASE(extest, test1);
}