/*
 * Phoenix-RTOS
 *
 * VirtIO devices tests
 *
 * Copyright 2020 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <errno.h>
#include <stdio.h>

#include <virtio.h>


/* VirtIO device descriptors */
static const virtio_devinfo_t info[] = {
	/* VirtIO network card devices */
	{ .type = vdevPCI, .id = 0x1000 },
	{ .type = vdevPCI, .id = 0x1041 },
	{ .type = vdevMMIO, .id = 0x01 },
	/* VirtIO block devices */
	{ .type = vdevPCI, .id = 0x1001 },
	{ .type = vdevPCI, .id = 0x1042 },
	{ .type = vdevMMIO, .id = 0x02 },
	/* VirtIO console devices */
	{ .type = vdevPCI, .id = 0x1003 },
	{ .type = vdevPCI, .id = 0x1043 },
	{ .type = vdevMMIO, .id = 0x03 },
	/* VirtIO entropy source devices */
	{ .type = vdevPCI, .id = 0x1005 },
	{ .type = vdevPCI, .id = 0x1044 },
	{ .type = vdevMMIO, .id = 0x04 },
	/* VirtIO memory ballooning devices (traditional) */
	{ .type = vdevPCI, .id = 0x1002 },
	{ .type = vdevPCI, .id = 0x1045 },
	{ .type = vdevMMIO, .id = 0x05 },
	/* VirtIO ioMemory devices */
	{ .type = vdevPCI, .id = 0x1046 },
	{ .type = vdevPCI, .id = 0x06 },
	/* VirtIO rpmsg devices */
	{ .type = vdevPCI, .id = 0x1047 },
	{ .type = vdevMMIO, .id = 0x07 },
	/* VirtIO SCSI host devices */
	{ .type = vdevPCI, .id = 0x1004 },
	{ .type = vdevPCI, .id = 0x1048 },
	{ .type = vdevMMIO, .id = 0x08 },
	/* VirtIO 9P transport devices */
	{ .type = vdevPCI, .id = 0x1009 },
	{ .type = vdevPCI, .id = 0x1049 },
	{ .type = vdevMMIO, .id = 0x09 },
	/* VirtIO mac80211 wlan devices */
	{ .type = vdevPCI, .id = 0x104a },
	{ .type = vdevMMIO, .id = 0x0a },
	/* VirtIO rproc serial devices */
	{ .type = vdevPCI, .id = 0x104b },
	{ .type = vdevMMIO, .id = 0x0b },
	/* VirtIO CAIF devices */
	{ .type = vdevPCI, .id = 0x104c },
	{ .type = vdevMMIO, .id = 0x0c },
	/* VirtIO memory balloon devices */
	{ .type = vdevPCI, .id = 0x104d },
	{ .type = vdevMMIO, .id = 0x0d },
	/* VirtIO GPU devices */
	{ .type = vdevPCI, .id = 0x1050 },
	{ .type = vdevMMIO, .id = 0x10 },
	/* VirtIO Timer/Clock devices */
	{ .type = vdevPCI, .id = 0x1051 },
	{ .type = vdevMMIO, .id = 0x11 },
	/* VirtIO input devices */
	{ .type = vdevPCI, .id = 0x1052 },
	{ .type = vdevMMIO, .id = 0x12 },
	/* VirtIO Socket devices */
	{ .type = vdevPCI, .id = 0x1053 },
	{ .type = vdevMMIO, .id = 0x13 },
	/* VirtIO Crypto devices */
	{ .type = vdevPCI, .id = 0x1054 },
	{ .type = vdevMMIO, .id = 0x14 },
	/* VirtIO Signal Distribution Module devices */
	{ .type = vdevPCI, .id = 0x1055 },
	{ .type = vdevMMIO, .id = 0x15 },
	/* VirtIO pstore devices */
	{ .type = vdevPCI, .id = 0x1056 },
	{ .type = vdevMMIO, .id = 0x16 },
	/* VirtIO IOMMU devices */
	{ .type = vdevPCI, .id = 0x1057 },
	{ .type = vdevMMIO, .id = 0x17 },
	/* VirtIO Memory devices */
	{ .type = vdevPCI, .id = 0x1058 },
	{ .type = vdevMMIO, .id = 0x18 },
#ifdef __TARGET_RISCV64
	/* Direct VirtIO MMIO QEMU descriptors */
	{ .type = vdevMMIO, .irq = 8, .base = { (void *)0x10008000, 0x1000 } },
	{ .type = vdevMMIO, .irq = 7, .base = { (void *)0x10007000, 0x1000 } },
	{ .type = vdevMMIO, .irq = 6, .base = { (void *)0x10006000, 0x1000 } },
	{ .type = vdevMMIO, .irq = 5, .base = { (void *)0x10005000, 0x1000 } },
	{ .type = vdevMMIO, .irq = 4, .base = { (void *)0x10004000, 0x1000 } },
	{ .type = vdevMMIO, .irq = 3, .base = { (void *)0x10003000, 0x1000 } },
	{ .type = vdevMMIO, .irq = 2, .base = { (void *)0x10002000, 0x1000 } },
	{ .type = vdevMMIO, .irq = 1, .base = { (void *)0x10001000, 0x1000 } },
#endif
	{ .type = vdevNONE }
};


static const char *name[] = {
	"network card",
	"block",
	"console",
	"entropy source",
	"memory ballooning (traditional)",
	"ioMemory",
	"rpmsg",
	"SCSI host",
	"9P transport",
	"mac80211 wlan",
	"rproc serial",
	"CAIF",
	"memory balloon",
	"GPU",
	"Timer/Clock",
	"input",
	"Socket",
	"Crypto",
	"Signal Distribution Module",
	"pstore",
	"IOMMU",
	"Memory"
};


static const char *test_virtio_name(virtio_dev_t *vdev, char *buff)
{
	unsigned int id = vdev->info.id;

	/* Convert legacy VirtIO PCI device ID */
	if ((id >= 0x1000) && (id <= 0x1040)) {
		if (id == 0x1000)
			id = 0x01;
		else if (id == 0x1001)
			id = 0x02;
		else if (id == 0x1002)
			id = 0x05;
		else if (id == 0x1003)
			id = 0x03;
		else if (id == 0x1004)
			id = 0x08;
		else if (id == 0x1005)
			id = 0x04;
		else if (id == 0x1009)
			id = 0x09;
		else
			id = 0x00;
	}
	/* Convert modern VirtIO PCI device ID */
	else if (id > 0x1040) {
		id -= 0x1040;
	}

	if (!id || (id > 0x0d && id < 0x10) || (id > 0x18)) {
		sprintf(buff, "unknown VirtIO device");
		return buff;
	}

	if (id > 0x0d)
		id -= 2;

	sprintf(buff, "%s VirtIO %s %s device (%#x)", virtio_legacy(vdev) ? "legacy" : "modern", (vdev->info.type == vdevPCI) ? "PCI" : "MMIO", name[id - 1], vdev->info.id);
	return buff;
};


/* Detects and initializes all VirtIO devices in the system */
static void test_virtio_init(void)
{
	virtio_dev_t vdev;
	virtio_ctx_t vctx;
	char buff[64];
	void *base;
	int err, i;

	virtio_init();

	printf("test_virtio: searching for VirtIO devices...\n");
	for (i = 0; info[i].type != vdevNONE; i++) {
		vctx.reset = 1;
		while ((err = virtio_find(&info[i], &vdev, &vctx)) != -ENODEV) {
			if (err < 0) {
				printf("test_virtio: failed to process VirtIO %s ", (info[i].type == vdevPCI) ? "PCI" : "MMIO");
				if (info[i].base.len)
					printf("direct descriptor, base: %#x. ", info[i].base.addr);
				else
					printf("descriptor, ID: %#x. ", info[i].id);
				printf("Skipping...\n");
				continue;
			}

			base = vdev.info.base.addr;
			err = virtio_initDev(&vdev);
			test_virtio_name(&vdev, buff);

			if (err < 0) {
				if (err != -ENODEV)
					printf("test_virtio: failed to init %s, base: %#x\n", buff, base);
				continue;
			}

			printf("test_virtio: found %s, base: %#x\n", buff, base);
			virtio_destroyDev(&vdev);
		}
	}

	virtio_done();
}


int main(void)
{
	printf("test_virtio: starting, main is at %p\n", main);

	test_virtio_init();

	return EOK;
}
