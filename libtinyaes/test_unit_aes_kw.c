/*
 * Phoenix-RTOS
 *
 * AES Key Wrapping unit tests
 *
 * Copyright 2021 Phoenix Systems
 * Author: Daniel Sawka
 *
 * %LICENSE%
 */

#ifdef WITH_AES_KW

#include <stdio.h>
#include <string.h>

#include <unity_fixture.h>

#include <aes.h>
#include <aes_kw.h>

TEST_GROUP(aes_kw);

TEST_SETUP(aes_kw)
{
    /* Nothing to do here */
}

TEST_TEAR_DOWN(aes_kw)
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

static void test_aes_kw_raw_wrap(const uint8_t key[AES_KEYLEN], uint8_t* buf, size_t bufLen, const uint8_t* target, int targetLen)
{
    struct AES_ctx ctx;

    PRINT("---\n");
    PRINT("key = ");
    PRINT_BUFFER(key, AES_KEYLEN);

    AES_init_ctx(&ctx, key);

    PRINT("in = ");
    PRINT_BUFFER(buf, bufLen);

    AES_KW_raw_wrap(&ctx, buf, bufLen);

    PRINT("out = ");
    PRINT_BUFFER(buf, bufLen);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(target, buf, targetLen);

    PRINT("\n");
}

static void test_aes_kw_raw_unwrap(const uint8_t key[AES_KEYLEN], uint8_t* buf, size_t bufLen, const uint8_t* target, int targetLen)
{
    struct AES_ctx ctx;

    PRINT("---\n");
    PRINT("key = ");
    PRINT_BUFFER(key, AES_KEYLEN);

    AES_init_ctx(&ctx, key);

    PRINT("in = ");
    PRINT_BUFFER(buf, bufLen);

    AES_KW_raw_unwrap(&ctx, buf, bufLen);

    PRINT("out = ");
    PRINT_BUFFER(buf, bufLen);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(target, buf, targetLen);

    PRINT("\n");
}

static void test_aes_kwp_ae(const uint8_t key[AES_KEYLEN], uint8_t* buf, size_t bufLen, const uint8_t* target, int targetLen)
{
    int res;
    struct AES_ctx ctx;

    PRINT("---\n");
    PRINT("key = ");
    PRINT_BUFFER(key, AES_KEYLEN);

    AES_init_ctx(&ctx, key);

    PRINT("in = ");
    PRINT_BUFFER(buf + AES_KWP_HEADER_LEN, bufLen);

    res = AES_KWP_wrap(&ctx, buf, bufLen);

    TEST_ASSERT_EQUAL_INT(targetLen, res);

    PRINT("out = ");
    PRINT_BUFFER(buf, targetLen);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(target, buf, targetLen);

    PRINT("\n");
}

static void test_aes_kwp_ad(const uint8_t key[AES_KEYLEN], uint8_t* buf, size_t bufLen, const uint8_t* target, int targetLen)
{
    int res;
    struct AES_ctx ctx;

    PRINT("---\n");
    PRINT("key = ");
    PRINT_BUFFER(key, AES_KEYLEN);

    AES_init_ctx(&ctx, key);

    PRINT("in = ");
    PRINT_BUFFER(buf, bufLen);

    res = AES_KWP_unwrap(&ctx, buf, bufLen);

    if (targetLen < 0) {
        TEST_ASSERT_LESS_THAN_INT(0, res);
        PRINT("Failed as expected\n");
    } else {
        PRINT("out buf = ");
        PRINT_BUFFER(buf, bufLen);

        TEST_ASSERT_EQUAL_INT(targetLen, res);

        PRINT("out = ");
        PRINT_BUFFER(buf + AES_KWP_HEADER_LEN, targetLen);

        TEST_ASSERT_EQUAL_HEX8_ARRAY(target, buf + AES_KWP_HEADER_LEN, targetLen);
    }

    PRINT("\n");
}

#define VECTOR_OK(KEY, B1, B2) \
    { \
        .key = (const uint8_t*) KEY, \
        .b1 = (const uint8_t*) B1, .b1Len = sizeof(B1) - 1, \
        .b2 = (const uint8_t*) B2, .b2Len = sizeof(B2) - 1 \
    }

#define VECTOR_FAIL(KEY, B1) \
    { \
        .key = (const uint8_t*) KEY, \
        .b1 = (const uint8_t*) B1, .b1Len = sizeof(B1) - 1, \
        .b2Len = -1 \
    }

struct kw_test_vector {
    const uint8_t* key;
    const uint8_t* b1;
    int b1Len;
    const uint8_t* b2;
    int b2Len;
};

static const struct kw_test_vector kw_raw_wrap_vectors[] = {
    VECTOR_OK("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
              "\xa6\xa6\xa6\xa6\xa6\xa6\xa6\xa6\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff",
              "\x1f\xa6\x8b\x0a\x81\x12\xb4\x47\xae\xf3\x4b\xd8\xfb\x5a\x7b\x82\x9d\x3e\x86\x23\x71\xd2\xcf\xe5"),
};

static const struct kw_test_vector kw_raw_unwrap_vectors[] = {
    VECTOR_OK("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
              "\x1f\xa6\x8b\x0a\x81\x12\xb4\x47\xae\xf3\x4b\xd8\xfb\x5a\x7b\x82\x9d\x3e\x86\x23\x71\xd2\xcf\xe5",
              "\xa6\xa6\xa6\xa6\xa6\xa6\xa6\xa6\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff"),
};

static const struct kw_test_vector kwp_ae_vectors[] = {
    VECTOR_OK("\x6d\xec\xf1\x0a\x1c\xaf\x8e\x3b\x80\xc7\xa4\xbe\x8c\x9c\x84\xe8",
              "\x49",
              "\x01\xa7\xd6\x57\xfc\x4a\x5b\x21\x6f\x26\x1c\xca\x4d\x05\x2c\x2b"),
    VECTOR_OK("\xa8\xe0\x6d\xa6\x25\xa6\x5b\x25\xcf\x50\x30\x82\x68\x30\xb6\x61",
              "\x43\xac\xff\x29\x31\x20\xdd\x5d",
              "\xb6\xf9\x67\x61\x6d\xd8\xd7\x72\xe9\xfe\xa2\x95\xa4\x56\xdb\xa7"),
    VECTOR_OK("\x78\x65\xe2\x0f\x3c\x21\x65\x9a\xb4\x69\x0b\x62\x9c\xdf\x3c\xc4",
              "\xbd\x68\x43\xd4\x20\x37\x8d\xc8\x96",
              "\x41\xec\xa9\x56\xd4\xaa\x04\x7e\xb5\xcf\x4e\xfe\x65\x96\x61\xe7\x4d\xb6\xf8\xc5\x64\xe2\x35\x00"),
    VECTOR_OK("\xbe\x96\xdc\x19\x5e\xc0\x34\xd6\x16\x48\x6e\xd7\x0e\x97\xfe\x83",
              "\x85\xb5\x43\x7b\x63\x35\xeb\xba\x76\x35\x90\x3a\x44\x93\xd1\x2a\x77\xd9\x35\x7a\x9e\x0d\xbc\x01"
              "\x34\x56\xd8\x5f\x1d\x32\x01",
              "\x97\x47\x69\xb3\xa7\xb4\xd5\xd3\x29\x85\xf8\x7f\xdd\xf9\x99\x06\x31\xe5\x61\x0f\xbf\xb2\x78\x38"
              "\x7b\x58\xb1\xf4\x8e\x05\xc7\x7d\x2f\xb7\x57\x5c\x51\x69\xeb\x0e"),
};

static const struct kw_test_vector kwp_ad_vectors[] = {
    VECTOR_OK("\x49\x31\x9c\x33\x12\x31\xcd\x6b\xf7\x4c\x2f\x70\xb0\x7f\xcc\x5c",
              "\x9c\x21\x1f\x32\xf8\xb3\x41\xf3\x2b\x05\x2f\xed\x5f\x31\xa3\x87",
              "\xe4"),
    VECTOR_FAIL("\x30\xbe\x7f\xf5\x12\x27\xf0\xee\xf7\x86\xcb\x7b\xe2\x48\x25\x10",
                "\x7f\x61\xa0\xa8\xb2\xfe\x78\x03\xf2\x94\x7d\x23\x3e\xc3\xa2\x55"),
    VECTOR_OK("\xbb\xf0\x83\x3c\xae\x02\x02\xb8\x30\xf7\xb9\x57\x55\x96\xef\x2f",
              "\x47\x36\xf4\x48\x8b\x53\xd4\xdc\x27\x10\x3e\x2f\x2e\x7d\x68\x41",
              "\x49\x0f\x98\xc6\x07\xfc\x7b\xb6"),
    VECTOR_FAIL("\xa8\xba\x81\xb7\xb5\xbe\xba\x13\xcf\x2c\xac\xa8\x49\x65\xd6\x75",
                "\x82\x78\x7e\xeb\xb0\xfd\xb7\x92\x83\xfa\x55\xe8\xcf\xdf\x85\x66"),
    VECTOR_OK("\x42\xf6\xde\x78\x7a\x35\xfe\x6d\x40\xab\x7e\x8a\xc3\xf8\xdf\x07",
              "\xcd\x99\x5e\x6f\xf5\x68\xb5\x67\x5b\x4e\xbe\x77\x0b\xb7\x76\x7d\x32\x02\x42\xc8\x14\x46\x92\x1f",
              "\xde\xd9\x79\xc1\x72\x04\xf6\x25\x4d"),
    VECTOR_FAIL("\x0d\xdc\x55\x41\x4f\xb3\xe9\x4d\x65\x27\xda\x3b\x02\x2a\xa9\x45",
                "\xa2\xd7\x3f\x55\x57\xb4\xb4\x1c\x69\x8a\x4f\xa5\x96\x44\x46\x39\x1b\x10\xa4\x5e\x09\x4f\x0e\x72"),
    VECTOR_OK("\x28\x90\x23\x37\x90\x78\xb8\x21\xfc\x24\xf7\x18\xbd\xc9\x43\x31",
              "\xff\x51\xb7\xae\x52\x46\x23\x44\xfc\x45\x5f\x72\xbe\x05\x9b\x56\xa9\x8c\xc8\x33\xa1\xcf\x3b\x20"
              "\xb6\x88\x71\x12\xf5\xa4\x3f\xd4\x5e\x9c\x5f\x51\xe7\xc6\x62\xf4",
              "\xbe\xd5\x24\xc6\x40\x2e\xeb\x77\x38\x69\x6f\x31\x06\x99\x9f\xc9\x31\xbe\xd6\x76\x88\x38\x34\x5d"
              "\x18\xba\x44\xe1\xb0\x32\xb8"),
    VECTOR_FAIL("\x69\x29\x11\x7e\x6c\xb1\x8e\xa4\xa2\x98\x58\x86\xf0\x8c\x0a\xe1",
                "\x5f\xd9\xe7\x7c\x37\x04\x1c\x2e\xbd\x4c\x34\x6d\x5b\x6c\x78\xf7\xb4\x85\xca\x58\x9d\x6b\x0b\x54"
                "\x16\xd0\x28\x7a\x6d\xb3\x6b\x39\xbd\xc9\x61\xb4\xdc\x2f\xec\xbc"),
};

#if AES_KEYLEN == 16

TEST(aes_kw, test_kw_raw_wrap)
{
    unsigned int i;
    uint8_t buf[256];

    for (i = 0; i < sizeof(kw_raw_wrap_vectors) / sizeof(*kw_raw_wrap_vectors); i++) {
        memcpy(buf, kw_raw_wrap_vectors[i].b1, kw_raw_wrap_vectors[i].b1Len);
        test_aes_kw_raw_wrap(kw_raw_wrap_vectors[i].key, buf, kw_raw_wrap_vectors[i].b1Len,
                             kw_raw_wrap_vectors[i].b2, kw_raw_wrap_vectors[i].b2Len);
    }
}

TEST(aes_kw, test_kw_raw_unwrap)
{
    unsigned int i;
    uint8_t buf[256];

    for (i = 0; i < sizeof(kw_raw_unwrap_vectors) / sizeof(*kw_raw_unwrap_vectors); i++) {
        memcpy(buf, kw_raw_unwrap_vectors[i].b1, kw_raw_unwrap_vectors[i].b1Len);
        test_aes_kw_raw_unwrap(kw_raw_unwrap_vectors[i].key, buf, kw_raw_unwrap_vectors[i].b1Len,
                               kw_raw_unwrap_vectors[i].b2, kw_raw_unwrap_vectors[i].b2Len);
    }
}

TEST(aes_kw, test_kwp_ae)
{
    unsigned int i;
    uint8_t buf[256];

    for (i = 0; i < sizeof(kwp_ae_vectors) / sizeof(*kwp_ae_vectors); i++) {
        memcpy(buf + AES_KWP_HEADER_LEN, kwp_ae_vectors[i].b1, kwp_ae_vectors[i].b1Len);
        test_aes_kwp_ae(kwp_ae_vectors[i].key, buf, kwp_ae_vectors[i].b1Len,
                        kwp_ae_vectors[i].b2, kwp_ae_vectors[i].b2Len);
    }
}

TEST(aes_kw, test_kwp_ad)
{
    unsigned int i;
    uint8_t buf[256];

    for (i = 0; i < sizeof(kwp_ad_vectors) / sizeof(*kwp_ad_vectors); i++) {
        memcpy(buf, kwp_ad_vectors[i].b1, kwp_ad_vectors[i].b1Len);
        test_aes_kwp_ad(kwp_ad_vectors[i].key, buf, kwp_ad_vectors[i].b1Len,
                        kwp_ad_vectors[i].b2, kwp_ad_vectors[i].b2Len);
    }
}

#endif

TEST_GROUP_RUNNER(aes_kw)
{
    RUN_TEST_CASE(aes_kw, test_kw_raw_wrap);
    RUN_TEST_CASE(aes_kw, test_kw_raw_unwrap);
    RUN_TEST_CASE(aes_kw, test_kwp_ae);
    RUN_TEST_CASE(aes_kw, test_kwp_ad);
}

#endif
