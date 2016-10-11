/*
 *	macaddr
 *
 *	Program to return the MAC address of an Ethernet
 *	adapter.  This was written to help configure the
 *	adapter based on the MAC address rather than the
 *	name.
 *
 *	Version 1.0	Eric Dittman	2001-10-19
 *
 *	This is released unther the GPL license.
 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <linux/if.h>

int main(int argc, char** argv) {

	int devsock;
	struct ifreq ifbuffer;
	int i;

	if (argc != 2) {
		printf("Usage: macaddr interface\n");
		exit(1);
	}

	devsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (devsock == -1) {
		printf("Failed opening socket\n");
		exit (1);
	}

	memset(&ifbuffer, 0, sizeof(ifbuffer));
	strcpy(ifbuffer.ifr_name, argv[1]);
	if (ioctl(devsock, SIOCGIFHWADDR, &ifbuffer) == -1) {
		printf("There is no MACADDR for %s\n", argv[1]);
		exit(1);
	}
	close (devsock);

	for (i = 0; i < IFHWADDRLEN; i++)
		printf("%02X", (unsigned char) ifbuffer.ifr_ifru.ifru_hwaddr.sa_data[i]);
	printf("\n");

	exit(0);

}
