#include "unity.h"
#include "unity_fixture.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <graph.h>

graph_t mock_graph;

#define MOCK_WIDTH 800
#define MOCK_HEIGHT 600
static uint32_t mock_framebuffer[MOCK_WIDTH * MOCK_HEIGHT];
typedef struct {
    unsigned int callsCnt;
    unsigned int x;
	unsigned int y;
	unsigned int dx;
	unsigned int dy;
    unsigned int stroke;
	unsigned int color;
} graph_line_mock_t;

static graph_line_mock_t graph_line_mock;
static bool graph_busy = false;

static int mock_graph_line(graph_t *graph, unsigned int x, unsigned int y, int dx, int dy, unsigned int stroke, unsigned int color) 
{
    graph_line_mock.callsCnt++;
    graph_line_mock.x = x;
    graph_line_mock.y = y;
    graph_line_mock.dx = dx;
    graph_line_mock.dy = dy;
    graph_line_mock.stroke = stroke;
    graph_line_mock.color = color;

    return 0;
}

static int mock_graph_isbusy(graph_t *graph) 
{
    return graph_busy;
}


TEST_GROUP(libgraph_line);

TEST_SETUP(libgraph_line)
{
    memset(&mock_graph, 0, sizeof(graph_t));
    memset(&graph_line_mock, 0, sizeof(graph_line_mock_t));

    mock_graph.width = MOCK_WIDTH;
    mock_graph.height = MOCK_HEIGHT;
    mock_graph.depth = 32;
    mock_graph.data = mock_framebuffer;

    memset(mock_framebuffer, 0, sizeof(mock_framebuffer));
    
    mock_graph.line = mock_graph_line;
    mock_graph.isbusy = mock_graph_isbusy;
}


TEST_TEAR_DOWN(libgraph_line)
{
}


TEST(libgraph_line, queue_high_correct)
{
    int ret = 1;
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int dx = mock_graph.width;
    unsigned int dy = mock_graph.height;
    unsigned int stroke = UINT_MAX;
    unsigned int color = UINT_MAX;

    ret = graph_line(&mock_graph, x, y, dx, dy, stroke, color, GRAPH_QUEUE_HIGH);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT(1, graph_line_mock.callsCnt);
    TEST_ASSERT_EQUAL_UINT(x, graph_line_mock.x);
    TEST_ASSERT_EQUAL_UINT(y, graph_line_mock.y);
    TEST_ASSERT_EQUAL_UINT(dx, graph_line_mock.dx);
    TEST_ASSERT_EQUAL_UINT(dy, graph_line_mock.dy);
    TEST_ASSERT_EQUAL_UINT(stroke, graph_line_mock.stroke);
    TEST_ASSERT_EQUAL_UINT(color, graph_line_mock.color);

    x = mock_graph.width;
    y = mock_graph.height;
    dx = 0;
    dy = 0;
    stroke = 0;
    color = 0;

    graph_stop(&mock_graph, GRAPH_QUEUE_LOW);

    ret = graph_line(&mock_graph, x, y, dx, dy, stroke, color, GRAPH_QUEUE_HIGH);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT(2, graph_line_mock.callsCnt);
    TEST_ASSERT_EQUAL_UINT(x, graph_line_mock.x);
    TEST_ASSERT_EQUAL_UINT(y, graph_line_mock.y);
    TEST_ASSERT_EQUAL_UINT(dx, graph_line_mock.dx);
    TEST_ASSERT_EQUAL_UINT(dy, graph_line_mock.dy);
    TEST_ASSERT_EQUAL_UINT(stroke, graph_line_mock.stroke);
    TEST_ASSERT_EQUAL_UINT(color, graph_line_mock.color);
}


TEST(libgraph_line, queue_high_busy)
{
    int ret = 1;
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int dx = 0;
    unsigned int dy = 0;
    unsigned int stroke = 0;
    unsigned int color = 0;

    graph_busy = true;

    ret = graph_line(&mock_graph, x, y, dx, dy, stroke, color, GRAPH_QUEUE_HIGH);
    TEST_ASSERT_EQUAL(-ENOSPC, ret);
    TEST_ASSERT_EQUAL(0, graph_line_mock.callsCnt);
}


TEST(libgraph_line, queue_high_stop)
{
    int ret = 1;
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int dx = 0;
    unsigned int dy = 0;
    unsigned int stroke = 0;
    unsigned int color = 0;

    graph_stop(&mock_graph, GRAPH_QUEUE_HIGH);

    ret = graph_line(&mock_graph, x, y, dx, dy, stroke, color, GRAPH_QUEUE_HIGH);
    TEST_ASSERT_EQUAL(-EACCES, ret);
    TEST_ASSERT_EQUAL(0, graph_line_mock.callsCnt);
}


TEST_GROUP_RUNNER(libgraph_line)
{
    RUN_TEST_CASE(libgraph_line, queue_high_correct);
    RUN_TEST_CASE(libgraph_line, queue_high_busy);
    RUN_TEST_CASE(libgraph_line, queue_high_stop);
}

static void runner(void)
{
    RUN_TEST_GROUP(libgraph_line);
}

int main(int argc, const char* argv[])
{
    return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
