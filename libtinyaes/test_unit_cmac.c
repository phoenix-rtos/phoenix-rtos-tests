/*
 * Phoenix-RTOS
 *
 * CMAC unit tests
 *
 * Copyright 2020 Phoenix Systems
 * Author: Daniel Sawka
 *
 * %LICENSE%
 */

#ifdef WITH_AES_CMAC

#include <stdio.h>
#include <string.h>

#include <unity_fixture.h>

#include <cmac.h>

TEST_GROUP(aes_cmac);

TEST_SETUP(aes_cmac)
{
    /* Nothing to do here */
}

TEST_TEAR_DOWN(aes_cmac)
{
    /* Nothing to do here */
}

#if defined(PS_DEBUG) && (PS_DEBUG == 1)

#define PRINT(...) \
    printf(__VA_ARGS__)


#define PRINT_BUFFER_(BUF, SIZE) \
    do { \
        size_t ITER; \
        for (ITER = 0U; ITER < (SIZE); ITER++) { \
            printf("%02x", (BUF)[ITER]); \
        } \
    } while (0)


#define PRINT_BUFFER(BUF, SIZE) \
    do { \
        PRINT_BUFFER_(BUF, SIZE); \
        printf("\n"); \
    } while (0)

#else
#define PRINT(...)
#define PRINT_BUFFER_(BUF, SIZE)
#define PRINT_BUFFER(BUF, SIZE)
#endif

static void test_cmac_generate_subkey(const uint8_t k[AES_KEYLEN], const uint8_t target_k1[AES_BLOCKLEN], const uint8_t target_k2[AES_BLOCKLEN])
{
    uint8_t k1[AES_BLOCKLEN];
    uint8_t k2[AES_BLOCKLEN];

    struct AES_ctx ctx;

    PRINT("---\n");
    PRINT("k = ");
    PRINT_BUFFER(k, AES_KEYLEN);

    AES_init_ctx(&ctx, k);
    CMAC_generate_subkey_k1_k2(&ctx, k1, k2);

    PRINT("k1 = ");
    PRINT_BUFFER(k1, AES_BLOCKLEN);
    PRINT("k2 = ");
    PRINT_BUFFER(k2, AES_BLOCKLEN);

    PRINT("target_k1 = ");
    PRINT_BUFFER(target_k1, AES_BLOCKLEN);
    PRINT("target_k2 = ");
    PRINT_BUFFER(target_k2, AES_BLOCKLEN);
    PRINT("\n");

    TEST_ASSERT_EQUAL_INT(0, memcmp(k1, target_k1, AES_BLOCKLEN));
    TEST_ASSERT_EQUAL_INT(0, memcmp(k2, target_k2, AES_BLOCKLEN));
}

static void test_cmac_calculate(const uint8_t key[AES_KEYLEN], const uint8_t* const* msg, const int* len, const uint8_t target_mac[AES_BLOCKLEN])
{
    struct CMAC_ctx ctx;
    uint8_t mac[AES_BLOCKLEN];
    int i = 0;

    PRINT("---\n");
    PRINT("key = ");
    PRINT_BUFFER(key, AES_KEYLEN);
    PRINT("msg = ");

    CMAC_init_ctx(&ctx, key);

    while (len[i] >= 0) {
        PRINT_BUFFER_(msg[i], (size_t) len[i]);
        CMAC_append(&ctx, msg[i], (size_t) len[i]);
        i += 1;
    }
    PRINT("\n");

    CMAC_calculate(&ctx, mac);

    PRINT("mac = ");
    PRINT_BUFFER(mac, AES_BLOCKLEN);
    PRINT("target_mac = ");
    PRINT_BUFFER(target_mac, AES_BLOCKLEN);
    PRINT("\n");

    TEST_ASSERT_EQUAL_INT(0, memcmp(mac, target_mac, AES_BLOCKLEN));
}

#if AES_KEYLEN == 16

static uint8_t key[AES_KEYLEN] = "\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c";
static uint8_t kmac[AES_KEYLEN] = "\xC9\xCD\x19\xFF\x5A\x9A\xAD\x5A\x6B\xBD\xA1\x3B\xD2\xC4\xC7\xAD";

TEST(aes_cmac, test_subkey_generation)
{
    test_cmac_generate_subkey((uint8_t*) "\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c",
                              (uint8_t*) "\xfb\xee\xd6\x18\x35\x71\x33\x66\x7c\x85\xe0\x8f\x72\x36\xa8\xde",
                              (uint8_t*) "\xf7\xdd\xac\x30\x6a\xe2\x66\xcc\xf9\x0b\xc1\x1e\xe4\x6d\x51\x3b");
}

TEST(aes_cmac, test_cmac_empty_string)
{
    test_cmac_calculate(key,
                        (const uint8_t*[]) { (uint8_t*) "" }, (int[]) { 0, -1 },
                        (uint8_t*) "\xbb\x1d\x69\x29\xe9\x59\x37\x28\x7f\xa3\x7d\x12\x9b\x75\x67\x46");
}

TEST(aes_cmac, test_cmac_one_short_string)
{
    test_cmac_calculate(key,
                        (const uint8_t*[]) { (uint8_t*) "\x6b\xc1\xbe\xe2\x2e" }, (int[]) { 5, -1 },
                        (uint8_t*) "\x40\xc3\xfd\x87\x8a\xbf\x00\x0c\xfa\x99\x98\xb7\x39\x80\xbc\x6c");
}

TEST(aes_cmac, test_cmac_two_short_strings)
{
    test_cmac_calculate(key,
                        (const uint8_t*[]) { (uint8_t*) "\x6b\xc1\xbe", (uint8_t*) "\xe2\x2e" }, (int[]) { 3, 2, -1 },
                        (uint8_t*) "\x40\xc3\xfd\x87\x8a\xbf\x00\x0c\xfa\x99\x98\xb7\x39\x80\xbc\x6c");
}

TEST(aes_cmac, test_cmac_one_full_block)
{
    test_cmac_calculate(key,
                        (const uint8_t*[]) {
                            (uint8_t*) "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a" },
                        (int[]) { 16, -1 },
                        (uint8_t*) "\x07\x0a\x16\xb4\x6b\x4d\x41\x44\xf7\x9b\xdd\x9d\xd0\x4a\x28\x7c");
}

TEST(aes_cmac, test_cmac_three_variable_strings)
{
    test_cmac_calculate(key,
                        (const uint8_t*[]) {
                            (uint8_t*) "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93",
                            (uint8_t*) "\x17\x2a\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
                            (uint8_t*) "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11" },
                        (int[]) { 14, 18, 8, -1 },
                        (uint8_t*) "\xdf\xa6\x67\x47\xde\x9a\xe6\x30\x30\xca\x32\x61\x14\x97\xc8\x27");
}

TEST(aes_cmac, test_cmac_four_full_blocks)
{
    test_cmac_calculate(key,
                        (const uint8_t*[]) {
                            (uint8_t*) "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
                            (uint8_t*) "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
                            (uint8_t*) "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
                            (uint8_t*) "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
                        },
                        (int[]) { 16, 16, 16, 16, -1 },
                        (uint8_t*) "\x51\xf0\xbe\xbf\x7e\x3b\x9d\x92\xfc\x49\x74\x17\x79\x36\x3c\xfe");
}

TEST(aes_cmac, test_cmac_five_variable_strings)
{
    test_cmac_calculate(kmac,
                        (const uint8_t*[]) {
                            (uint8_t*) "\x25\xb3\x0a\x00\x00\x7a\x75\x00",
                            (uint8_t*) "\x20\x07\x10\x90\x58\x47\x5f\x4b\xc9\x1d",
                            (uint8_t*) "\xf8\x78\xb8\x0a\x1b",
                            (uint8_t*) "\x0f\x98\xb6\x29\x02\x4a\xac\x72\x79",
                            (uint8_t*) "\x42\xbf\xc5\x49\x23\x3c\x01\x40\x82\x9b\x93",
                        },
                        (int[]) { 8, 10, 5, 9, 11, -1 },
                        (uint8_t*) "\x21\x92\x4d\x4f\x2f\xb6\x6e\x01\x60\xce\x5f\x71\xf1\xb7\x43\x10");
}

/* TODO: Add test vectors for 192- and 256-bit keys */
/* #elif AES_KEYLEN == 24 */
/* #elif AES_KEYLEN == 32 */

#endif

TEST_GROUP_RUNNER(aes_cmac)
{
    RUN_TEST_CASE(aes_cmac, test_subkey_generation);
    RUN_TEST_CASE(aes_cmac, test_cmac_empty_string);
    RUN_TEST_CASE(aes_cmac, test_cmac_one_short_string);
    RUN_TEST_CASE(aes_cmac, test_cmac_two_short_strings);
    RUN_TEST_CASE(aes_cmac, test_cmac_one_full_block);
    RUN_TEST_CASE(aes_cmac, test_cmac_three_variable_strings);
    RUN_TEST_CASE(aes_cmac, test_cmac_four_full_blocks);
    RUN_TEST_CASE(aes_cmac, test_cmac_five_variable_strings);
}

#endif
