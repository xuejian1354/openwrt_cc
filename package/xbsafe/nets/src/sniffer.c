#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<stdio.h>   //For standard things
#include<stdlib.h>  //malloc
#include<string.h>  //strlen
#include <ctype.h>


#include <net/if.h>
#include<netinet/ip_icmp.h> //Provides declarations for icmp header
#include<netinet/udp.h> //Provides declarations for udp header
#include<netinet/tcp.h> //Provides declarations for tcp header
#include<netinet/ip.h>  //Provides declarations for ip header
#include<netinet/if_ether.h>    //For ETH_P_ALL
#include<net/ethernet.h>    //For ether_header
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<sys/time.h>
#include<sys/types.h>
#include<unistd.h>
#include <getopt.h>

#include <limits.h>

#include<stdio.h>
#include<signal.h>
#include<sys/time.h>//itimerval结构体的定义

#include <pthread.h>

#define TOTAL_TIME 10   // 10s for flow statistics

#define IS_EQUAL_MAC(mac1, mac2) (mac1[0] == mac2[0] && \
       mac1[1] == mac2[1] && \
       mac1[2] == mac2[2] && \
       mac1[3] == mac2[3] && \
       mac1[4] == mac2[4] && \
       mac1[5] == mac2[5])

typedef struct Args {
    char* host;
    int port;
    char* ether;

    char* direction;
    char* logfile;
    int debug;
    int output;   //0: packages, 1: bytes
    int verbose;
} Args;

#define DEFAULT_ARGS \
    { NULL, 0, NULL, "", NULL, 0, 0, 0 }

unsigned char mac_array[50][6];
int mac_count=0;

unsigned long couter_array[50][2];   //0: src upload, 1:dst downstream
unsigned long couter_array2[50][2];  // for bandwidth
unsigned long bandwidth_array[50][2];  // for bandwidth

float device_rx_ratio= 1.0;
float device_tx_ratio = 1.0;

char convert_dir(Args* args);

void sniffer_initialize(void);
void sniffer_uninitialize(void);

int find_interface(const char* ipaddr, char interface_buffer[]);

void network_flow2_start(char* wan, char* local);
void network_flow2_end(char* wan, char* local);
void adjust_flow(unsigned char* wan, unsigned char* local);

int netflow_start_thread();
void add_mac_count(unsigned char* mac, int stream, int Size);
void print_couter(void);

void update_bandwidth(void);

int count_ethernet(unsigned char* Buffer, int Size, char dir);

void netflow_mac_bandwidth(char* macStr, unsigned int* upstream, unsigned int* downstream);

void ProcessPacket(unsigned char* , int, Args*);
void print_ip_header(unsigned char* , int);
void print_tcp_packet(unsigned char * , int );
void print_udp_packet(unsigned char * , int );
void print_icmp_packet(unsigned char* , int );
void PrintData (unsigned char* , int);

void print_simple_ethernet_header(unsigned char* , int);
void print_simple_ip_header(unsigned char* , int);
int match_ethernet(unsigned char* Buffer, int Size, unsigned char* mac, char dir);
int match_ip(unsigned char* Buffer, int Size, unsigned int ip, char dir);

int macNumToStr(char *macAddr, char *str);
int macStrToMacNum(unsigned char *macstr,unsigned char *macAddrNum);

FILE *logfile = NULL;
struct sockaddr_in source,dest;
int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,i,j; 
unsigned int tcp_bytes=0,udp_bytes=0,icmp_bytes=0,others_bytes=0,igmp_bytes=0,total_bytes=0;

int dbg = 0;
int output = 0;

Args                args                = DEFAULT_ARGS;

int handle_count=0;
static int running = 0;
timer_t fade_in_timer;
int sock_raw = -1;


/*static*/void fade_in_callback(union sigval v)
{ 
   handle_count++;
   //printf("have handle count is %d\n",handle_count);
    if(args.output == 1)
        printf("TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\n", tcp , udp , icmp , igmp , others , total);
    else if(args.output == 2)
        printf("bytes: TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\n", 
            tcp_bytes , udp_bytes , icmp_bytes , igmp_bytes , others_bytes , total_bytes);   

    //unsigned int up, down;
    //char* macStr = "60:B6:17:F8:46:FC";
    //netflow_mac_bandwidth(macStr, &up, &down);
    //printf("bandwidth %s, %d, %d\n", macStr, up, down);

    update_bandwidth();

    if(handle_count%5 == 0)
        print_couter();

   if(handle_count > 60) //timeout
   {
    timer_delete(fade_in_timer);
    handle_count = 0;
    running = 0;
   }  

}

int starttimer()
{
    struct sigevent evp; 
    memset(&evp, 0, sizeof(evp));
    evp.sigev_value.sival_ptr = 0; //这里传一个参数进去，在timer的callback回调函数里面可以获得它  
    evp.sigev_notify = SIGEV_THREAD; //定时器到期后内核创建一个线程执行sigev_notify_function函数 
    evp.sigev_notify_function = fade_in_callback; //这个就是指定回调函数

    int ret = 0;
    ret = timer_create(CLOCK_REALTIME, &evp, &fade_in_timer);
    if(ret < 0)
    {
        printf("timer_create() fail, ret:%d", ret);
        return ret;
    }
 
    struct itimerspec ts;

    ts.it_interval.tv_sec = 1;
    ts.it_interval.tv_nsec = 0; //200ms 
    ts.it_value.tv_sec = 1;
    ts.it_value.tv_nsec = 0; //200ms 
    ret = timer_settime(fade_in_timer, TIMER_ABSTIME, &ts, NULL);
    if(ret < 0)
    {
        printf("timer_settime() fail, ret:%d", ret); 
        timer_delete(fade_in_timer);
        return ret;
    } 

    return 0;
}



void set_time(void)
{
   struct itimerval itv;
   itv.it_interval.tv_sec=1;//自动装载，之后每10秒响应一次
   itv.it_interval.tv_usec=0;
   itv.it_value.tv_sec=1;//第一次定时的时间
   itv.it_value.tv_usec=0;
   setitimer(ITIMER_REAL,&itv,NULL);
}

void clean_timer(void)
{
   struct itimerval itv;
   memset(&itv, 0, sizeof itv);
   setitimer(ITIMER_REAL,&itv,NULL);
}

void alarm_handle(int sig)
{
   handle_count++;

   //printf("have handle count is %d\n",handle_count);

    if(args.output == 1)
        printf("TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\n", tcp , udp , icmp , igmp , others , total);
    else if(args.output == 2)
        printf("bytes: TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\n", 
            tcp_bytes , udp_bytes , icmp_bytes , igmp_bytes , others_bytes , total_bytes);   

    //unsigned int up, down;
    //char* macStr = "60:B6:17:F8:46:FC";
    //netflow_mac_bandwidth(macStr, &up, &down);
    //printf("bandwidth %s, %d, %d\n", macStr, up, down);

    update_bandwidth();

    if(handle_count%5 == 0)
        print_couter();

   if(handle_count >= TOTAL_TIME) //timeout
   {
    clean_timer();
    handle_count = 0;
    running = 0;
   }    
}

/******************************************************************************
 * usage
 ******************************************************************************/
static void usage(void)
{
    fprintf(stderr, "Usage: encode [options]\n\n"
      "Options:\n"
      "-h | --host       Speech file to record to\n"
      "-p | --port        Video file to record to\n"
      "-e | --ether     Video standard to use for display (see below).\n"
      "-l | --logfile   Video standard to use for display (see below).\n"
      "-d | --debug     Video standard to use for display (see below).\n"
      "-o | --output     Video standard to use for display (see below).\n");
}


/******************************************************************************
 * parseArgs
 ******************************************************************************/
static void parseArgs(int argc, char *argv[], Args *argsp)
{
    const char shortOptions[] = "h:p:e:l:d:o:v";
    const struct option longOptions[] = {
        {"host",       required_argument, NULL, 'h'},
        {"port",        required_argument, NULL, 'p'},
        {"ether",       required_argument, NULL, 'e'},
        {"logfile",       required_argument, NULL, 'l'},
        {"direction",       required_argument, NULL, 'd'},
        {"output",       required_argument, NULL, 'o'},
        {"verbose",       no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    int     index;
    int     c;
    char    *extension;

    for (;;) {
        c = getopt_long(argc, argv, shortOptions, longOptions, &index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                break;

            case 'h':
                printf("hh: %s\n", optarg);
                argsp->host = strdup(optarg);
                break;

            case 'p':
                argsp->port = atoi(optarg);
                break;

            case 'e':
                argsp->ether = strdup(optarg);
                break;

            case 'l':
                argsp->logfile = strdup(optarg);
                break;

            case 'd':
                argsp->direction = strdup(optarg);
                break;
           case 'o':
                argsp->output = atoi(optarg);
                break;
           case 'v':
                argsp->verbose = 1;
                break;

            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }

}

static void netflow_clean(){
    //printf("mac size %d, %d\n", sizeof mac_array, sizeof couter_array);
    memset(mac_array, 0, sizeof mac_array);
    memset(couter_array, 0, sizeof couter_array);
    memset(couter_array2, 0, sizeof couter_array2);
    memset(bandwidth_array, 0, sizeof bandwidth_array);
   mac_count = 0;
}

void sniffer_initialize(void){
    int ret;

    char wan[64];
    memset(wan, 0, sizeof(wan));
    const char* wanIP = getWanIP();
    if(!find_interface(wanIP, wan)){
        perror("get_wan_interface Error");
        return;
    }

    sock_raw = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL)) ;

    ret = setsockopt(sock_raw , SOL_SOCKET , SO_BINDTODEVICE , wan , strlen(wan)+ 1 );
    printf("SO_BINDTODEVICE %d\n", ret);

    if(sock_raw < 0)
    {
        //Print the error with proper message
        perror("Socket Error");
    }

}

void sniffer_uninitialize(void){
    if(sock_raw > 0)
        close(sock_raw);
}

int get_local_interface(char* eth);
int get_wan_interface(char* eth);

static unsigned long timeGetTime()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec * 1000 + now.tv_usec/1000);
}

static void * thread_body(void *ptr){
    int saddr_size , data_size;
    struct sockaddr saddr;
    int ret;
    char wan_eth[128];
    char local_eth[128];

    unsigned long begin_time;

    printf("Starting netflow...\n");

    memset(wan_eth, 0, sizeof(wan_eth));
    memset(local_eth, 0, sizeof(local_eth));
    
    get_wan_interface(wan_eth);
    get_local_interface(local_eth);
    printf("wan if %s, lan if %s\n", wan_eth, local_eth);

    unsigned char *buffer = (unsigned char *) malloc(65536); //Its Big! 

    netflow_clean();
    running = 1;
    
    signal(SIGALRM,alarm_handle);
    set_time();  
    //starttimer();

    begin_time = timeGetTime();
    network_flow2_start(wan_eth, local_eth);


    while(running)
    {
        saddr_size = sizeof saddr;
        //Receive a packet
        data_size = recvfrom(sock_raw , buffer , 65536 , 0 , &saddr , (socklen_t*)&saddr_size);
        if(data_size <0)
        {
            printf("Recvfrom error , failed to get packets\n");
            sniffer_uninitialize();
            sniffer_initialize();
            sleep(2);
            break;
        }
        //Now process the packet
        ProcessPacket(buffer , data_size, &args);     
    }

    network_flow2_end(wan_eth, local_eth);
    adjust_flow(wan_eth, local_eth);
    flow_check(wan_eth, local_eth);

    free(buffer);    

THREAD_EXIT:

    pthread_detach(pthread_self());
    pthread_exit(0);    
    printf("Finished");
    return 0;

}


#ifdef DEBUG_SNIFFER
int main(int argc, char ** argv)
{
    int saddr_size , data_size;
    struct sockaddr saddr;

    /* Parse the arguments given to the app and set the app environment */
    parseArgs(argc, argv, &args);

    if(argc > 1)
        dbg = atoi(argv[1]);

    if(args.logfile){
        if(!strcmp("stdout", args.logfile))
            logfile= stdout;
        else 
            logfile= fopen(args.logfile,"w");
    }    
    printf("Starting...\n");
    sniffer_initialize();

     //netflow_start_thread();
    thread_body(NULL);

    printf("Finished");
    return 0;
}

#endif 

void ProcessPacket(unsigned char* buffer, int size, Args *args)
{
    //Get the IP Header part of this packet , excluding the ethernet header
    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));

    int found = 0;

    if(1)
    {
        if(args->ether){
            char mac[6];
            macStrToMacNum(args->ether, mac);
            found = match_ethernet(buffer , size, mac, convert_dir(args));
            if(found && args->verbose){
                print_simple_ethernet_header(buffer, size);
                print_simple_ip_header(buffer , size);
            }
        }else if(args->host){
            found = match_ip(buffer , size, inet_addr(args->host), convert_dir(args));
            if(found && args->verbose){
                print_simple_ethernet_header(buffer, size);
                print_simple_ip_header(buffer , size);

            }
        }else if(iph->protocol == args->port && args->port != 0){
            found = 1;
            if(found && args->verbose){
                print_simple_ethernet_header(buffer, size);
                print_simple_ip_header(buffer , size);   
            }
        }else{
           found = 1;   
           count_ethernet(buffer , size, convert_dir(args));
        }

    }

    if(found){
    ++total;
    total_bytes +=size;

    switch (iph->protocol) //Check the Protocol and do accordingly...
    {
        case 1:  //ICMP Protocol
            ++icmp;
            icmp_bytes +=size;
            if(args->logfile != NULL)
                print_icmp_packet( buffer , size);
            break;
        
        case 2:  //IGMP Protocol
            ++igmp;
            igmp_bytes +=size;
            break;
        
        case 6:  //TCP Protocol
            ++tcp;
            tcp_bytes +=size;
            if(args->logfile  != NULL)
                print_tcp_packet(buffer , size);           
            break;
        
        case 17: //UDP Protocol
            ++udp;
            udp_bytes +=size;
            if(args->logfile  != NULL)
                print_udp_packet(buffer , size);
            break;
        
        default: //Some Other Protocol like ARP etc.
            ++others;
            others_bytes +=size;
            break;
    }
    }

}

char convert_dir(Args* args){
    if(!strcmp(args->direction, "src"))
        return 's';
    else if(!strcmp(args->direction, "dst"))
        return 'd';
    else
        return 'b';    
}


int match_ethernet(unsigned char* Buffer, int Size, unsigned char* mac, char dir)
{
    struct ethhdr *eth = (struct ethhdr *)Buffer;

    if(dir == 's')
        return IS_EQUAL_MAC(eth->h_source, mac);
    else if(dir == 'd')
        return IS_EQUAL_MAC(eth->h_dest, mac);
    else
        return IS_EQUAL_MAC(eth->h_source, mac) || IS_EQUAL_MAC(eth->h_dest, mac);
}

int count_ethernet(unsigned char* Buffer, int Size, char dir)
{
    struct ethhdr *eth = (struct ethhdr *)Buffer;

    if(dir == 's')
        add_mac_count(eth->h_source, 0, Size);
    else if(dir == 'd')
        add_mac_count(eth->h_dest, 1, Size);
    else{
        add_mac_count(eth->h_source, 0, Size);
        add_mac_count(eth->h_dest, 1, Size);
    } 


}

int match_ip(unsigned char* Buffer, int Size, unsigned int ip, char dir)
{
    unsigned short iphdrlen;
        
    struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr) );
    iphdrlen =iph->ihl*4;
    
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
    
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;

    if(dir == 's')
        return ip == iph->saddr;
    else if(dir == 'd')
        return ip == iph->daddr;
    else
        return ip == iph->saddr || ip == iph->daddr;
}

void print_ethernet_header(unsigned char* Buffer, int Size)
{
    struct ethhdr *eth = (struct ethhdr *)Buffer;
    
    fprintf(logfile , "\n");
    fprintf(logfile , "Ethernet Header\n");
    fprintf(logfile , "   |-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_dest[0] , eth->h_dest[1] , eth->h_dest[2] , eth->h_dest[3] , eth->h_dest[4] , eth->h_dest[5] );
    fprintf(logfile , "   |-Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_source[0] , eth->h_source[1] , eth->h_source[2] , eth->h_source[3] , eth->h_source[4] , eth->h_source[5] );
    fprintf(logfile , "   |-Protocol            : %u \n",(unsigned short)eth->h_proto);
}

void print_simple_ethernet_header(unsigned char* Buffer, int Size)
{
    char src[32], dst[32];
    struct ethhdr *eth = (struct ethhdr *)Buffer;
    
    macNumToStr(eth->h_source, src);
    macNumToStr(eth->h_dest, dst);
    printf("  eth %s, %s, len %d\n", src, dst, Size);
}

void print_simple_ip_header(unsigned char* Buffer, int Size)
{
    unsigned short iphdrlen;
    char src[32], dst[32];

    struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr) );
    iphdrlen =iph->ihl*4;
    
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
    
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;
    
    strcpy(src, inet_ntoa(source.sin_addr));
    strcpy(dst, inet_ntoa(dest.sin_addr));

    printf("    ip %s, %s, len %d\n", src, dst, ntohs(iph->tot_len));
}

void print_ip_header(unsigned char* Buffer, int Size)
{
    print_ethernet_header(Buffer , Size);
  
    unsigned short iphdrlen;
        
    struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr) );
    iphdrlen =iph->ihl*4;
    
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
    
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;
    
    fprintf(logfile , "\n");
    fprintf(logfile , "IP Header\n");
    fprintf(logfile , "   |-IP Version        : %d\n",(unsigned int)iph->version);
    fprintf(logfile , "   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
    fprintf(logfile , "   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
    fprintf(logfile , "   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
    fprintf(logfile , "   |-Identification    : %d\n",ntohs(iph->id));
    //fprintf(logfile , "   |-Reserved ZERO Field   : %d\n",(unsigned int)iphdr->ip_reserved_zero);
    //fprintf(logfile , "   |-Dont Fragment Field   : %d\n",(unsigned int)iphdr->ip_dont_fragment);
    //fprintf(logfile , "   |-More Fragment Field   : %d\n",(unsigned int)iphdr->ip_more_fragment);
    fprintf(logfile , "   |-TTL      : %d\n",(unsigned int)iph->ttl);
    fprintf(logfile , "   |-Protocol : %d\n",(unsigned int)iph->protocol);
    fprintf(logfile , "   |-Checksum : %d\n",ntohs(iph->check));
    fprintf(logfile , "   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
    fprintf(logfile , "   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
}

void print_tcp_packet(unsigned char* Buffer, int Size)
{
    unsigned short iphdrlen;
    
    struct iphdr *iph = (struct iphdr *)( Buffer  + sizeof(struct ethhdr) );
    iphdrlen = iph->ihl*4;
    
    struct tcphdr *tcph=(struct tcphdr*)(Buffer + iphdrlen + sizeof(struct ethhdr));
            
    int header_size =  sizeof(struct ethhdr) + iphdrlen + tcph->doff*4;
    
    fprintf(logfile , "\n\n***********************TCP Packet*************************\n");  
        
    print_ip_header(Buffer,Size);
        
    fprintf(logfile , "\n");
    fprintf(logfile , "TCP Header\n");
    fprintf(logfile , "   |-Source Port      : %u\n",ntohs(tcph->source));
    fprintf(logfile , "   |-Destination Port : %u\n",ntohs(tcph->dest));
    fprintf(logfile , "   |-Sequence Number    : %u\n",ntohl(tcph->seq));
    fprintf(logfile , "   |-Acknowledge Number : %u\n",ntohl(tcph->ack_seq));
    fprintf(logfile , "   |-Header Length      : %d DWORDS or %d BYTES\n" ,(unsigned int)tcph->doff,(unsigned int)tcph->doff*4);
    //fprintf(logfile , "   |-CWR Flag : %d\n",(unsigned int)tcph->cwr);
    //fprintf(logfile , "   |-ECN Flag : %d\n",(unsigned int)tcph->ece);
    fprintf(logfile , "   |-Urgent Flag          : %d\n",(unsigned int)tcph->urg);
    fprintf(logfile , "   |-Acknowledgement Flag : %d\n",(unsigned int)tcph->ack);
    fprintf(logfile , "   |-Push Flag            : %d\n",(unsigned int)tcph->psh);
    fprintf(logfile , "   |-Reset Flag           : %d\n",(unsigned int)tcph->rst);
    fprintf(logfile , "   |-Synchronise Flag     : %d\n",(unsigned int)tcph->syn);
    fprintf(logfile , "   |-Finish Flag          : %d\n",(unsigned int)tcph->fin);
    fprintf(logfile , "   |-Window         : %d\n",ntohs(tcph->window));
    fprintf(logfile , "   |-Checksum       : %d\n",ntohs(tcph->check));
    fprintf(logfile , "   |-Urgent Pointer : %d\n",tcph->urg_ptr);
    fprintf(logfile , "\n");
    fprintf(logfile , "                        DATA Dump                         ");
    fprintf(logfile , "\n");
        
    fprintf(logfile , "IP Header\n");
    PrintData(Buffer,iphdrlen);
        
    fprintf(logfile , "TCP Header\n");
    PrintData(Buffer+iphdrlen,tcph->doff*4);
        
    fprintf(logfile , "Data Payload\n");    
    PrintData(Buffer + header_size , Size - header_size );
                        
    fprintf(logfile , "\n###########################################################");
}

void print_udp_packet(unsigned char *Buffer , int Size)
{
    
    unsigned short iphdrlen;
    
    struct iphdr *iph = (struct iphdr *)(Buffer +  sizeof(struct ethhdr));
    iphdrlen = iph->ihl*4;
    
    struct udphdr *udph = (struct udphdr*)(Buffer + iphdrlen  + sizeof(struct ethhdr));
    
    int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof udph;
    
    fprintf(logfile , "\n\n***********************UDP Packet*************************\n");
    
    print_ip_header(Buffer,Size);           
    
    fprintf(logfile , "\nUDP Header\n");
    fprintf(logfile , "   |-Source Port      : %d\n" , ntohs(udph->source));
    fprintf(logfile , "   |-Destination Port : %d\n" , ntohs(udph->dest));
    fprintf(logfile , "   |-UDP Length       : %d\n" , ntohs(udph->len));
    fprintf(logfile , "   |-UDP Checksum     : %d\n" , ntohs(udph->check));
    
    fprintf(logfile , "\n");
    fprintf(logfile , "IP Header\n");
    PrintData(Buffer , iphdrlen);
        
    fprintf(logfile , "UDP Header\n");
    PrintData(Buffer+iphdrlen , sizeof udph);
        
    fprintf(logfile , "Data Payload\n");    
    
    //Move the pointer ahead and reduce the size of string
    PrintData(Buffer + header_size , Size - header_size);
    
    fprintf(logfile , "\n###########################################################");
}

void print_icmp_packet(unsigned char* Buffer , int Size)
{
    unsigned short iphdrlen;
    
    struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr));
    iphdrlen = iph->ihl * 4;
    
    struct icmphdr *icmph = (struct icmphdr *)(Buffer + iphdrlen  + sizeof(struct ethhdr));
    
    int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof icmph;
    
    fprintf(logfile , "\n\n***********************ICMP Packet*************************\n"); 
    
    print_ip_header(Buffer , Size);
            
    fprintf(logfile , "\n");
        
    fprintf(logfile , "ICMP Header\n");
    fprintf(logfile , "   |-Type : %d",(unsigned int)(icmph->type));
            
    if((unsigned int)(icmph->type) == 11)
    {
        fprintf(logfile , "  (TTL Expired)\n");
    }
    else if((unsigned int)(icmph->type) == ICMP_ECHOREPLY)
    {
        fprintf(logfile , "  (ICMP Echo Reply)\n");
    }
    
    fprintf(logfile , "   |-Code : %d\n",(unsigned int)(icmph->code));
    fprintf(logfile , "   |-Checksum : %d\n",ntohs(icmph->checksum));
    //fprintf(logfile , "   |-ID       : %d\n",ntohs(icmph->id));
    //fprintf(logfile , "   |-Sequence : %d\n",ntohs(icmph->sequence));
    fprintf(logfile , "\n");

    fprintf(logfile , "IP Header\n");
    PrintData(Buffer,iphdrlen);
        
    fprintf(logfile , "UDP Header\n");
    PrintData(Buffer + iphdrlen , sizeof icmph);
        
    fprintf(logfile , "Data Payload\n");    
    
    //Move the pointer ahead and reduce the size of string
    PrintData(Buffer + header_size , (Size - header_size) );
    
    fprintf(logfile , "\n###########################################################");
}

void PrintData (unsigned char* data , int Size)
{
    int i , j;
    for(i=0 ; i < Size ; i++)
    {
        if( i!=0 && i%16==0)   //if one line of hex printing is complete...
        {
            fprintf(logfile , "         ");
            for(j=i-16 ; j<i ; j++)
            {
                if(data[j]>=32 && data[j]<=128)
                    fprintf(logfile , "%c",(unsigned char)data[j]); //if its a number or alphabet
                
                else fprintf(logfile , "."); //otherwise print a dot
            }
            fprintf(logfile , "\n");
        } 
        
        if(i%16==0) fprintf(logfile , "   ");
            fprintf(logfile , " %02X",(unsigned int)data[i]);
                
        if( i==Size-1)  //print the last spaces
        {
            for(j=0;j<15-i%16;j++) 
            {
              fprintf(logfile , "   "); //extra spaces
            }
            
            fprintf(logfile , "         ");
            
            for(j=i-i%16 ; j<=i ; j++)
            {
                if(data[j]>=32 && data[j]<=128) 
                {
                  fprintf(logfile , "%c",(unsigned char)data[j]);
                }
                else 
                {
                  fprintf(logfile , ".");
                }
            }
            
            fprintf(logfile ,  "\n" );
        }
    }
}

unsigned char hex_digit( char ch )
{
    if(( '0' <= ch ) && ( ch <= '9' )) 
        { ch -= '0'; }
    else
    {
        if(( 'a' <= ch ) && ( ch <= 'f' )) 
            { ch += 10 - 'a'; }
        else
        {
            if(( 'A' <= ch ) && ( ch <= 'F' )) 
                { ch += 10 - 'A'; }
            else
                { ch = 16; }
        }
    }
    return ch;
}

/* 
 * "AA:BB:CC:DD:EE:FF" -> mac[6]
 */

int macStrToMacNum(unsigned char *macstr,unsigned char *mac)
{
    //unsigned char mac[6];
    uint idx;
    for(idx = 0; idx < 6; ++idx )
    {
        mac[idx]  = hex_digit( macstr[     3 * idx ] ) << 4;
        mac[idx] |= hex_digit( macstr[ 1 + 3 * idx ] );
    }
    return 0;
}

int macStr2ToMacNum(unsigned char *macstr,unsigned char *mac)
{
    //unsigned char mac[6];
    uint idx;
    for(idx = 0; idx < 6; ++idx )
    {
        mac[idx]  = hex_digit( macstr[     2 * idx ] ) << 4;
        mac[idx] |= hex_digit( macstr[ 1 + 2 * idx ] );
    }
    return 0;
}

int macStrAutoToMacNum(unsigned char *macstr,unsigned char *mac)
{
    if(strlen((char*)macstr) == 17)
        macStrToMacNum(macstr, mac);
    else
        macStr2ToMacNum(macstr, mac);
    return 0;
}

int macNumToStr(char *macAddr, char *str)
{
   if ( macAddr == NULL ) return -1;
   if ( str == NULL ) return -1;
   sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
           (unsigned char ) macAddr[0], (unsigned char ) macAddr[1], (unsigned char ) macAddr[2],
           (unsigned char ) macAddr[3], (unsigned char ) macAddr[4], (unsigned char ) macAddr[5]);
   return 0;
}


void add_mac_count(unsigned char* mac, int stream, int Size){
    int i;
    for(i=0; i< mac_count; i++){
        if(IS_EQUAL_MAC(mac, mac_array[i]))
            break;
    }

    if(i < mac_count)  
        couter_array[i][stream] += Size;
    else if(mac_count < 50){
        mac_count++;
        couter_array[i][stream] += Size;
        mac_array[i][0] = mac[0];
        mac_array[i][1] = mac[1];
        mac_array[i][2] = mac[2];
        mac_array[i][3] = mac[3];
        mac_array[i][4] = mac[4];
        mac_array[i][5] = mac[5];
    }

}

void print_couter(void){
    int i;
    char macStr[32];

    printf("\nmacs %d\n", mac_count);
    for(i=0; i< mac_count; i++){
       macNumToStr(mac_array[i], macStr);
       printf("count: %s, %12d,(%d), %12d(%d)\n", macStr, couter_array[i][0], bandwidth_array[i][0],couter_array[i][1], bandwidth_array[i][1]);
    }
}

void netflow_mac_stats(unsigned char* macStr, unsigned int* upstream, unsigned int* downstream)
{
    unsigned char mac[6];

    *upstream = 0;
    *downstream = 0;
    
    if(strlen(macStr) == 17)
        macStrToMacNum(macStr, mac);
    else
        macStr2ToMacNum(macStr, mac);

    for(i=0; i< mac_count; i++){
       if(IS_EQUAL_MAC(mac, mac_array[i])){
            *upstream = couter_array[i][0];
            *downstream = couter_array[i][1];
            break;
        }
    }
}

void update_bandwidth(void){
    int i;

    for(i=0; i< mac_count; i++){
        bandwidth_array[i][0] = (couter_array[i][0]-couter_array2[i][0])/1024;
        bandwidth_array[i][1] = (couter_array[i][1]-couter_array2[i][1])/1024;

        couter_array2[i][0] = couter_array[i][0];
        couter_array2[i][1] = couter_array[i][1];
    }
}

void netflow_mac_bandwidth_one_second(char* macStr, unsigned int* upstream, unsigned int* downstream)
{
    unsigned char mac[6];

    *upstream = 0;
    *downstream = 0;
    
    if(strlen(macStr) == 17)
        macStrToMacNum(macStr, mac);
    else
        macStr2ToMacNum(macStr, mac);

    for(i=0; i< mac_count; i++){    
        if(IS_EQUAL_MAC(mac, mac_array[i])){
            *upstream = bandwidth_array[i][0]; // KB/s
            *downstream = bandwidth_array[i][1]; // KB/s

            break;
        }
    }
}

void netflow_mac_bandwidth(char* macStr, unsigned int* upstream, unsigned int* downstream)
{
    unsigned char mac[6];

    *upstream = 0;
    *downstream = 0;
    
    if(strlen(macStr) == 17)
        macStrToMacNum(macStr, mac);
    else
        macStr2ToMacNum(macStr, mac);

    for(i=0; i< mac_count; i++){    
        if(IS_EQUAL_MAC(mac, mac_array[i])){
            *upstream = couter_array[i][0]/device_tx_ratio/1024/TOTAL_TIME; // KB/s
            *downstream = couter_array[i][1]/device_rx_ratio/1024/TOTAL_TIME; // KB/s

            break;
        }
    }
}

int netflow_start_thread(){
    if(running)
        return -1;

    pthread_t threadID;
    int result = pthread_create(&threadID, NULL, &thread_body, NULL);

}

int netflow_stop_thread(){
    if(running){
        clean_timer();
        running = 0;
    }
    

}

unsigned long compute_diff(unsigned long end,  unsigned long start){
    if(end >= start)
        return end - start;
    else
        return UINT_MAX-start + end;
}

unsigned long  sys_file_value(char* file){
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

int find_interface(const char* ipaddr, char interface_buffer[]) {
 
 struct ifconf ifc;  
 int numreqs = 20;  
 int fd;  
 int i;  
 struct ifreq * ifr = NULL;  
 char addrbuf[128];
 int found = 0;

 if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)  
 {  
  perror("socket");  
  exit(1);  
 }  
  
 ifc.ifc_len = sizeof(struct ifreq) * numreqs;  
 ifc.ifc_buf = malloc(ifc.ifc_len);  
 memset(ifc.ifc_buf, 0, ifc.ifc_len);  
  
 if(ioctl(fd, SIOCGIFCONF, &ifc) != 0)  
 {  
  perror("SIOCGIFCONF");  
  free(ifc.ifc_buf);  
  close(fd);  
  exit(1);  
 }  
  
 close(fd);  
  
 if(ifc.ifc_len == sizeof(struct ifreq) * numreqs)  
  printf("The system has at least %d interfaces, "  
   "please increase the buffer.\n", numreqs);  
  
 ifr = ifc.ifc_req;  
  
 printf("Interfaces in the system (by SIOCGIFCONF):\n");  
 printf("--------------------------------------------------\n");  
 for(i=0; i<ifc.ifc_len; i+=sizeof(struct ifreq)) {  
  //printf("%s\n", ifr->ifr_name);  
 switch (ifr->ifr_addr.sa_family) {
        case AF_INET:
            ++i;
            inet_ntop(ifr->ifr_addr.sa_family, &((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr, addrbuf, sizeof(addrbuf));
            printf("%d. %s, %s\n", i, ifr->ifr_name, addrbuf);

            if(!strcmp(ipaddr, addrbuf))
            {
                strcpy(interface_buffer, ifr->ifr_name);
                found = 1;
            }    
            break;
#if 0
        case AF_INET6:
            ++i;
            printf("%d. %s : %s\n", i, ifr->ifr_name, inet_ntop(ifr->ifr_addr.sa_family, &((struct sockaddr_in6*)&ifr->ifr_addr)->sin6_addr, addrbuf, sizeof(addrbuf)));
            break;
#endif
      }    
  ifr++;  
 }  
  
 free(ifc.ifc_buf);  

 return found;
}

int get_mac(char* eth, char* mac)
{
    int sockfd;
    struct ifreq tmp;   
    char mac_addr[30];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0)
    {
        perror("create socket fail\n");
        return -1;
    }

    memset(&tmp,0,sizeof(struct ifreq));
    strncpy(tmp.ifr_name,eth,sizeof(tmp.ifr_name)-1);
    if( (ioctl(sockfd,SIOCGIFHWADDR,&tmp)) < 0 )
    {
        printf("mac ioctl error\n");
        return -1;
    }

    sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x",
            (unsigned char)tmp.ifr_hwaddr.sa_data[0],
            (unsigned char)tmp.ifr_hwaddr.sa_data[1],
            (unsigned char)tmp.ifr_hwaddr.sa_data[2],
            (unsigned char)tmp.ifr_hwaddr.sa_data[3],
            (unsigned char)tmp.ifr_hwaddr.sa_data[4],
            (unsigned char)tmp.ifr_hwaddr.sa_data[5]
            );

    close(sockfd);
    strcpy(mac,mac_addr);
    return 0;
}


unsigned long local_rx_start, local_rx_end;
unsigned long local_tx_start, local_tx_end;
unsigned long wan_rx_start, wan_rx_end;
unsigned long wan_tx_start, wan_tx_end;

void network_flow2_start(char* wan, char* local){
    char buf[256];

    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", wan);
    wan_rx_start = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", wan);
    wan_tx_start = sys_file_value(buf);

    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", local);
    local_rx_start = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", local);
    local_tx_start = sys_file_value(buf);

}

void network_flow2_end(char* wan, char* local){
    char buf[256];

    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", wan);
    wan_rx_end = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", wan);
    wan_tx_end = sys_file_value(buf);

    sprintf(buf, "/sys/class/net/%s/statistics/rx_bytes", local);
    local_rx_end = sys_file_value(buf);
    sprintf(buf, "/sys/class/net/%s/statistics/tx_bytes", local);
    local_tx_end = sys_file_value(buf);

}

// device rx flow (local_tx_bytes, if local_tx_bytes > 500K, use local_srcStream/(float)local_tx_bytes to adjust device rx flow.
// device tx flow (local_rx_bytes, if local_rx_bytes > 500K, use local_dstStream/(float)local_rx_bytes) to adjust device rx flow.
void adjust_flow(unsigned char* wan, unsigned char* local){
    char buf[32];

    unsigned int wan_srcStream, wan_dstStream;
    unsigned int local_srcStream, local_dstStream;

    unsigned long local_rx_bytes = compute_diff(local_rx_end, local_rx_start);
    unsigned long local_tx_bytes = compute_diff(local_tx_end, local_tx_start);
    unsigned long wan_rx_bytes = compute_diff(wan_rx_end, wan_rx_start);
    unsigned long wan_tx_bytes = compute_diff(wan_tx_end, wan_tx_start);

    memset(buf, 0, sizeof(buf));
    get_mac(wan, buf);
    printf("wan: %s\n", buf);
    get_mac(local, buf);
    printf("local: %s\n", buf);


    get_mac(wan, buf);
    netflow_mac_stats(buf, &wan_srcStream, &wan_dstStream);

    get_mac(local, buf);
    netflow_mac_stats(buf, &local_srcStream, &local_dstStream);

    if(local_tx_bytes > 500*1024)
        device_rx_ratio = local_srcStream/(float)local_tx_bytes;
    else
        device_rx_ratio = 1.0;

    if(local_rx_bytes > 500*1024)
        device_tx_ratio = local_dstStream/(float)local_rx_bytes;
    else
        device_tx_ratio = 1.0;

    printf("device ratio rx %f, tx %f\n", device_rx_ratio, device_tx_ratio);

}

void flow_check(unsigned char* wan, unsigned char* local){
    char buf[32];

    unsigned int wan_srcStream, wan_dstStream;
    unsigned int local_srcStream, local_dstStream;

    unsigned long local_rx_bytes = compute_diff(local_rx_end, local_rx_start);
    unsigned long local_tx_bytes = compute_diff(local_tx_end, local_tx_start);
    unsigned long wan_rx_bytes = compute_diff(wan_rx_end, wan_rx_start);
    unsigned long wan_tx_bytes = compute_diff(wan_tx_end, wan_tx_start);

    get_mac(wan, buf);
    printf("wan: %s\n", buf);
    get_mac(local, buf);
    printf("local: %s\n", buf);


    get_mac(wan, buf);
    netflow_mac_stats(buf, &wan_srcStream, &wan_dstStream);

    get_mac(local, buf);
    netflow_mac_stats(buf, &local_srcStream, &local_dstStream);

    printf("   local rx %10ld, tx %10ld\n", local_rx_bytes, local_tx_bytes);
    printf("my local rx %10ld, tx %10ld\n", local_dstStream, local_srcStream);
    printf("\n");

    printf("   wan rx %10ld, tx %10ld\n", wan_rx_bytes, wan_tx_bytes);    
    printf("my wan rx %10ld, tx %10ld\n", wan_dstStream, wan_srcStream);
    printf("\n");

    printf("verify RX by sys flow %10ld, %10ld, %f\n", wan_rx_bytes, local_tx_bytes, wan_rx_bytes/(float)local_tx_bytes);
    printf("verify TX by sys flow %10ld, %10ld, %f\n", wan_tx_bytes, local_rx_bytes, wan_tx_bytes/(float)local_rx_bytes);
    printf("\n");   


    // verify device tx flow : dstStream in local mac : local_rx_bytes 
    printf("device TX flow %10ld, %10ld, %f\n", local_dstStream, local_rx_bytes, local_dstStream/(float)local_rx_bytes);
    // device rx flow : srcStream in local mac : local_tx_bytes
    printf("device RX flow %10ld, %10ld, %f\n", local_srcStream, local_tx_bytes, local_srcStream/(float)local_tx_bytes);
    printf("\n");

    // device rx flow : dstStream in wan mac : wan_rx_bytes
    printf("wan RX flow %10ld, %10ld, %f\n", wan_dstStream, wan_rx_bytes, wan_dstStream/(float)wan_rx_bytes);

    // device tx flow : srcStream in wan mac : wan_tx_bytes
    printf("wan TX flow %10ld, %10ld, %f\n", wan_srcStream, wan_tx_bytes, wan_srcStream/(float)wan_tx_bytes);



}
