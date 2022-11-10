#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

int get_ip(char *devname, char *ipaddr)
{
	struct ifreq ifr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	strcpy(ifr.ifr_name, devname);
	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
	{
		close(fd);
		return -1;
	}
	char *pIP = inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr);
	if (pIP)
	{
		strcpy(ipaddr, pIP);
		close(fd);
		return 0;
	}
	return -1;
}

int get_ip_auto(char *ipaddr)
{
	int ret = get_ip("eth0", ipaddr);
	if (ret == 0)
	{
		return ret;
	}

	ret = get_ip("wlan0", ipaddr);
	if (ret == 0)
	{
		return ret;
	}

	ret = get_ip("usb0", ipaddr);
	if (ret == 0)
	{
		return ret;
	}
	return ret;
}