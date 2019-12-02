#ifndef PHYSICALSOCKET_H
#define PHYSICALSOCKET_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>

inline bool IsBlockingError(int e) {
	return (e == EWOULDBLOCK) || (e == EAGAIN) || (e == EINPROGRESS);
}

class PhysicalSocket
{
public:
	PhysicalSocket(void);
	~PhysicalSocket(void);
	
	enum ConnState {
		CS_CLOSED,
		CS_CONNECTING,
		CS_CONNECTED
	};

	enum Option {
		OPT_DONTFRAGMENT,
		OPT_RCVBUF,  // receive buffer size
		OPT_SNDBUF,  // send buffer size
		OPT_NODELAY  // whether Nagle algorithm is enabled
	};
public:
	static int TranslateOption(Option opt, int* slevel, int* sopt);
	void UpdateLastError();
	int Close();
	int Accept(sockaddr_in& saddr) ;
	int Listen(int backlog) ;
	int Select(bool bread, bool bwrite, struct timeval * timeout );
	int RecvFrom(void *pv, int cb, sockaddr_in& saddr);
	int Recv(void *pv, int cb);
	int SendTo(const void *pv, int cb, const sockaddr_in& saddr) ;
	int Send(const void *pv, int cb);
	int SetOption(Option opt, int value) ;
	int GetOption(Option opt, int* value) ;
	int DoConnect(const sockaddr_in& saddr);
	int Connect(const sockaddr_in& saddr) ;
	int Bind(const sockaddr_in& saddr) ;
	bool Create(int type);
	bool SetSocketBlock(unsigned long ln);
	int GetError() const ;
	int GetState() const ;
	int GetSocket() const;

	bool m_bReadFdIset;
	bool m_bWriteFdIset;
private:
	int error_;
	int s_;
	bool udp_;
	int state_;
	unsigned long m_nonblocking;//×èÈû0£¬²»×èÈû1£¬Ä¬ÈÏ×èÈû
	fd_set fdsRead;
	fd_set fdsWrite;
};

#endif