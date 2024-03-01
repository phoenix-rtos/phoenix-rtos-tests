
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include <unity_fixture.h>

#include <aes.h>
#include <aes_gcm.h>


TEST_GROUP(aes_gcm);


TEST_SETUP(aes_gcm)
{
	// Nothing to do here
}


TEST_TEAR_DOWN(aes_gcm)
{
	// Nothing to do here
}


struct test_vector {
	uint8_t key[16];
	uint8_t iv[12];
	uint8_t tag[16];

	const uint8_t *aad;
	size_t aad_len;

	const uint8_t *ptext;
	const uint8_t *ctext;
	size_t xtext_len;
};


// clang-format off
static const struct test_vector vectors[] = {
    // Test case 3 from https://luca-giuzzi.unibs.it/corsi/Support/papers-cryptography/gcm-spec.pdf
    // chosen for: 96 bit IV, no AAD, xtext_len % block_len == 0
    {
        .key = { 0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c, 0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08 },
        .iv = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88 },
        .tag = { 0x4d, 0x5c, 0x2a, 0xf3, 0x27, 0xcd, 0x64, 0xa6, 0x2c, 0xf3, 0x5a, 0xbd, 0x2b, 0xa6, 0xfa, 0xb4 },

        .aad = NULL,
        .aad_len = 0,

        .ptext = (const uint8_t [64]) {
            0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7,
            0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda, 0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, 0x1c, 0x3c, 0x0c, 0x95,
            0x95, 0x68, 0x09, 0x53, 0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25, 0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d,
            0xe6, 0x57, 0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55
        },
        .ctext = (const uint8_t [64]) {
            0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24, 0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c, 0xe3, 0xaa,
            0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0, 0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e, 0x21, 0xd5, 0x14, 0xb2,
            0x54, 0x66, 0x93, 0x1c, 0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05, 0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a,
            0xac, 0x97, 0x3d, 0x58, 0xe0, 0x91, 0x47, 0x3f, 0x59, 0x85
        },
        .xtext_len=64
    },

    // Test case 4 from https://luca-giuzzi.unibs.it/corsi/Support/papers-cryptography/gcm-spec.pdf
    // chosen for: 96 bit IV, with AAD, xtext_len % block_len != 0
    {
        .key = { 0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c, 0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08 },
        .iv = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88 },
        .tag = { 0x5b, 0xc9, 0x4f, 0xbc, 0x32, 0x21, 0xa5, 0xdb, 0x94, 0xfa, 0xe9, 0x5a, 0xe7, 0x12, 0x1a, 0x47 },

        .aad = (const uint8_t[20]) {
            0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xab, 0xad,
            0xda, 0xd2
        },
        .aad_len = 20,

        .ptext = (const uint8_t [60]) {
            0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7,
            0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda, 0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, 0x1c, 0x3c, 0x0c, 0x95,
            0x95, 0x68, 0x09, 0x53, 0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25, 0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d,
            0xe6, 0x57, 0xba, 0x63, 0x7b, 0x39
        },
        .ctext = (const uint8_t [60]) {
            0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24, 0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c, 0xe3, 0xaa,
            0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0, 0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e, 0x21, 0xd5, 0x14, 0xb2,
            0x54, 0x66, 0x93, 0x1c, 0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05, 0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a,
            0xac, 0x97, 0x3d, 0x58, 0xe0, 0x91
        },
        .xtext_len=60
    }
};
// clang-format on


static inline void test_xcrypt(const struct test_vector *tv)
{
	uint8_t xbuf[64];
	struct AES_ctx aes_ctx;

	TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(xbuf), tv->xtext_len);

	memcpy(xbuf, tv->ptext, tv->xtext_len);

	AES_GCM_init(&aes_ctx, tv->key);
	AES_GCM_crypt(&aes_ctx, tv->iv, xbuf, tv->xtext_len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(tv->ctext, xbuf, tv->xtext_len);

	AES_GCM_init(&aes_ctx, tv->key);
	AES_GCM_crypt(&aes_ctx, tv->iv, xbuf, tv->xtext_len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(tv->ptext, xbuf, tv->xtext_len);
};


static inline void test_tag(const struct test_vector *tv)
{
	uint8_t tag[16] = { 0 };
	struct AES_ctx aes_ctx;

	AES_GCM_init(&aes_ctx, tv->key);
	AES_GCM_mac(&aes_ctx, tv->iv, tv->aad, tv->aad_len, tv->ctext, tv->xtext_len, tag);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(tv->tag, tag, sizeof(tag));
};


TEST(aes_gcm, xcrypt)
{
	for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
		test_xcrypt(&vectors[i]);
	}
}


TEST(aes_gcm, tag)
{
	for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
		test_tag(&vectors[i]);
	}
}


TEST_GROUP_RUNNER(aes_gcm)
{
	RUN_TEST_CASE(aes_gcm, xcrypt);
	RUN_TEST_CASE(aes_gcm, tag);
}
