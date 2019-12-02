#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>

char* run_cmd(const char* str_cmd, int max_lines, int max_buffer_len);

typedef struct {
    char name[64];
    unsigned long rx_bytes;
    unsigned long rx_packets;
}if_stats_t;
static if_stats_t if_stats_begin[64];
static if_stats_t if_stats_end[64];

typedef struct {
    char name[64];
    unsigned long rx_bytes;
    unsigned long rx_packets;
    int bps;
    int pps;
    int active_score;    //0
    int wan_type;    //0, loopback, 1, br0, 2, eth0, eth1, 
    int avg_packet_size;    // as 1460
    int similar_count;   // 

}if_bps_stats_t;
static if_bps_stats_t if_bps_stats[64];
static int order[64];

static int line_parsing(char *data_text, if_stats_t if_stats[64])
{
  char delims[] = "\n\r";
  char *result = NULL;
  int i=0;

  result = strtok( data_text, delims );
  while( result != NULL  && i < 64) {
     sscanf(result, "%s %lu %lu", if_stats[i].name, &if_stats[i].rx_bytes, &if_stats[i].rx_packets);
     result = strtok( NULL, delims );
     i++;
  }
  return i;
}

static int get_interface_flow_by_procfs(if_stats_t if_stats[64])
{
    char* pdata=NULL;
    char cmd_buffer[256];
    sprintf(cmd_buffer, "cat /proc/net/dev | awk '/:/ { print($1, $2, $3) }'");
    int count = 0;
    pdata = run_cmd(cmd_buffer, 1000, 10*1024);
    if(pdata){
        count = line_parsing(pdata, if_stats);
        sscanf(pdata, "%s %lu %lu", if_stats[0].name, &if_stats[0].rx_bytes, &if_stats[0].rx_packets);
        free(pdata);
    }
    
    return count;   
}

int get_interface_flow_by_procfs_1st(){
    return get_interface_flow_by_procfs(if_stats_begin);
}

int get_interface_flow_by_procfs_2nd(){
    return get_interface_flow_by_procfs(if_stats_end);
}

static int compute_packets(unsigned long bandwidth_mbps, int time_seconds, int packetBytes, int realPacketBytes);
static int diff_percent(unsigned long a, unsigned long b);
static int match_general_pon_port(char* ifname);
static int match_vendor_pon_port(const char*vendor, char* ifname);


#define diff_ulong(x, y) x >= y ? (x - y) : (ULONG_MAX - y + x)
static char policy_status[128];

static unsigned long start_compute(if_bps_stats_t s[64], if_stats_t s1[64], if_stats_t s2[64], const char* vendor, int count, int seconds, int min_packet_count, char** policy){

    *policy = policy_status;
    memset(policy_status, 0, sizeof(policy_status));

    //计算bps, pps, 根据测速时间。读数为0的标记score0, 
    for(int i=0; i< count; i++){
        int len = strlen(s1[i].name);
        strcpy(s[i].name, s1[i].name);

        if(len == 0){
            s[i].active_score = 0;
            continue;
        }

        if(s[i].name[len -1] == ':')
          s[i].name[len -1] = '\0';

        if(s2[i].rx_bytes == 0 && s2[i].rx_packets ==0 ){
            s[i].rx_bytes = s[i].rx_packets = 0;
            s[i].active_score = 0;
        }else{
            s[i].rx_bytes = diff_ulong(s2[i].rx_bytes, s1[i].rx_bytes);
            s[i].rx_packets = diff_ulong(s2[i].rx_packets, s1[i].rx_packets);
            s[i].active_score = 1;
        }

        s[i].bps = s[i].rx_bytes/seconds;
        s[i].pps = s[i].rx_packets/seconds;

        //
        
    }

    for(int i=0; i< count; i++){
        if(s[i].active_score == 0)
            continue;
        if(strstr(s[i].name, "lo") == s[i].name || strstr(s[i].name, "lo0") == s[i].name)
            s[i].wan_type = 0;
        else if(strstr(s[i].name, "br0") == s[i].name || strstr(s[i].name, "br1") == s[i].name || strstr(s[i].name, "br2") == s[i].name)
            s[i].wan_type = 1;
        else if(strstr(s[i].name, "eth") == s[i].name)
            s[i].wan_type = 2;  // local phy device.
        else
            s[i].wan_type = 3; 
    }

    int speed_list[10][64];
    int speed_count = 0;

    memset(speed_list, 0, sizeof(speed_list));

    for(int i=0; i< count; i++){
        if(s[i].active_score == 0 || s[i].wan_type == 0 ||  s[i].wan_type == 1 || s[i].rx_packets < min_packet_count)
            continue;

        int sub_index=0;
        for(int j=i+1; j< count; j++){
            if(s[j].active_score == 0 || s[j].wan_type == 0 ||  s[j].wan_type == 1 || s[j].rx_packets < min_packet_count)
                continue;
            if(j == i || s[j].similar_count == 1 || s[i].similar_count == 1)
                continue;

            if(diff_percent(s[i].rx_packets, s[j].rx_packets) < 7 && diff_percent(s[i].rx_bytes, s[j].rx_bytes) < 7){
                s[i].similar_count = 1;
                s[j].similar_count = 1;
                speed_list[speed_count][sub_index + 2] = j;  
                sub_index++;
                printf("found similar %s, %s \n", s[i].name, s[j].name);
            }
            
        }

        if(sub_index){
            speed_list[speed_count][1] = i; 
            speed_list[speed_count][0] = sub_index + 1; 
            speed_count++;
        }

        if(speed_count == 9)
            break;
    }

    int max_bps = 0;
    int max_idx=0;
    for(int i=0; i< speed_count; i++){
        for(int j=0; j< speed_list[i][0]; j++){
            int idx = speed_list[i][j + 1];
            int mbps = s[idx].bps/131072; 
            if(match_vendor_pon_port(vendor, s[idx].name)){
                if(mbps > max_bps){
                    max_bps = mbps;
                    max_idx = idx;
                }

            }
        }
    }
    if(max_bps > 0){
        snprintf(policy_status, sizeof(policy_status)-1, "%s, %s", "matchvpon", s[max_idx].name);
        return s[max_idx].bps;
    }

    for(int i=0; i< speed_count; i++){
        for(int j=0; j< speed_list[i][0]; j++){
            int idx = speed_list[i][j + 1];
            int mbps = s[idx].bps/131072; 
            if(match_general_pon_port(s[idx].name)){
                if(mbps > max_bps){
                    max_bps = mbps;
                    max_idx = idx;
                }

            }
        }
    }

    if(max_bps > 0){
        snprintf(policy_status, sizeof(policy_status)-1, "%s, %s", "matchgpon", s[max_idx].name);
        return s[max_idx].bps;
    }

    for(int i=0; i< count; i++){
        if(s[i].active_score == 0 || s[i].wan_type == 0 ||  s[i].wan_type == 1 || s[i].rx_packets < min_packet_count)
            continue;
        int mbps = s[i].bps/131072; 

        if(match_vendor_pon_port(vendor, s[i].name)){
            if(mbps > max_bps){
                max_bps = mbps;
                max_idx = i;
            }
        }
    }

    if(max_bps > 0){
         snprintf(policy_status, sizeof(policy_status)-1, "%s, %s", "allvpon", s[max_idx].name);
        return s[max_idx].bps;
    }

    for(int i=0; i< count; i++){
        if(s[i].active_score == 0 || s[i].wan_type == 0 ||  s[i].wan_type == 1 || s[i].rx_packets < min_packet_count)
            continue;

        int mbps = s[i].bps/131072; 

        if(match_general_pon_port(s[i].name)){
            if(mbps > max_bps){
                max_bps = mbps;
                max_idx = i;
            }

        }
    }

    if(max_bps > 0){
        snprintf(policy_status, sizeof(policy_status)-1, "%s, %s", "allgpon", s[max_idx].name);
        return s[max_idx].bps;
    }

    for(int i=0; i< count; i++){
        if(s[i].active_score == 0 || s[i].wan_type == 0 ||  s[i].wan_type == 1 || s[i].rx_packets < min_packet_count)
            continue;

        int mbps = s[i].bps/131072; 

        if(mbps > max_bps){
            max_bps = mbps;
            max_idx = i;
        }

    }

    if(max_bps > 0){
        snprintf(policy_status, sizeof(policy_status)-1, "%s, %s", "all", s[max_idx].name);
        return s[max_idx].bps;
    }

    // not found
    return 0;
}

/*
8145C   eth0    gem1    wan2    ppp258  gem1，比eth0更小
8125C   eth0    gem1    wan3    ppp259  
F650G (1.0)
F652    pon0    pon nbif2   ppp0    pon0， pon更底层
                    
TEWA-600AGM veip0   gpon0.ani   bcmsw   ppp1.3  gpon0.ani bcmsw
    veip0:  gpon0.ani
: pon_41_0_1:   pon:    ppp121: gpon0.ani
pon 更地层
TEWA-600AEM     epon0   bcmsw   ppp0.2 wanIP    bcmsw比veip0更底层，epon0和bcmsw一样，都比较合适
PT921G      nas1_1
    pon ppp9    
HG2201T 2.0     pon0.1033   pon0:   ppp0:   
TEWA-700G   bcmsw   gpon0.ani   veip0
veip0.5 ppp1.5: 
PT924 (MT7526)      nas4_0  pon ppp32   
PT924G
 (rtl9600)      nas0_1  nas0    ppp0    ppp0在200M的时候，数据都是0，系统停止工作了

*/

static int match_vendor_pon_port(const char*vendor, char* ifname)
{
    if(strstr(vendor, "huawei") != NULL){ 
        if(strstr(ifname, "gem") == ifname || strstr(ifname, "wan") == ifname )
            return 1;
    }else if(strstr(vendor, "zhongxing") != NULL){ 
        if(strstr(ifname, "pon") == ifname || strstr(ifname, "nbif") == ifname )
            return 1;
    }else if(strstr(vendor, "tianyi") != NULL){ 
        if(strstr(ifname, "gpon") == ifname || strstr(ifname, "veip") == ifname )
            return 1;
    }else if(strstr(vendor, "youhua") != NULL){
        if(strstr(ifname, "pon") == ifname || strstr(ifname, "nas") == ifname )
            return 1;
    }else if(strstr(vendor, "fenghuo") != NULL){ 
        if(strstr(ifname, "pon") == ifname ) //test code eth
            return 1;
    }
    return 0;
}

static int match_general_pon_port(char* ifname)
{
    if(strstr(ifname, "gpon") == ifname || 
       strstr(ifname, "epon") == ifname ||
       strstr(ifname, "pon") == ifname ||
       strstr(ifname, "wan") == ifname ||
       strstr(ifname, "veip") == ifname ||
       strstr(ifname, "nas") == ifname )
        return 1;
    return 0;
}

static int diff_percent(unsigned long a, unsigned long b)
{
    if(a >= b){
        return (a-b)*100.0/b;
    }else{
        return (b-a)*100.0/b;
    }
}

static void print_bps(int count){
    for(int i=0; i< count; i++){
        if(if_bps_stats[i].rx_bytes !=0 && if_bps_stats[i].rx_packets !=0)
            printf("%-10s bps:%dMbps\tpps:%d\n", if_bps_stats[i].name, if_bps_stats[i].bps/131072, if_bps_stats[i].pps);
    }
    printf("\n");
}

static void print_if_stat(if_stats_t if_stats[64], int count){
    for(int i=0; i< count; i++){
        printf("proc dev %s %lu, %lu\n", if_stats[i].name, if_stats[i].rx_bytes, if_stats[i].rx_packets);
    }
}

unsigned long if_stats_start_compute(const char* vendor, int count, int seconds, int min_packet_count, char** policy){
    return start_compute(if_bps_stats, if_stats_begin, if_stats_end, vendor, count, seconds, min_packet_count, policy);
}

#if 0
int if_stats_main(int argc, char* argv[])
{
    int count = 0;
    int sleep_time = 10;

    int bandwidth = 100;
    int packetBytes = 1460;
    int realPacketBytes = 1460;
    int time_seconds = 10;

    int min_packet_count = compute_packets(50, time_seconds, packetBytes, realPacketBytes);

    int packet_count = compute_packets(bandwidth, time_seconds, packetBytes, realPacketBytes);

    printf("idea packets %d min packets %d\n", packet_count, min_packet_count);

    time_t begin = time(NULL);

    printf("ULONG_MAX %lu\n", ULONG_MAX);
    
    count = get_interface_flow_by_procfs(if_stats_begin);
    print_if_stat(if_stats_begin, count);

    memset(if_bps_stats, 0, sizeof(if_bps_stats));

    for(int i=0; i<1; i++){
        
        sleep(sleep_time);

        count = get_interface_flow_by_procfs(if_stats_end);
        //print_if_stat(if_stats_end, count);

        //printf("abc %s\n%s\n", abc, def);         

        time_t end = time(NULL);

        int seconds = sleep_time; //end - begin;
        char * policy;
        int  bps = start_compute(if_bps_stats, if_stats_begin, if_stats_end, "fenghuo", count, seconds, min_packet_count, &policy);
        printf("last policy %s  %dMbps\n", policy, bps/131072);

        print_bps(count); 
        memcpy(if_stats_begin, if_stats_end, sizeof(if_stats_begin));
    }

}
#endif

