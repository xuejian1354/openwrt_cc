 #if defined(SW_PLATFORM_YOUHUA)

 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <ifaddrs.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <linux/if_link.h>

// https://stackoverflow.com/questions/4951257/using-c-code-to-get-same-info-as-ifconfig

 int get_interface_flow_by_api(const char* ifname, unsigned long *rx_bytes, unsigned long *rx_packets)
 {
     struct ifaddrs *ifaddr, *ifa;
     int family, s, n;

     *rx_bytes = 0;
     *rx_packets = 0;

     if (getifaddrs(&ifaddr) == -1) {
         perror("getifaddrs");
     }

     /* Walk through linked list, maintaining head pointer so we
        can free list later */

     for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
         if (ifa->ifa_addr == NULL)
             continue;

         family = ifa->ifa_addr->sa_family;

         /* Display interface name and family (including symbolic
            form of the latter for the common families) */

         if(strcmp(ifname, ifa->ifa_name) != 0)
          continue;

         printf("%-8s %s (%d)\n",
                ifa->ifa_name,
                (family == AF_PACKET) ? "AF_PACKET" :
                (family == AF_INET) ? "AF_INET" :
                (family == AF_INET6) ? "AF_INET6" : "???",
                family);

         /* For an AF_INET* interface address, display the address */

         if (family == AF_PACKET && ifa->ifa_data != NULL) {
             struct rtnl_link_stats *stats = (struct rtnl_link_stats *)ifa->ifa_data;

             printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
                    "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
                    stats->tx_packets, stats->rx_packets,
                    stats->tx_bytes, stats->rx_bytes);
             *rx_bytes = stats->rx_bytes;
             *rx_packets = stats->rx_packets;
             break;
         }
     }

     freeifaddrs(ifaddr);
     return 0;
 }
 
#else
int get_interface_flow_by_api(const char* ifname, unsigned long *rx_bytes, unsigned long *rx_packets)
{
   return 0;
}
#endif
