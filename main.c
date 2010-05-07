#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

void getifaces(int fd) {
    struct {
        struct nlmsghdr hdr;
        struct rtgenmsg g;
    } req;

    uint8_t * buf = 0;
    size_t buflen = getpagesize();
    struct sockaddr_nl sa;
    socklen_t salen = sizeof(sa);
    uint32_t rcvd = 0;
    struct nlmsghdr * rhdr = NULL;

    memset(&req, 0, sizeof(req));
    req.hdr.nlmsg_type = RTM_GETLINK;
    req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP; 
    req.hdr.nlmsg_seq = 0;
    req.hdr.nlmsg_pid = getpid();
    req.hdr.nlmsg_len = sizeof(req);
    req.g.rtgen_family = AF_UNSPEC;
    if (send(fd, &req, sizeof(req), 0) == -1) {
        perror("send");
        return;
    }

    buf = malloc(buflen);
    memset(buf, 0, buflen);
    rcvd = recvfrom(fd, buf, buflen, 0, (struct sockaddr *) &sa, &salen);
    if (rcvd == -1) {
        perror("recvfrom");
        return;
    }
    printf("rcvd: %d family: %d pid: %d\n", rcvd, sa.nl_family, sa.nl_pid); 

    rhdr = (struct nlmsghdr *) buf;

    while(NLMSG_OK(rhdr, rcvd)) {
        struct ifinfomsg * ifi = NULL;
        struct rtattr * rta = NULL;
        uint32_t rtalen = 0;
        uint32_t msg_len = 0; 
        
        printf("type: %d len: %d flags: 0x%x\n", rhdr->nlmsg_type, rhdr->nlmsg_len, rhdr->nlmsg_flags);
        if (rhdr->nlmsg_type == RTM_GETLINK) 
            printf("getlink\n");
        if (rhdr->nlmsg_flags & NLM_F_MULTI) {
            printf("multipart message\n");
        }
        
        msg_len = rhdr->nlmsg_len;
        ifi = NLMSG_DATA(rhdr);

        printf("iface(%d), type: %d flags: 0x%x\n", ifi->ifi_index, ifi->ifi_type, ifi->ifi_flags);

        rta = NLMSG_DATA(rhdr) + sizeof(struct ifinfomsg);
        rtalen = msg_len - sizeof(struct ifinfomsg) - sizeof(struct nlmsghdr);
        printf("rta attributes, len: %d\n", rtalen);
        while(1) {
            if (!RTA_OK(rta, rtalen)) {
                printf("last attr\n");
                break;
            }
            printf("type: %d(0x%x) len: %d\n", rta->rta_type, rta->rta_type, rta->rta_len);
            switch (rta->rta_type) {
                case IFLA_IFNAME:
                    printf("name: %s\n", (char *) RTA_DATA(rta));
                    break;
                default: 
                    break;
            }
            rta = RTA_NEXT(rta, rtalen);
            printf("rta buf left: %d\n", rtalen);
        }

        rhdr = NLMSG_NEXT(rhdr, rcvd);
        printf("data left: %d\n", rcvd);
    }
}
int main(void) {
    int fd = 0;
    struct sockaddr_nl sa;
    
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;
    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("bind");
        goto end_close;
    }

    getifaces(fd);
end_close:
    close(fd);
    return 0;
}
