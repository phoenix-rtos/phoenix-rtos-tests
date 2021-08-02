/*
 * Phoenix-RTOS
 *
 * Graphics library test
 *
 * Copyright 2021 Phoenix Systems
 * Copyright 2002-2007 IMMOS
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <graph.h>

#include "cursor.h"
#include "font.h"
#include "logo8.h"
#include "logo16.h"
#include "logo32.h"


/* Forces all scheduled tasks completion */
static int test_trigger(graph_t *graph)
{
	while (graph_trigger(graph))
		;
	return graph_commit(graph);
}


/* Forces next scheduled task to run immediately after vsync */
static int test_vtrigger(graph_t *graph)
{
	while (graph_trigger(graph))
		;
	while (!graph_vsync(graph))
		;
	return graph_commit(graph);
}


int test_lines1(graph_t *graph, unsigned int dx, unsigned int dy, int step)
{
	unsigned int i;
	int err;

	/* Slow lines */
	for (i = 0; i < 500; i++) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_line(graph, rand() % (graph->width - dx - 2 * step) + step, rand() % (graph->height - dx - 2 * step) + step, rand() % dx, rand() % dy, 1, rand() % (1ULL << 8 * graph->depth), GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	/* Fast lines */
	for (i = 0; i < 100000; i++) {
		if ((err = test_trigger(graph)) < 0)
			return err;
		if ((err = graph_line(graph, rand() % (graph->width - 2 * dx - 2 * step) + step + dx, rand() % (graph->height - 2 * dy - 2 * step) + step + dy, rand() % (2 * dx) - dx, rand() % (2 * dy) - dy, 1, rand() % (1ULL << 8 * graph->depth), GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	/* Move up */
	for (i = 0; i < graph->height; i += step) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_move(graph, 0, step, graph->width, graph->height - step, 0, -step, GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	return EOK;
}


int test_lines2(graph_t *graph, unsigned int dx, unsigned int dy, int step)
{
	unsigned int i;
	int err;

	/* Background rectangle */
	if ((err = graph_rect(graph, dx, dy, graph->width - 2 * dx + 1, graph->height - 2 * dy + 1, rand() % (1ULL << 8 * graph->depth), GRAPH_QUEUE_HIGH)) < 0)
		return err;

	/* Slow lines */
	for (i = 0; i < graph->height - 2 * dy; i += step) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_line(graph, dx, dy + i, graph->width - 2 * dx, graph->height - 2 * dy - i * step, 1, rand() % (1ULL << 8 * graph->depth), GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	for (i = 0; i < graph->width - 2 * dy; i += step) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_line(graph, dx + i, graph->height - dy, graph->width - 2 * dx - i * step, 2 * dy - graph->height, 1, rand() % (1ULL << 8 * graph->depth), GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	/* Move up */
	for (i = 0; i < graph->height; i += step) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_move(graph, 0, step, graph->width, graph->height - step, 0, -step, GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	return EOK;
}


int test_rectangles(graph_t *graph, unsigned int dx, unsigned int dy, int step)
{
	unsigned int i;
	int err;

	/* Slow rectangles */
	for (i = 0; i < 300; i++) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_rect(graph, rand() % (graph->width - dx - 2 * step) + step, rand() % (graph->height - dy - 2 * step) + step, dx, dy, rand() % (1ULL << 8 * graph->depth), GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	/* Fast rectangles */
	for (i = 0; i < 10000; i++) {
		if ((err = test_trigger(graph)) < 0)
			return err;
		if ((err = graph_rect(graph, rand() % (graph->width - dx - 2 * step) + step, rand() % (graph->height - dy - 2 * step) + step, dx, dy, rand() % (1ULL << 8 * graph->depth), GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	/* Move right */
	for (i = 0; i < graph->width; i += step) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_move(graph, 0, 0, graph->width - step, graph->height, step, 0, GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	return EOK;
}


int test_logo(graph_t *graph, int step)
{
	static const char text[] = "Phoenix-RTOS";                      /* Text under logo */
	static const unsigned int fx = (sizeof(text) - 1) * font.width; /* Text width */
	static const unsigned int fy = font.height;                     /* Text height */
	static const unsigned int lx = 200;                             /* Logo width */
	static const unsigned int ly = 150;                             /* Logo height */
	static const unsigned int dy = ly + (6 * fy) / 5;               /* Total height */
	const unsigned char *logo;
	unsigned char buff[2][3];
	unsigned int i, x, y, bg, fg;
	int err, sy, ay;

	switch (graph->depth) {
		case 1:
			logo = logo8[0];
			bg = *(uint8_t *)logo;
			fg = 1;
			graph_colorget(graph, buff[0], 0, 1);
			graph_colorset(graph, cmap[0], 0, 1);
			break;

		case 2:
			logo = logo16[0];
			bg = *(uint16_t *)logo;
			fg = 0xffff;
			break;

		case 4:
			logo = logo32[0];
			bg = *(uint32_t *)logo;
			fg = 0xffffffff;
			break;

		default:
			printf("test_graph: logo test not supported for selected graphics mode. Skipping...\n");
			return EOK;
	}

	x = graph->width - lx - 2 * step;
	y = graph->height - dy - 2 * step;

	/* Compose logo at bottom left corner */
	if ((err = graph_rect(graph, 0, 0, graph->width, graph->height, bg, GRAPH_QUEUE_HIGH)) < 0)
		return err;
	if ((err = graph_copy(graph, logo, (void *)((uintptr_t)graph->data + graph->depth * ((graph->height - dy) * graph->width + step)), lx, ly, graph->depth * lx, graph->depth * graph->width, GRAPH_QUEUE_HIGH)) < 0)
		return err;
	if ((err = graph_print(graph, &font, text, step + (lx - fx) / 2 + 1, graph->height - fy, font.height, font.height, fg, GRAPH_QUEUE_HIGH)) < 0)
		return err;

	/* Move right */
	for (i = 0; i < x; i += step) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_move(graph, 0, graph->height - dy - step, graph->width - step, dy, step, 0, GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	/* Move diagonal */
	for (i = 0, ay = 0; i < x; i += step, ay += sy) {
		sy = i * y / x;
		sy = (ay < sy) ? sy - ay : 0;
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_move(graph, step, step, graph->width - step, graph->height - step, -step, -sy, GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	/* Move right */
	for (i = 0; i < x; i += step) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_move(graph, 0, 0, graph->width - step, dy, step, 0, GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	/* Move diagonal to center */
	for (i = 0, ay = 0, x >>= 1, y >>= 1; i < x; i += step, ay += sy) {
		sy = i * y / x;
		sy = (ay < sy) ? sy - ay : 0;
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_move(graph, step, 0, graph->width - step, graph->height - step, -step, sy, GRAPH_QUEUE_HIGH)) < 0)
			return err;
	}

	/* Restore color map */
	if (graph->depth == 1)
		graph_colorset(graph, buff[0], 0, 1);

	return EOK;
}


int test_cursor(graph_t *graph)
{
	unsigned int i;
	int err;

	if ((err = graph_cursorset(graph, amask[0], xmask[0], 0xff000000, 0xffffffff)) < 0) {
		if (err != -ENOTSUP)
			return err;
		printf("test_graph: hardware cursor not supported. Skipping...\n");
		return EOK;
	}

	if ((err = graph_cursorshow(graph)) < 0)
		return err;

	for (i = 0; i < graph->height; i++) {
		if ((err = test_vtrigger(graph)) < 0)
			return err;
		if ((err = graph_cursorpos(graph, i * graph->width / graph->height, i)) < 0)
			return err;
	}

	if ((err = graph_cursorhide(graph)) < 0)
		return err;

	return EOK;
}


void test_help(const char *prog)
{
	printf("Usage: %s [adapter] [-m mode] [-f freq]\n", prog);
	printf("\tGraphics adapters:\n");
	printf("\t--cirrus     - use Cirrus Logic GD5446 VGA graphics adapter\n");
	printf("\t--virtio-gpu - use VirtIO GPU graphics adapter\n");
	printf("\t--vga        - use generic VGA adapter\n");
	printf("\tOther arguments:\n");
	printf("\t-m, --mode   - graphics mode index\n");
	printf("\t-f, --freq   - screen refresh rate index\n");
	printf("\t-h, --help   - prints this help message\n");
}


int main(int argc, char *argv[])
{
	int adapter = GRAPH_ANY;
	int mode = GRAPH_DEFMODE;
	int freq = GRAPH_DEFFREQ;
	struct option longopts[] = {
		{ "cirrus", no_argument, &adapter, GRAPH_CIRRUS },
		{ "virtio-gpu", no_argument, &adapter, GRAPH_VIRTIOGPU },
		{ "vga", no_argument, &adapter, GRAPH_VGA },
		{ "mode", required_argument, NULL, 'm' },
		{ "freq", required_argument, NULL, 'f' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};
	graph_t graph;
	int ret, c;

	while ((c = getopt_long(argc, argv, "m:f:h", longopts, NULL)) != -1) {
		switch (c) {
			case 0:
				/* Graphics adapter */
				break;

			case 'm':
				/* Skip default and power management modes */
				mode = atoi(optarg) + 5;
				break;

			case 'f':
				/* Skip default frequency */
				freq = atoi(optarg) + 1;
				break;

			case 'h':
			case '?':
			default:
				test_help(argv[0]);
				return EOK;
		}
	}

	if ((ret = graph_init()) < 0) {
		fprintf(stderr, "test_graph: failed to initialize library\n");
		return ret;
	}

	if ((ret = graph_open(&graph, adapter, 0x2000)) < 0) {
		fprintf(stderr, "test_graph: failed to initialize graphics adapter\n");
		graph_done();
		return ret;
	}

	do {
		if ((ret = graph_mode(&graph, mode, freq)) < 0) {
			fprintf(stderr, "test_graph: failed to set graphics mode\n");
			break;
		}
		printf("test_graph: starting test in %ux%ux%u graphics mode\n", graph.width, graph.height, graph.depth << 3);

		if ((ret = graph_rect(&graph, 0, 0, graph.width, graph.height, 0, GRAPH_QUEUE_HIGH)) < 0) {
			fprintf(stderr, "test_graph: failed to clear screen\n");
			break;
		}

		printf("test_graph: starting lines1 test...\n");
		if ((ret = test_lines1(&graph, 64, 64, 2)) < 0) {
			fprintf(stderr, "test_graph: lines1 test failed\n");
			break;
		}

		printf("test_graph: starting lines2 test...\n");
		if ((ret = test_lines2(&graph, 64, 64, 2)) < 0) {
			fprintf(stderr, "test_graph: lines2 test failed\n");
			break;
		}

		printf("test_graph: starting rectangles test...\n");
		if ((ret = test_rectangles(&graph, 32, 32, 2)) < 0) {
			fprintf(stderr, "test_graph: rectangles test failed\n");
			break;
		}

		printf("test_graph: starting logo test...\n");
		if ((ret = test_logo(&graph, 2)) < 0) {
			fprintf(stderr, "test_graph: logo test failed\n");
			break;
		}

		printf("test_graph: starting cursor test...\n");
		if ((ret = test_cursor(&graph)) < 0) {
			fprintf(stderr, "test_graph: cursor test failed\n");
			break;
		}
	} while (0);

	test_trigger(&graph);
	graph_close(&graph);
	graph_done();

	if (!ret)
		printf("test_graph: test finished successfully\n");

	return ret;
}
