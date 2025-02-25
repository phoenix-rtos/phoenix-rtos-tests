#include "unity.h"
#include "unity_fixture.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 20

static const char* mock_strlen_input = NULL;
static size_t mock_strlen_return = 0;

/* declaration of real function */
size_t __real_strlen(const char *s);

/* mock*/
size_t __wrap_strlen(const char *s)
{
    mock_strlen_input = s;

    return mock_strlen_return;
}

TEST_GROUP(strncat_basic);

TEST_SETUP(strncat_basic)
{
    mock_strlen_input = NULL;
    mock_strlen_return = 0;
}

TEST_TEAR_DOWN(strncat_basic)
{
}

TEST(strncat_basic, correct)
{
    char test_buffer[BUFFER_SIZE] = "Hello";

    mock_strlen_return = 5;

    char* result = strncat(test_buffer, " World", 7);

    TEST_ASSERT_EQUAL_PTR(test_buffer, result);
    TEST_ASSERT_EQUAL_STRING("Hello World", test_buffer);
    TEST_ASSERT_EQUAL_PTR(test_buffer, mock_strlen_input);
}

TEST_GROUP_RUNNER(strncat_basic)
{
    RUN_TEST_CASE(strncat_basic, correct);
}

static void runner(void)
{
    RUN_TEST_GROUP(strncat_basic);
}

int main(int argc, const char* argv[])
{
    return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
