#include<stdio.h>	//printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h> // for close
#include "echoClient.h"

#define SERVER "127.0.0.1"
#define BUFLEN 512	//Max length of buffer
#define PORT 8888	//The port on which to send data

void die(char *s)
{
	perror(s);
	//exit(1);
}

int udp_send_msg_only(const char* host, int port, char* message)
{
	struct sockaddr_in si_other;
	int s, i, slen=sizeof(si_other);

	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	
	if (inet_aton(host , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		close(s);
		return -1;
	}

	do
	{

		if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
		{
			close(s);
			return -1;
		}
		
		//puts(response);
	}while(0);

	close(s);
	return 0;
}

int socket_message(const char* host, int port, char* message, char response[], int max_len, int timeout)
{
	struct sockaddr_in si_other;
	int s, i, slen=sizeof(si_other);
	struct timeval timeoutRecv = {timeout, 0};

	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}

  	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeoutRecv, sizeof(timeoutRecv));

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	
	if (inet_aton(host , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		close(s);
		return -1;
	}

	do
	{
		//printf("Enter message : ");
		//gets(message);
		
		
		//send the message
		if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
		{
			close(s);
			return -1;
		}
		
		//receive a reply and print it
		//clear the buffer by filling null, it might have previously received data
		memset(response,'\0', max_len);
		//try to receive some data, this is a blocking call
		if (recvfrom(s, response, max_len, 0, (struct sockaddr *) &si_other, &slen) == -1)
		{
			close(s);
			return -1;
		}
		
		//puts(response);
	}while(0);

	close(s);
	return 0;
}

int udp_download_command(const char * server, int port, int bandWidth, int PacketSize, int realPacketSize, int id)
{
	char message[BUFLEN];
	char buf[BUFLEN];	
	sprintf(message, "%d,%d,%d,%d", bandWidth, PacketSize, realPacketSize, id);
	memset(buf,'\0', BUFLEN);
	socket_message(server, port, message, buf, BUFLEN, 15);
	if(strlen(buf) > 0)
		return 0;
	else
		return -1;
}

int udp_download_command2(const char * server, int port, int bandWidth, int PacketSize, int realPacketSize, int TimeDuration, int delayMS, int id, int socketDelaySeconds)
{
	char message[BUFLEN];
	char buf[BUFLEN];	
	sprintf(message, "%d,%d,%d,%d,%d,%d", bandWidth, PacketSize, realPacketSize, TimeDuration, delayMS, id);
	memset(buf,'\0', BUFLEN);
	socket_message(server, port, message, buf, BUFLEN, socketDelaySeconds);
	if(strlen(buf) > 0)
		return 0;
	else
		return -1;
}

int send_ctrl_message_low(int sock, const char* host, int port, char* message, char response[], int max_len)
{
	struct sockaddr_in si_other;
	int i, slen=sizeof(si_other);

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	
	if (inet_aton(host , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		return -1;
	}

	do
	{
		//send the message
		if (sendto(sock, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
		{
			return -1;
		}
		
		//receive a reply and print it
		//clear the buffer by filling null, it might have previously received data
		memset(response,'\0', max_len);
		//try to receive some data, this is a blocking call
		if (recvfrom(sock, response, max_len, 0, (struct sockaddr *) &si_other, &slen) == -1)
		{
			return -1;
		}
		
		//puts(response);
	}while(0);

	return 0;
}

int send_ctrl_message(int sock, const char* host, int port, char* message, char response[], int max_len, int timeout)
{
	struct timeval timeoutRecv = {timeout, 0};

  	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeoutRecv, sizeof(timeoutRecv));

  	return send_ctrl_message_low(sock, host, port, message, response, max_len);
}

int recv_ctrl_message(int sock, const char* host, int port, char response[], int max_len, int timeout)
{
	struct sockaddr_in si_other;
	int i, slen=sizeof(si_other);
	struct timeval timeoutRecv = {timeout, 0};

  	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeoutRecv, sizeof(timeoutRecv));

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	
	if (inet_aton(host , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		return -1;
	}

	do
	{
		
		//receive a reply and print it
		//clear the buffer by filling null, it might have previously received data
		memset(response,'\0', max_len);
		//try to receive some data, this is a blocking call
		if (recvfrom(sock, response, max_len, 0, (struct sockaddr *) &si_other, &slen) == -1)
		{
			return -1;
		}
		
		//puts(response);
	}while(0);

	return 0;
}

int udp_packets_command(int sock, const char * server, int port)
{
	char message[BUFLEN];
	char buf[BUFLEN];	
	int packets_send = 0;
	sprintf(message, "packetnumv2");
	memset(buf,'\0', BUFLEN);
	send_ctrl_message(sock, server, port, message, buf, BUFLEN, 15);
	packets_send = atoi(buf);
	return packets_send;
}

int udp_packets_command2(int sock, const char * server, int port, int socketDelaySeconds)
{
	char message[BUFLEN];
	char buf[BUFLEN];	
	int packets_send = 0;
	sprintf(message, "packetnumv2");
	memset(buf,'\0', BUFLEN);
	send_ctrl_message(sock, server, port, message, buf, BUFLEN, socketDelaySeconds);
	packets_send = atoi(buf);
	return packets_send;
}

int udp_upload_test_command(int sock, const char * server, int port, int timeout)
{
	int ret = -1;
	char message[BUFLEN];
	char buf[BUFLEN];	
	int packets_send = 0;
	sprintf(message, "upload_test");
	memset(buf,'\0', BUFLEN);
	ret = send_ctrl_message(sock, server, port, message, buf, BUFLEN, timeout);
	if(ret == -1)
		return ret;
	packets_send = atoi(buf);
	return packets_send;
}

int udp_upload_result_command(int sock, const char * server, int port, int id, int timeout, char * response, int response_len)
{
	int ret = -1;
	char message[BUFLEN];
	int packets_send = 0;
	sprintf(message, "upload_result,%d", id);
	memset(response,'\0', BUFLEN);
	ret = send_ctrl_message(sock, server, port, message, response, response_len, timeout);
	if(ret == -1)
		return ret;
	return 0;
}

int udp_query_packet_count_command(int sock, const char * server, int port, int id, int timeout)
{
	int ret = -1;
	char message[BUFLEN];
	char buf[BUFLEN];	
	int packets_send = 0;
	sprintf(message, "query_count,%d", id);
	memset(buf,'\0', BUFLEN);
	ret = send_ctrl_message(sock, server, port, message, buf, BUFLEN, timeout);
	if(ret == -1)
		return ret;
	packets_send = atoi(buf);
	return packets_send;
}

int wait_ctrl_message(int sock, const char * server, int port)
{
	char buf[BUFLEN];	
	int packets_send = -1;

	sprintf(buf, "%d", packets_send);
	recv_ctrl_message(sock, server, port, buf, BUFLEN, 15);
	packets_send = atoi(buf);
	return packets_send;
}

#if 0
int main(int argc, char * argv[])
{

	char* server = SERVER;
	int port = PORT;
	int bandWidth = 100;
	int PacketSize = 1460;

	if(argc >= 1) server = argv[1];
	if(argc >= 2) port = atoi(argv[2]);
	if(argc >= 3) bandWidth = atoi(argv[3]);
	if(argc >= 4) PacketSize = atoi(argv[4]);

	udp_download_command(server, port, bandWidth, PacketSize);

}
#endif
