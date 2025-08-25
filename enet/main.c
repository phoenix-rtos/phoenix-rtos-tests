#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <phoenix/ethtool.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/minmax.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "unity.h"
#include "unity_fixture.h"

#define _TP_DST          "\xff\xff\xff\xff\xff\xff"
#define _TP_SRC          "ssssss" /* overridden in setup */
#define _TP_ETHTYPE      "tt"     /* overridden in setup */
#define _TP_10DIG        "0123456789"
#define TEST_HEADER      _TP_DST _TP_SRC _TP_ETHTYPE
#define TEST_HEADER_LEN  (sizeof((TEST_HEADER)) - 1)
#define TEST_PAYLOAD     _TP_10DIG _TP_10DIG _TP_10DIG _TP_10DIG _TP_10DIG _TP_10DIG _TP_10DIG
#define TEST_PAYLOAD_LEN (sizeof((TEST_PAYLOAD)) - 1)
#define TEST_PACKET      TEST_HEADER TEST_PAYLOAD
#define TEST_PACKET_LEN  (sizeof((TEST_PACKET)) - 1)

#define ETHERTYPE       (0x0800)
#define MAX_PAYLOAD_LEN (1500)
#define MAX_BUF_LEN     (ETH_ALEN + ETH_ALEN + 2 + 4 + MAX_PAYLOAD_LEN) /* dst + src + ethertype + crc + MTU */


typedef union {
	struct {
		struct ether_header header;
		char payload[MAX_PAYLOAD_LEN];
	} __attribute__((packed));
	char raw_buf[MAX_BUF_LEN];
} ether_frame_t;


static char ifname[IFNAMSIZ];
static int ctrl_sock, send_sock, recv_sock;
static char *dynamic_buf;
static struct ifreq ifr;
static ether_frame_t send_frame;
static ether_frame_t recv_frame;
static struct sockaddr self_hwaddr;
static struct sockaddr_ll src_addr = {
	.sll_family = AF_PACKET,
	.sll_protocol = htons(ETH_P_ALL),
};
static struct sockaddr_ll dst_addr = {
	.sll_family = AF_PACKET,
	.sll_protocol = htons(ETH_P_ALL),
};


static inline void setup_send_frame_header(const char *src)
{
	memcpy(send_frame.header.ether_dhost, _TP_DST, ETH_ALEN);
	memcpy(send_frame.header.ether_shost, src, ETH_ALEN);
	send_frame.header.ether_type = htons(ETHERTYPE);
}


static inline void make_rand_frame(char *buf, size_t buf_sz)
{
	for (size_t i = 0; i < buf_sz; i++) {
		buf[i] = rand() & 0x7f;
	}
}


static inline uint64_t now_us(void)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec * 1000 * 1000) + (now.tv_nsec / 1000);
}


static inline void set_iface_loopback(int fd, struct ifreq *ifr, bool enable)
{
	int err;
	struct ethtool_value loopback = {
		.cmd = ETHTOOL_SLOOPBACK,
		.data = enable,
	};
	ifr->ifr_data = (char *)&loopback;

	err = ioctl(fd, SIOCETHTOOL, ifr);
	TEST_ASSERT_EQUAL_MESSAGE(0, err, strerror(errno));

	loopback.cmd = ETHTOOL_GLOOPBACK;
	loopback.data = -1;
	err = ioctl(fd, SIOCETHTOOL, ifr);
	TEST_ASSERT_EQUAL_MESSAGE(0, err, strerror(errno));
	TEST_ASSERT_EQUAL_MESSAGE(enable, loopback.data, "loopback was not set");
}


static inline int setup_socket(const struct sockaddr_ll *addr, const char *dbg_name)
{
	int s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (s < 0) {
		perror(dbg_name);
		TEST_FAIL_MESSAGE("socket creation failed");
	}

	if (addr != NULL) {
		if (bind(s, (const struct sockaddr *)addr, sizeof(*addr)) < 0) {
			perror(dbg_name);
			TEST_FAIL_MESSAGE("bind failed");
		}
	}

	return s;
}


TEST_GROUP(enet);

TEST_SETUP(enet)
{
	int err;

	srand(time(NULL));
	ctrl_sock = setup_socket(NULL, "ctrl socket");

	memcpy(ifr.ifr_name, ifname, IFNAMSIZ);

	err = ioctl(ctrl_sock, SIOCGIFFLAGS, &ifr);
	TEST_ASSERT_EQUAL_MESSAGE(0, err, strerror(errno));

	ifr.ifr_flags |= IFF_PROMISC | IFF_UP | IFF_RUNNING;
	err = ioctl(ctrl_sock, SIOCSIFFLAGS, &ifr);
	TEST_ASSERT_EQUAL_MESSAGE(0, err, strerror(errno));

	err = ioctl(ctrl_sock, SIOCGIFHWADDR, &ifr);
	TEST_ASSERT_EQUAL_MESSAGE(0, err, strerror(errno));
	self_hwaddr = ifr.ifr_hwaddr;

	memcpy(dst_addr.sll_addr, _TP_DST, ETH_ALEN);
	memcpy(src_addr.sll_addr, self_hwaddr.sa_data, ETH_ALEN);

	err = ioctl(ctrl_sock, SIOCGIFINDEX, &ifr);
	TEST_ASSERT_EQUAL_MESSAGE(0, err, strerror(errno));
	dst_addr.sll_ifindex = ifr.ifr_ifindex;
	src_addr.sll_ifindex = ifr.ifr_ifindex;

	setup_send_frame_header(self_hwaddr.sa_data);
}

TEST_TEAR_DOWN(enet)
{
	if (send_sock > 0) {
		close(send_sock);
		send_sock = 0;
	}

	if (recv_sock > 0) {
		close(recv_sock);
		recv_sock = 0;
	}

	if (ctrl_sock > 0) {
		set_iface_loopback(ctrl_sock, &ifr, 0);
		close(ctrl_sock);
		ctrl_sock = 0;
	}

	if (dynamic_buf != NULL) {
		free(dynamic_buf);
		dynamic_buf = NULL;
	}
}

TEST(enet, selftest)
{
	int err;
	struct ethtool_test test_config = {
		.cmd = ETHTOOL_TEST,
		.flags = ETH_TEST_FL_OFFLINE,
		.len = 0,
	};

	ifr.ifr_data = (char *)&test_config;
	err = ioctl(ctrl_sock, SIOCETHTOOL, &ifr);
	if (err < 0 && errno == EOPNOTSUPP) {
		TEST_IGNORE_MESSAGE("selftest not supported");
	}
	TEST_ASSERT_EQUAL_MESSAGE(0, err, strerror(errno));

	TEST_ASSERT_EQUAL(0, test_config.flags & ETH_TEST_FL_FAILED);
	TEST_ASSERT_NOT_EQUAL(0, test_config.flags & ETH_TEST_FL_OFFLINE);
}

TEST(enet, one_packet)
{
	set_iface_loopback(ctrl_sock, &ifr, 1);

	ssize_t result;
	struct sockaddr_ll from_addr;
	socklen_t from_addr_len = sizeof(from_addr);

	send_sock = setup_socket(&src_addr, "send socket");
	recv_sock = setup_socket(&src_addr, "recv socket");

	memcpy(send_frame.payload, TEST_PAYLOAD, TEST_PAYLOAD_LEN);

	result = sendto(send_sock, send_frame.raw_buf, TEST_PACKET_LEN, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr));
	TEST_ASSERT_EQUAL_MESSAGE(TEST_PACKET_LEN, result, strerror(errno));

	result = recvfrom(recv_sock, recv_frame.raw_buf, TEST_PACKET_LEN, 0, (struct sockaddr *)&from_addr, &from_addr_len);
	TEST_ASSERT_EQUAL_MESSAGE(TEST_PACKET_LEN, result, result < 0 ? strerror(errno) : "length mismatch");
	TEST_ASSERT_EQUAL(sizeof(src_addr), from_addr_len);
	TEST_ASSERT_EQUAL_MEMORY(src_addr.sll_addr, from_addr.sll_addr, ETH_ALEN);
	TEST_ASSERT_EQUAL_MEMORY(send_frame.raw_buf, recv_frame.raw_buf, TEST_PACKET_LEN);
}


TEST(enet, load)
{
	set_iface_loopback(ctrl_sock, &ifr, 1);

	const size_t payload_size = 1024;
	const size_t total_bytes = 10 * 1024 * 1024; /* 10 MB */
	ssize_t left = total_bytes;
	ssize_t result;
	uint64_t start_us, end_us, elapsed_us;

	send_sock = setup_socket(&src_addr, "send socket");
	recv_sock = setup_socket(&src_addr, "recv socket");

	make_rand_frame(send_frame.payload, payload_size);

	start_us = now_us();
	while (left > 0) {
		const size_t current_payload_size = min(payload_size, left);
		const size_t current_frame_size = current_payload_size + sizeof(struct ether_header);

		result = sendto(send_sock, send_frame.raw_buf, current_frame_size, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr));
		TEST_ASSERT_EQUAL_MESSAGE(current_frame_size, result, strerror(errno));

		left -= current_payload_size;

		result = recv(recv_sock, recv_frame.raw_buf, current_frame_size, 0);
		TEST_ASSERT_EQUAL_MESSAGE(current_frame_size, result, strerror(errno));

		TEST_ASSERT_EQUAL_MEMORY(send_frame.raw_buf, recv_frame.raw_buf, current_frame_size);
	}
	end_us = now_us();

	elapsed_us = end_us - start_us;
	float elapsed_s = (float)elapsed_us / (1000 * 1000);
	float mb_per_s = ((float)(total_bytes * 8) / (1024 * 1024)) / elapsed_s;
	printf("test_load: %zuB transferred in ~%.3fs => ~%.3f Mb/s\n", total_bytes, elapsed_s, mb_per_s);
}

TEST(enet, more_data_than_mtu)
{
	set_iface_loopback(ctrl_sock, &ifr, 1);

	const size_t bufsz = 4096;

	dynamic_buf = malloc(bufsz);
	TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, dynamic_buf, "malloc");

	struct ether_header *header = (void *)dynamic_buf;
	memcpy(header->ether_dhost, _TP_DST, ETH_ALEN);
	memcpy(header->ether_shost, self_hwaddr.sa_data, ETH_ALEN);
	header->ether_type = htons(ETHERTYPE);
	make_rand_frame(dynamic_buf + sizeof(struct ether_header), bufsz - sizeof(struct ether_header));

	send_sock = setup_socket(&src_addr, "send_socket");

	TEST_ASSERT_EQUAL(-1, sendto(send_sock, dynamic_buf, bufsz, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr)));
	TEST_ASSERT_EQUAL(ENOBUFS, errno);
}

TEST_GROUP_RUNNER(enet)
{
	RUN_TEST_CASE(enet, selftest);
	RUN_TEST_CASE(enet, one_packet);
	RUN_TEST_CASE(enet, load);
	RUN_TEST_CASE(enet, more_data_than_mtu);
}

static void runner(void)
{
	RUN_TEST_GROUP(enet);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: test-enet [interface]\n");
		return 1;
	}
	size_t len = strlen(argv[1]);
	if (len >= IFNAMSIZ) {
		fprintf(stderr, "Error: interface name too long\n");
		return 1;
	}
	memcpy(ifname, argv[1], len);
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
