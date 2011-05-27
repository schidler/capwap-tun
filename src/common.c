#include "common.h"
//#include "CWProtocol.h"

int get_tap_interface(char *ifname)
{
    int fd;
    struct ifreq ifr;

    /* Get file descriptor */
    if ((fd = open(TUN_CTL_DEV, O_RDWR)) < 0) {
        dbg_printf("Can't open " TUN_CTL_DEV "\n");
        return -1;
    }

    /* Create TAP interface */
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP|IFF_NO_PI;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        dbg_printf("Can't set up TUN/TAP.\n");
	return -1;
    }
    strcpy(ifname, ifr.ifr_name);
    return fd;
}

void revmoe_tap_interface(int fd)
{
    if (fd > 0)
        close(fd);
}

int add_tap_to_bridge(char *ifname, char *br)
{
    int fd;
    struct ifreq ifr;

    /* Add into bridge */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, br, IFNAMSIZ);
    if ((ifr.ifr_ifindex = if_nametoindex(ifname)) == 0) {
        dbg_printf("Can't get index of interface %s.\n", ifname);
        close(fd);
        return -1;
    }
    if (ioctl(fd, SIOCBRADDIF, &ifr) < 0) {
        dbg_printf("Can't get add interface %s into bridge %s.\n", ifname, br);
        close(fd);
        return -1;
    }

    /* Bring tap interface up */
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        dbg_printf("Can't get flags of %s.\n", ifname);
        close(fd);
        return -1;
    }
    ifr.ifr_flags |= IFF_UP;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        dbg_printf("Can't get flags of %s.\n", ifname);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

void remove_from_bridge(char *ifname, char *br)
{
    int fd;
    struct ifreq ifr;

    /* Remove bridge member interface */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, br, IFNAMSIZ);
    ifr.ifr_ifindex = if_nametoindex(ifname);
    ioctl(fd, SIOCBRADDIF, &ifr);
    close(fd);
}

int add_to_event_loop(struct tun_info *info, void (*cb)(int, short, void*))
{
    /* Add into event loop */
    event_set(&info->tun_ev, info->tun_fd, EV_READ|EV_PERSIST, cb, info);
    event_add(&info->tun_ev, NULL);
}

void remove_from_event_loop(struct tun_info *info)
{
    /* Clean event */
    if (event_initialized(&info->tun_ev) && 
            event_pending(&info->tun_ev, EV_READ, NULL))
        event_del(&info->tun_ev);
}

char *get_sockaddr_host(struct sockaddr *addr, size_t addrlen, char *buf)
{
    int ret;
    
    ret = getnameinfo(addr, addrlen, buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    if (ret) {
	fprintf(stderr, "Can't get host from sockaddr.\n");
	return NULL;
    }
    return buf;
}

int sockaddr_host_equal(struct sockaddr *src_addr, size_t src_addrlen,
	struct sockaddr *dst_addr, size_t dst_addrlen)
{
    char src_host[NI_MAXHOST], dst_host[NI_MAXHOST];
    int ret;

    if (src_addrlen != dst_addrlen)
	return 0;

    if (get_sockaddr_host(src_addr, src_addrlen, src_host) &&
	    get_sockaddr_host(dst_addr, dst_addrlen, dst_host) &&
	    strcmp(src_host, dst_host) == 0)
	return 1;
    return 0;
}

