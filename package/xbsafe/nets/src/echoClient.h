#ifndef ECHOCLIENT_H
#define ECHOCLIENT_H 1

#ifdef __cplusplus
extern "C" {
#endif

int udp_download_command(const char * server, int port, int bandWidth, int PacketSize, int realPacketSize, int id);
int udp_packets_command(int sock, const char * server, int port);

// for speed test algorithm
int udp_download_command2(const char * server, int port, int bandWidth, int PacketSize, int realPacketSize, int TimeDuration, int delayMS, int id, int socketDelaySeconds);
int udp_packets_command2(int sock, const char * server, int port,  int socketDelaySeconds);

int udp_query_packet_count_command(int sock, const char * server, int port, int id, int timeout);

// low level socket function
int send_ctrl_message_low(int sock, const char* host, int port, char* message, char response[], int max_len);
int udp_send_msg_only(const char* host, int port, char* message);

int udp_upload_test_command(int sock, const char * server, int port, int timeout);
int udp_upload_result_command(int sock, const char * server, int port, int id, int timeout, char * response, int response_len);

#ifdef __cplusplus
}
#endif

#endif