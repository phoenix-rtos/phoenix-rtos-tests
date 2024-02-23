#ifdef WITH_AES_EAX

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <unity_fixture.h>

#include <aes.h>
#include <aes_eax.h>

TEST_GROUP(aes_eax);

TEST_SETUP(aes_eax)
{
    srand(time(NULL));
}

TEST_TEAR_DOWN(aes_eax)
{
    /* Nothing to do here */
}

TEST(aes_eax, aes_eax__encr_decr_w_damage_test_short)
{
    size_t idx;
    struct AES_ctx aes;

    uint8_t msg[] = { 0xF7, 0xFB };
    uint8_t key[] = { 0x91, 0x94, 0x5D, 0x3F, 0x4D, 0xCB, 0xEE, 0x0B, 0xF4, 0x5E, 0xF5, 0x22, 0x55, 0xF0, 0x95, 0xA4 };
    uint8_t nonce[] = { 0xBE, 0xCA, 0xF0, 0x43, 0xB0, 0xA2, 0x3D, 0x84, 0x31, 0x94, 0xBA, 0x97, 0x2C, 0x66, 0xDE, 0xBD };
    uint8_t hdr[] = { 0xFA, 0x3B, 0xFD, 0x48, 0x06, 0xEB, 0x53, 0xFA };
    uint8_t cipher[] = { 0x19, 0xDD, 0x5C, 0x4C, 0x93, 0x31, 0x04, 0x9D, 0x0B, 0xDA, 0xB0, 0x27, 0x74, 0x08, 0xF6, 0x79, 0x67, 0xE5 };

    uint8_t data[sizeof(msg)], tag[AES_KEYLEN] = { 0 };

    /* encrypt message */
    memcpy(data, msg, sizeof(msg));
    TEST_ASSERT_EQUAL_INT(0, AES_EAX_crypt(key, nonce, sizeof(nonce), /* initialization vector */
                                           hdr, sizeof(hdr),          /* header */
                                           data, sizeof(data),        /* message to encrypt */
                                           tag,                       /* tag */
                                           aes_eax__encrypt));

    /* verify encrypted data end tag separately */
    TEST_ASSERT_EQUAL_MEMORY(data, cipher, sizeof(data));
    TEST_ASSERT_EQUAL_MEMORY(tag, &cipher[sizeof(data)], sizeof(tag));

    /* decrypt and test positive authentication */
    if (AES_EAX_crypt(key, nonce, sizeof(nonce), /* initialization vector */
                      hdr, sizeof(hdr),          /* header */
                      data, sizeof(data),        /* message to decrypt */
                      &cipher[sizeof(data)],     /* tag */
                      aes_eax__decrypt)          /**/
    ) {
        TEST_FAIL_MESSAGE("Failed to authenticate, wrong tag");
    }

    /* verify plain data */
    TEST_ASSERT_EQUAL_INT(0, memcmp(data, msg, sizeof(data)));

    /* randomly damage the cipher */
    idx = rand() % sizeof(cipher);
    cipher[idx] = ~cipher[idx];

    /* decrypt and test negative authentication */
    TEST_ASSERT_NOT_EQUAL_INT(0, AES_EAX_crypt(key, nonce, sizeof(nonce), /* initialization vector */
                                               hdr, sizeof(hdr),          /* header */
                                               &cipher[0], sizeof(msg),   /* message to decrypt */
                                               &cipher[sizeof(data)],     /* tag */
                                               aes_eax__decrypt));
}

TEST(aes_eax, aes_eax__encr_decr_w_damage_test_long)
{
    size_t idx;
    struct AES_ctx aes;

    uint8_t msg[] = {
        0xA0, 0x02, 0x1D, 0x02, 0x00, 0xED, 0x27, 0x11, 0x00, 0xAF, 0x4D, 0x6D, 0xCC, 0xF1, 0x4D, 0xE7, /* 0000 */
        0xC1, 0xC4, 0x23, 0x5E, 0x6F, 0xEF, 0x6C, 0x15, 0x1F, 0x2B, 0x01, 0x00                          /* 0010 */
    };
    uint8_t key[] = { 0x45, 0xCA, 0x5C, 0xA2, 0x60, 0xB9, 0xDD, 0x87, 0x6A, 0x42, 0x58, 0x74, 0xE6, 0xB5, 0x7F, 0x05 };
    uint8_t nonce[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t hdr[] = {
        0x01, 0x01, 0x00, 0x56, 0x2F, 0x80, 0x11, 0x84, 0x8D, 0x16, 0xBC, 0x76, 0x76, 0xF6, 0x35, 0x65, /* 0000 */
        0x90, 0x12, 0x08, 0x2B, 0x3A, 0x97                                                              /* 0010 */
    };
    uint8_t cipher[] = {
        0x14, 0x31, 0xAE, 0x2D, 0xAF, 0xD9, 0xAC, 0x44, 0x2D, 0x0C, 0x7E, 0x55, 0xB2, 0x9B, 0x89, 0x1B, /* 0000 */
        0xF1, 0x98, 0x45, 0xC5, 0xA8, 0x88, 0xAB, 0x4F, 0x89, 0x8D, 0x6C, 0x56, 0x56, 0x21, 0xFE, 0xB3, /* 0010 */
        0xA6, 0x0C, 0x10, 0xF8, 0x2E, 0xE6, 0xC8, 0xF3, 0xF8, 0x8B, 0x99, 0x1E                          /* 0020 */
    };

    uint8_t data[sizeof(msg)], tag[AES_KEYLEN] = { 0 };

    /* encrypt message */
    memcpy(data, msg, sizeof(msg));
    TEST_ASSERT_EQUAL_INT(0, AES_EAX_crypt(key, nonce, sizeof(nonce), /* initialization vector */
                                           hdr, sizeof(hdr),          /* header */
                                           data, sizeof(data),        /* message to encrypt */
                                           tag,                       /* tag */
                                           aes_eax__encrypt));

    /* verify encrypted data end tag separately */
    TEST_ASSERT_EQUAL_MEMORY(data, cipher, sizeof(data));
    TEST_ASSERT_EQUAL_MEMORY(tag, &cipher[sizeof(data)], sizeof(tag));

    /* decrypt and test positive authentication */
    if (AES_EAX_crypt(key, nonce, sizeof(nonce), /* initialization vector */
                      hdr, sizeof(hdr),          /* header */
                      data, sizeof(data),        /* message to decrypt */
                      &cipher[sizeof(data)],     /* tag */
                      aes_eax__decrypt)          /**/
    ) {
        TEST_FAIL_MESSAGE("Failed to authenticate, wrong tag");
    }

    /* verify plain data */
    TEST_ASSERT_EQUAL_INT(0, memcmp(data, msg, sizeof(data)));

    /* randomly damage the cipher */
    idx = rand() % sizeof(cipher);
    cipher[idx] = ~cipher[idx];

    /* decrypt and test negative authentication */
    TEST_ASSERT_NOT_EQUAL_INT(0, AES_EAX_crypt(key, nonce, sizeof(nonce), /* initialization vector */
                                               hdr, sizeof(hdr),          /* header */
                                               &cipher[0], sizeof(msg),   /* message to decrypt */
                                               &cipher[sizeof(data)],     /* tag */
                                               aes_eax__decrypt));
}


TEST_GROUP_RUNNER(aes_eax)
{
    RUN_TEST_CASE(aes_eax, aes_eax__encr_decr_w_damage_test_short);
    RUN_TEST_CASE(aes_eax, aes_eax__encr_decr_w_damage_test_long);
}

#endif /* end of WITH_AES_EAX */
