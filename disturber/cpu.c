/*
 * Phoenix-RTOS
 *
 * Disturber CPU
 *
 * CPU-time saturating module
 *
 * Copyright 2026 Phoenix Systems
 * Author: Jakub Klimek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "disturber.h"

#if defined(__ARM_FEATURE_MVE)
#include <arm_mve.h>
#endif

#define DATA_LEN 0x5000U


static char data[DATA_LEN];


static int loop(int n)
{
	for (int i = 0; i < n; i++) {
		__asm__ volatile("nop");
	}
	return 0;
}


static int fibonacci(int n)
{
	if (n <= 1) {
		return 0;
	}
	int a = 0, b = 1, c;
	for (int j = 2; j <= n; j++) {
		c = a + b;
		a = b;
		b = c;
	}
	__asm__ volatile("" : : "r"(b) : "memory");
	return b;
}


static unsigned long crypto(int n)
{
	uint32_t key[4];
	uint32_t h = 0;

	for (int j = 0; j < 4; j++) {
		key[j] = ((uint32_t)(unsigned char)data[j * 4] << 24) |
				((uint32_t)(unsigned char)data[j * 4 + 1] << 16) |
				((uint32_t)(unsigned char)data[j * 4 + 2] << 8) |
				(uint32_t)(unsigned char)data[j * 4 + 3];
	}

	for (int iteration = 0; iteration < n; iteration++) {
		for (unsigned int offset = 0; offset + 3U < DATA_LEN; offset += 4U) {
			uint32_t v0 = ((uint32_t)(unsigned char)data[offset] << 24) |
					((uint32_t)(unsigned char)data[offset + 1] << 16) |
					((uint32_t)(unsigned char)data[offset + 2] << 8) |
					(uint32_t)(unsigned char)data[offset + 3];
			uint32_t v1 = h ^ (uint32_t)iteration ^ offset;
			uint32_t sum = 0;

			/* XTEA: a compact block cipher commonly suited to small systems. */
			for (int round = 0; round < 32; round++) {
				v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3U]);
				sum += 0x9e3779b9U;
				v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3U]);
			}
			h ^= v0 ^ v1;
		}
	}
	__asm__ volatile("" : : "r"(h) : "memory");
	return h;
}


static unsigned long fixed_point_fft(int n)
{
	/* 16-point radix-2 FFT; data supplies signed, random Q0 input samples. */
	static const int16_t twiddleRe[8] = { 32767, 30274, 23170, 12540, 0, -12540, -23170, -30274 };
	static const int16_t twiddleIm[8] = { 0, -12540, -23170, -30274, -32767, -30274, -23170, -12540 };
	unsigned long hash = 0;

	for (int iteration = 0; iteration < n; iteration++) {
		int32_t real[16];
		int32_t imag[16];
		size_t offset = ((size_t)iteration * 32U) % DATA_LEN;

		for (int i = 0; i < 16; i++) {
			real[i] = (int8_t)data[(offset + (size_t)i * 2U) % DATA_LEN];
			imag[i] = (int8_t)data[(offset + (size_t)i * 2U + 1U) % DATA_LEN];
		}

		/* Bit-reverse the input before iterative decimation-in-time stages. */
		for (int i = 1, j = 0; i < 16; i++) {
			int bit = 8;
			for (; (j & bit) != 0; bit >>= 1) {
				j ^= bit;
			}
			j ^= bit;
			if (i < j) {
				int32_t value = real[i];
				real[i] = real[j];
				real[j] = value;
				value = imag[i];
				imag[i] = imag[j];
				imag[j] = value;
			}
		}

		for (int length = 2; length <= 16; length <<= 1) {
			int half = length >> 1;
			int twiddleStep = 16 / length;

			for (int base = 0; base < 16; base += length) {
				for (int j = 0; j < half; j++) {
					int index = j * twiddleStep;
					int32_t br = real[base + j + half];
					int32_t bi = imag[base + j + half];
					int32_t tr = (twiddleRe[index] * br - twiddleIm[index] * bi) / 32768;
					int32_t ti = (twiddleRe[index] * bi + twiddleIm[index] * br) / 32768;
					int32_t ar = real[base + j];
					int32_t ai = imag[base + j];

					/* Scale each stage to retain a fixed-point Q0 result. */
					real[base + j] = (ar + tr) / 2;
					imag[base + j] = (ai + ti) / 2;
					real[base + j + half] = (ar - tr) / 2;
					imag[base + j + half] = (ai - ti) / 2;
				}
			}
		}

		for (int i = 0; i < 16; i++) {
			hash = (hash << 5) - hash + (unsigned long)(uint32_t)(real[i] ^ imag[i]);
		}
	}

	__asm__ volatile("" : : "r"(hash) : "memory");
	return hash;
}


static unsigned long fixed_point_fft_simd(int n)
{
#if defined(__ARM_FEATURE_MVE)
	/* Q15 twiddle factors for a 16-point radix-2 FFT. */
	static const int16_t twiddleRe[8] = { 32767, 30274, 23170, 12540, 0, -12540, -23170, -30274 };
	static const int16_t twiddleIm[8] = { 0, -12540, -23170, -30274, -32767, -30274, -23170, -12540 };
	unsigned long hash = 0;

	for (int iteration = 0; iteration < n; iteration++) {
		int32_t real[16];
		int32_t imag[16];
		size_t offset = ((size_t)iteration * 32U) % DATA_LEN;

		for (int i = 0; i < 16; i++) {
			real[i] = (int8_t)data[(offset + (size_t)i * 2U) % DATA_LEN];
			imag[i] = (int8_t)data[(offset + (size_t)i * 2U + 1U) % DATA_LEN];
		}

		/* Bit-reverse the input for iterative decimation-in-time. */
		for (int i = 1, j = 0; i < 16; i++) {
			int bit = 8;
			for (; (j & bit) != 0; bit >>= 1) {
				j ^= bit;
			}
			j ^= bit;
			if (i < j) {
				int32_t value = real[i];
				real[i] = real[j];
				real[j] = value;
				value = imag[i];
				imag[i] = imag[j];
				imag[j] = value;
			}
		}

		/* Process four independent butterflies per MVE vector. */
		for (int length = 2; length <= 16; length <<= 1) {
			int half = length >> 1;
			int twiddleStep = 16 / length;

			for (int butterfly = 0; butterfly < 8; butterfly += 4) {
				int32_t ar[4], ai[4], br[4], bi[4], wr[4], wi[4];
				int indices[4];

				for (int lane = 0; lane < 4; lane++) {
					int number = butterfly + lane;
					int base = (number / half) * length;
					int j = number % half;
					int index = base + j;
					int twiddle = j * twiddleStep;

					indices[lane] = index;
					ar[lane] = real[index];
					ai[lane] = imag[index];
					br[lane] = real[index + half];
					bi[lane] = imag[index + half];
					wr[lane] = twiddleRe[twiddle];
					wi[lane] = twiddleIm[twiddle];
				}

				int32x4_t var = vldrwq_s32(ar);
				int32x4_t vai = vldrwq_s32(ai);
				int32x4_t vbr = vldrwq_s32(br);
				int32x4_t vbi = vldrwq_s32(bi);
				int32x4_t vwr = vldrwq_s32(wr);
				int32x4_t vwi = vldrwq_s32(wi);
				int32x4_t vtr = vshrq_n_s32(vsubq_s32(vmulq_s32(vwr, vbr), vmulq_s32(vwi, vbi)), 15);
				int32x4_t vti = vshrq_n_s32(vaddq_s32(vmulq_s32(vwr, vbi), vmulq_s32(vwi, vbr)), 15);

				vstrwq_s32(ar, vshrq_n_s32(vaddq_s32(var, vtr), 1));
				vstrwq_s32(ai, vshrq_n_s32(vaddq_s32(vai, vti), 1));
				vstrwq_s32(br, vshrq_n_s32(vsubq_s32(var, vtr), 1));
				vstrwq_s32(bi, vshrq_n_s32(vsubq_s32(vai, vti), 1));

				for (int lane = 0; lane < 4; lane++) {
					real[indices[lane]] = ar[lane];
					imag[indices[lane]] = ai[lane];
					real[indices[lane] + half] = br[lane];
					imag[indices[lane] + half] = bi[lane];
				}
			}
		}

		for (int i = 0; i < 16; i++) {
			hash = (hash << 5) - hash + (unsigned long)(uint32_t)(real[i] ^ imag[i]);
		}
	}

	__asm__ volatile("" : : "r"(hash) : "memory");
	return hash;
#else
	return fixed_point_fft(n);
#endif
}


static double floating_point_fft(int n)
{
	/* 16-point radix-2 FFT; each iteration uses another window of data. */
	static const double twiddleRe[8] = {
		1.0, 0.9238795325112867, 0.7071067811865476, 0.3826834323650898,
		0.0, -0.3826834323650898, -0.7071067811865476, -0.9238795325112867
	};
	static const double twiddleIm[8] = {
		0.0, -0.3826834323650898, -0.7071067811865476, -0.9238795325112867,
		-1.0, -0.9238795325112867, -0.7071067811865476, -0.3826834323650898
	};
	double checksum = 0.0;

	for (int iteration = 0; iteration < n; iteration++) {
		double real[16];
		double imag[16];
		size_t offset = ((size_t)iteration * 32U) % DATA_LEN;

		for (int i = 0; i < 16; i++) {
			real[i] = (double)(int8_t)data[(offset + (size_t)i * 2U) % DATA_LEN];
			imag[i] = (double)(int8_t)data[(offset + (size_t)i * 2U + 1U) % DATA_LEN];
		}

		/* Bit-reverse the input for iterative decimation-in-time stages. */
		for (int i = 1, j = 0; i < 16; i++) {
			int bit = 8;
			for (; (j & bit) != 0; bit >>= 1) {
				j ^= bit;
			}
			j ^= bit;
			if (i < j) {
				double value = real[i];
				real[i] = real[j];
				real[j] = value;
				value = imag[i];
				imag[i] = imag[j];
				imag[j] = value;
			}
		}

		for (int length = 2; length <= 16; length <<= 1) {
			int half = length >> 1;
			int twiddleStep = 16 / length;

			for (int base = 0; base < 16; base += length) {
				for (int j = 0; j < half; j++) {
					int index = j * twiddleStep;
					double br = real[base + j + half];
					double bi = imag[base + j + half];
					double tr = twiddleRe[index] * br - twiddleIm[index] * bi;
					double ti = twiddleRe[index] * bi + twiddleIm[index] * br;
					double ar = real[base + j];
					double ai = imag[base + j];

					real[base + j] = ar + tr;
					imag[base + j] = ai + ti;
					real[base + j + half] = ar - tr;
					imag[base + j + half] = ai - ti;
				}
			}
		}

		for (int i = 0; i < 16; i++) {
			checksum += real[i] * 0.5 + imag[i] * 0.25;
		}
	}

	__asm__ volatile("" : : "g"(checksum) : "memory");
	return checksum;
}


static double montecarlo_pi(int n)
{
	int inside = 0;
	for (int i = 0; i < n; i++) {
		double x = (double)rand() / RAND_MAX;
		double y = (double)rand() / RAND_MAX;

		if (x * x + y * y <= 1.0) {
			inside++;
		}
	}
	double pi_estimate = 4.0 * inside / n;
	return pi_estimate;
}


static void *worker_thread(void *arg)
{
	int threadId = (int)(intptr_t)arg;

	while (1) {
		if (disturber_sleepMs > 0) {
			usleep(disturber_sleepMs * 1000U);
		}
		if ((threadId & 1) == 1) {
			loop(10000000);
			fibonacci(1000000);
			crypto(100);
			fixed_point_fft(100);
		}
		if ((threadId & 2) == 2) {
			/* FPU/Vector operations */
			fixed_point_fft_simd(100);
			floating_point_fft(100);
			montecarlo_pi(1000000);
		}
	}

	return NULL;
}


static void init(void)
{
	for (size_t i = 0; i < DATA_LEN; i++) {
		data[i] = rand() % 256;
	}
}


const disturber_t disturber_cpu = {
	.name = "cpu",
	.nthreads = 2,
	.firstThreadId = 1,
	.init = init,
	.worker = worker_thread,
};
