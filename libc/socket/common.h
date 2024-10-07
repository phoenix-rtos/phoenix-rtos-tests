/*
 * Phoenix-RTOS
 *
 * test-libc-socket
 *
 * common part of socket tests
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Ziemowit Leszczynski, Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TEST_SOCKET_COMMON_H
#define _TEST_SOCKET_COMMON_H

#define MAX_FD_CNT         16
#define CLOSE_LOOP_CNT     50
#define SENDMSG_LOOP_CNT   50
#define FORK_LOOP_CNT      50
#define MAX_TRANSFER_CNT   (1024 * 16)
#define TRANSFER_LOOP_CNT  50
#define CONNECTED_LOOP_CNT 10
#ifdef __phoenix__
#define DATA_SIZE _PAGE_SIZE - sizeof(ssize_t)
#else
#define DATA_SIZE 10000
#endif

ssize_t msg_send(int sock, void *buf, size_t len, int *fd, size_t fdcnt);

ssize_t msg_recv(int sock, void *buf, size_t len, int *fd, size_t *fdcnt);

int set_nonblock(int fd, int enable);

int open_files(int *fd, size_t cnt);

int close_files(int *fd, size_t cnt);

int write_files(int *fd, size_t cnt, char *data);

int read_files(int *fd, size_t cnt, char *data, char *buf);

int stat_files(int *fd, size_t cnt, int exists);

int createTmpIfMissing(int *isMissing);

#endif /* TEST_SOCKET_COMMON_H */
