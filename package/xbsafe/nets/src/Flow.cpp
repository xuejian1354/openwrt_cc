#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <limits.h>
#include "Common.h"
#include "Flow.h"


extern "C" int get_local_interface(char* eth);
extern "C" int get_wan_interface(char* eth);
//extern "C" int get_interface_flow_by_api(const char* ifname, unsigned long *rx_bytes, unsigned long *rx_packets);
static int get_interface_flow_by_procfs(const char* ifname, unsigned long *rx_bytes, unsigned long *rx_packets);

static unsigned long  sys_file_value(char* file){
    unsigned long val = 0;

    char buf[1024];    
    FILE *p=NULL;
    p = fopen(file,"r");
    if(p == NULL)
        return 0;

    //fseek(p , 0 , 0);
    fscanf(p,"%lu",&val);
    fclose(p);
    return val; 
}

unsigned long compute_diff(unsigned long end,  unsigned long start){
    if(end >= start)
        return end - start;
    else
        return UINT_MAX-start + end;
}


NetFlow::NetFlow(void){
    strcpy(local, "");
    strcpy(wan, "");
    
	get_local_interface(local);
	get_wan_interface(wan);

#if defined(SW_PLATFORM_YOUHUA)    
    struct stat statbuf;

    sysfs_exist = 0;
    if (stat("/sys/class/net", &statbuf) != -1) {
        if (S_ISDIR(statbuf.st_mode)) {
            sysfs_exist = 1;
        }
    }
    printf("detect /sys %d\n", sysfs_exist);
#else
    sysfs_exist = 1;
#endif
    
}


/*
robin@inspiron:~/code/200M_speed/cplugin$ cat /sys/class/net/enp3s0/statistics/
collisions           rx_errors            rx_packets           tx_errors
multicast            rx_fifo_errors       tx_aborted_errors    tx_fifo_errors
rx_bytes             rx_frame_errors      tx_bytes             tx_heartbeat_errors
rx_compressed        rx_length_errors     tx_carrier_errors    tx_packets
rx_crc_errors        rx_missed_errors     tx_compressed        tx_window_errors
rx_dropped           rx_over_errors       tx_dropped           

*/
void NetFlow::sysflow_start(void){
    char buf[256];	

    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", wan);
    flow.wan_rx_start = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", wan);
    flow.wan_tx_start = sys_file_value(buf);

    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", local);
    flow.local_rx_start = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", local);
    flow.local_tx_start = sys_file_value(buf);

    sprintf(buf, "/sys/class/net/%s/statistics/rx_packets", wan);
    flow.wan_packet_rx_start = sys_file_value(buf);

    sprintf(buf, "/sys/class/net/%s/statistics/rx_dropped", wan);
    flow.wan_packet_rx_droped_start = sys_file_value(buf);

    flow.wan_rx_end = flow.wan_rx_start;
    flow.wan_tx_end = flow.wan_tx_start;
}

void NetFlow::nlflow_start(void){
    get_interface_flow_by_procfs(wan, &flow.wan_rx_start, &flow.wan_packet_rx_start);
   
    flow.wan_rx_end = flow.wan_rx_start;

    // not support following 
    flow.wan_tx_start = 0;
    flow.wan_tx_end = 0;  
    flow.wan_packet_rx_droped_start = 0;
    flow.wan_packet_rx_droped_end = 0;
    flow.local_tx_start = 0;
    flow.local_tx_end = 0;

}

void NetFlow::nlflow_stop(void){
    get_interface_flow_by_procfs(wan, &flow.wan_rx_end, &flow.wan_packet_rx_end);
}

void NetFlow::checkpoint_nl(void){
    flow.wan_rx_start = flow.wan_rx_end;
    get_interface_flow_by_procfs(wan, &flow.wan_rx_end, &flow.wan_packet_rx_end);
}

void NetFlow::checkpoint_sys(void){
	char buf[256];

    flow.wan_rx_start = flow.wan_rx_end;
    flow.wan_tx_start = flow.wan_tx_end;

    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", wan);
    flow.wan_rx_end = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", wan);
    flow.wan_tx_end = sys_file_value(buf);

    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", local);
    flow.local_rx_end = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", local);
    flow.local_tx_end = sys_file_value(buf);

}

void NetFlow::checkpoint(void){

    if(sysfs_exist == 0)
        checkpoint_nl();
    else
        checkpoint_sys();
}

void NetFlow::sysflow_stop(void){
    char buf[256];
    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", wan);
    flow.wan_rx_end = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", wan);
    flow.wan_tx_end = sys_file_value(buf);

    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", local);
    flow.local_rx_end = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", local);
    flow.local_tx_end = sys_file_value(buf);

    sprintf(buf, "/sys/class/net/%s/statistics/rx_packets", wan);
    flow.wan_packet_rx_end = sys_file_value(buf);

    sprintf(buf, "/sys/class/net/%s/statistics/rx_dropped", wan);
    flow.wan_packet_rx_droped_end = sys_file_value(buf);    
}

void NetFlow::compute(void){
    local_rx_bytes = compute_diff(flow.local_rx_end, flow.local_rx_start);
    local_tx_bytes = compute_diff(flow.local_tx_end, flow.local_tx_start);
    wan_rx_bytes = compute_diff(flow.wan_rx_end, flow.wan_rx_start);
    wan_tx_bytes = compute_diff(flow.wan_tx_end, flow.wan_tx_start);

}

void NetFlow::start(void){
	tp.Begin();
    if(sysfs_exist == 0)
        nlflow_start();
    else
        sysflow_start();
}

void NetFlow::stop(void){
    if(sysfs_exist == 0)
        nlflow_stop();
    else
	   sysflow_stop();	

	time_interval = tp.End();
    if(time_interval == 0)
     	time_interval +=1;
     		
	compute();
}

unsigned long NetFlow::getWanBytes(void){
	return compute_diff(flow.wan_rx_end, flow.wan_rx_start);
}

unsigned long NetFlow::getWanRxPackets(void){
    return compute_diff(flow.wan_packet_rx_end, flow.wan_packet_rx_start);
}

unsigned long NetFlow::getWanRxDropedPackets(void){
    return compute_diff(flow.wan_packet_rx_droped_end, flow.wan_packet_rx_droped_start);
}

unsigned long NetFlow::getWanBandwidth(void){
    return (wan_rx_bytes + wan_tx_bytes)/time_interval*1000/1024 ;
}

unsigned long NetFlow::getLocalBandwidth(void){
	return (local_rx_bytes + local_tx_bytes)/time_interval*1000/1024 ;
}

/* for method: cat /proc/net/dev | grep ppp32 | awk '/:/ { print($2, $3) }'
497373541 776895

# cat /proc/net/dev
Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
  eth3: 3889016520 3235931    0    0    0     0          0     73094 133916550 1677888    0    0    0     0       0          0
 wl0.1:       0       0    0    0    0 259922          0         0        0       0    0    0    0     0       0          0
*/

char* run_cmd(const char* str_cmd, int max_lines, int max_buffer_len);

static int get_interface_flow_by_procfs(const char* ifname, unsigned long *rx_bytes, unsigned long *rx_packets)
{
    char* pdata=NULL;
    char cmd_buffer[256];
    sprintf(cmd_buffer, "cat /proc/net/dev | grep %s | awk '/:/ { print($2, $3) }'", ifname);

    *rx_bytes = 0;
    *rx_packets = 0;
    pdata = run_cmd(cmd_buffer, 10, 1*1024);
    if(pdata){
        sscanf(pdata, "%lu %lu", rx_bytes, rx_packets);
        free(pdata);
    }
    //printf("proc dev rx %lu, %lu\n", *rx_bytes, *rx_packets);
    return 0;   
}