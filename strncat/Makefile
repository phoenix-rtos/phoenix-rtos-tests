CC = gcc
LDFLAGS = -Wl,--wrap=strlen

UNITY_ROOT = ../unity
TEST_NAME = test-strncat
TEST_PATH = ../../_fs/host-generic-pc/root/bin/$(TEST_NAME)
TEST_SRC = test_strncat.c
UNITY_SRC = $(UNITY_ROOT)/unity.c \
            $(UNITY_ROOT)/unity_fixture.c

.PHONY: all clean test

all: $(TEST_PATH)
test: $(TEST_PATH)

$(TEST_PATH): $(TEST_SRC) $(UNITY_SRC)
	$(CC) -I$(UNITY_ROOT) $(LDFLAGS) $^ -o $@


clean:
	rm -f $(TEST_PATH)
