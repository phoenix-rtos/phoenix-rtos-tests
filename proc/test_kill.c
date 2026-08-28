#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "unity_fixture.h"

TEST_GROUP(test_kill);


TEST_SETUP(test_kill)
{
}


TEST_TEAR_DOWN(test_kill)
{
}


TEST(test_kill, kill_pid_zero)
{
    int res;

    res = kill(0, 0);
    TEST_ASSERT_EQUAL_INT(0, res);
}


TEST(test_kill, kill_pid_negone)
{
    int res;

    res = kill(-1, 0);
    TEST_ASSERT_EQUAL_INT(0, res);
}


TEST_GROUP_RUNNER(test_kill)
{
    RUN_TEST_CASE(test_kill, kill_pid_zero);
    RUN_TEST_CASE(test_kill, kill_pid_negone);
}


void runner(void)
{
    RUN_TEST_GROUP(test_kill);
}


int main(int argc, char *argv[])
{
    int failures = UnityMain(argc, (const char **)argv, runner);
    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
