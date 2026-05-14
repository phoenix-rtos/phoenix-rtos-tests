#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/threads.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <stdint.h>
#include <phoenix/file.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <posix/utils.h>
#include <time.h>


#include "shmsrv.h"


static struct {
	size_t size;
	void *buf;
} shmsrv_common;


int shmsrv_start(char *path)
{
	oid_t oid;
	uint32_t port;
	if (portCreate(&port) != 0) {
		fprintf(stderr, "portCreate");
		return 1;
	}

	oid.port = port;
	oid.id = 0;
	if (create_dev(&oid, path) != 0) {
		fprintf(stderr, "create_dev");
		return 1;
	}

	shmsrv_common.size = 0;
	shmsrv_common.buf = NULL;

	msg_t msg = { 0 };
	msg_rid_t rid;

	size_t size;

	while (1) {
		msgRecv(port, &msg, &rid);
		switch (msg.type) {
			case mtOpen:
			case mtClose: {
				msg.o.err = EOK;
				break;
			}
			case mtRead: {
				size = msg.o.size;
				if (msg.i.io.offs > shmsrv_common.size) {
					msg.o.err = -EINVAL;
					break;
				}
				if (msg.i.io.offs + size >= shmsrv_common.size) {
					size = shmsrv_common.size - msg.i.io.offs;
				}

				memcpy(msg.o.data, (char *)shmsrv_common.buf + msg.i.io.offs, size);
				msg.o.err = size;
				break;
			}
			case mtWrite: {
				size = msg.o.size;
				if (msg.i.io.offs > shmsrv_common.size) {
					msg.o.err = -EINVAL;
					break;
				}
				if (msg.i.io.offs + size >= shmsrv_common.size) {
					size = shmsrv_common.size - msg.i.io.offs;
				}
				memcpy((char *)shmsrv_common.buf + msg.i.io.offs, msg.i.data, size);
				msg.o.err = size;
				break;
			}
			case mtTruncate: {
				shmsrv_common.size = msg.i.io.len;
				shmsrv_common.buf = realloc(shmsrv_common.buf, shmsrv_common.size);
				if (shmsrv_common.buf == NULL) {
					msg.o.err = -ENOMEM;
					break;
				}
				msg.o.err = EOK;
				break;
			}
			case mtGetAttr: {
				switch (msg.i.attr.type) {
					case atSize:
						msg.o.attr.val = shmsrv_common.size;
						msg.o.err = EOK;
						break;
					default:
						msg.o.err = -ENOSYS;
						break;
				}
				break;
			}
			case mtGetAttrAll: {
				struct _attrAll *attrs = msg.o.data;
				if ((attrs == NULL) || (msg.o.size < sizeof(struct _attrAll))) {
					msg.o.err = -EINVAL;
				}
				else {
					memset(attrs, 0, sizeof(struct _attrAll));
					attrs->size.val = shmsrv_common.size;
					attrs->size.err = EOK;
					msg.o.err = EOK;
				}
				break;
			}
			default:
				msg.o.err = -ENOSYS;
				break;
		}
		msgRespond(port, &msg, rid);
	}
}
