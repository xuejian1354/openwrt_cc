#include "PhysicalSocket.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <WS2tcpip.h>

PhysicalSocket::PhysicalSocket(void):m_nonblocking(0)
{
	m_bReadFdIset = false;
	m_bWriteFdIset = false;
	error_ = 0;
	s_ = -1;
	udp_ = false;
	state_ = CS_CLOSED;
}


PhysicalSocket::~PhysicalSocket(void)
{
//	if (state_ != SC_CLOSE)
	if (state_ != CS_CLOSED)
	{
		Close();
	}
}

// Creates the underlying OS socket (same as the "socket" function).
//SOCK_STREAM,SOCK_DGRAM
bool PhysicalSocket::Create(int type) {
	printf("ZLZ - %s, %d , %s, B.F.\n", __FILE__, __LINE__, strerror(errno));
	Close();

	printf("ZLZ - %s, %d , %s, B.F.\n", __FILE__, __LINE__, strerror(errno));

	s_ = ::socket(AF_INET, type, 0);
	udp_ = (SOCK_DGRAM == type);
	UpdateLastError();
	//int on=1;
	//setsockopt(s_ ,SOL_SOCKET,SO_REUSEADDR | SO_BROADCAST,&on,sizeof(on));

	printf("%d , %s A.F.\n", __LINE__, strerror(errno));

	return s_ != -1;
}


int PhysicalSocket::Bind(const sockaddr_in &saddr) {
	int err = ::bind(s_, (sockaddr*)&saddr, sizeof(saddr));
	UpdateLastError();

	return err;
}

int PhysicalSocket::Connect(const sockaddr_in &addr) {
	// TODO: Implicit creation is required to reconnect...
	// ...but should we make it more explicit?
	if ((s_ == -1) && !Create(SOCK_STREAM))
		return -1;

	return DoConnect(addr);
}

int PhysicalSocket::DoConnect(const sockaddr_in &saddr) {

	int err = ::connect(s_, (sockaddr*)&saddr, sizeof(saddr));
	if (m_nonblocking == 0)
	{
		UpdateLastError();
		if (err == 0) {
			state_ = CS_CONNECTED;
		} else if (IsBlockingError(error_)) {
			state_ = CS_CONNECTING;
		} else {
			return -1;
		}
	}else
	{
		if (err == -1)
		{
			fd_set FDSET;
			FD_ZERO(&FDSET);
			FD_SET(s_, &FDSET);
			struct timeval tv;
			// 设置连接超时时间

			tv.tv_sec = 25000; // 秒数
			tv.tv_usec = 0; // 毫秒
			int nError = ::select(s_+1,0,&FDSET,0,&tv);
			UpdateLastError();
			if (nError <= 0) {
				return -1;
			}
			state_ = CS_CONNECTED;
		}
		else 
			return -1;
	}
	
	return 0;
}

int PhysicalSocket::GetOption(Option opt, int* value) {
	int slevel;
	int sopt;
	if (TranslateOption(opt, &slevel, &sopt) == -1)
		return -1;
	socklen_t optlen = sizeof(*value);
	int ret = ::getsockopt(s_, slevel, sopt, (char *)value, &optlen);

	return ret;
}

int PhysicalSocket::SetOption(Option opt, int value) {
	int slevel;
	int sopt;
	if (TranslateOption(opt, &slevel, &sopt) == -1)
		return -1;

	return ::setsockopt(s_, slevel, sopt, (char *)&value, sizeof(value));
}

int PhysicalSocket::Send(const void *pv, int cb) {

	int sent = ::send(s_, reinterpret_cast<const char *>(pv), (int)cb,	0);
	if (!udp_)
	{
		UpdateLastError();
	}
	if (udp_ && sent < 0)
	{
		UpdateLastError();
	}
	

	return sent;
}

int PhysicalSocket::SendTo(const void *pv, int cb, const sockaddr_in &saddr) {
	errno = 0;
	int sent = ::sendto(s_, (const char *)pv, (int)cb,	0,	(sockaddr*)&saddr, sizeof(saddr));
	if (!udp_)
	{
		UpdateLastError();
	}

	if (udp_ && sent < 0)
	{
		UpdateLastError();
	}
	return sent;
}

int PhysicalSocket::Recv(void *pv, int cb) {

	int received = ::recv(s_, (char *)pv, (int)cb, 0);

	UpdateLastError();

	return received;
}

int PhysicalSocket::RecvFrom(void *pv, int cb, sockaddr_in &saddr) {
	socklen_t cbAddr = sizeof(saddr);
	int received = ::recvfrom(s_, (char *)pv, (int)cb, 0, (struct sockaddr*)&saddr, &cbAddr);
	UpdateLastError();

	return received;
}

int PhysicalSocket::Listen(int backlog) {

	int err = listen(s_, backlog);
	UpdateLastError();
	if (err == 0) {
		state_ = CS_CONNECTING;
	}
	return err;
}

int PhysicalSocket::Accept(sockaddr_in& saddr) {
	socklen_t cbAddr = sizeof(saddr);
	int s = accept(s_, (sockaddr*)&saddr, &cbAddr);
	UpdateLastError();

	return s;
}

int PhysicalSocket::Close() {
	if (s_ == -1)
		return 0;
	//int err = closesocket(s_);
	int err = close(s_);
	UpdateLastError();
	s_ = -1;
	state_ = CS_CLOSED;

	return err;
}

void PhysicalSocket::UpdateLastError() {
	//error_ = LAST_SYSTEM_ERROR;
	error_ = errno;
}

int PhysicalSocket::TranslateOption(Option opt, int* slevel, int* sopt) 
{
	switch (opt) {
	case OPT_DONTFRAGMENT:
		*slevel = IPPROTO_IP;
		//*sopt = IP_DONTFRAGMENT;
		*sopt = IP_MTU_DISCOVER;
		break;
	case OPT_RCVBUF:
		*slevel = SOL_SOCKET;
		*sopt = SO_RCVBUF;
		break;
	case OPT_SNDBUF:
		*slevel = SOL_SOCKET;
		*sopt = SO_SNDBUF;
		break;
	case OPT_NODELAY:
		*slevel = IPPROTO_TCP;
		*sopt = TCP_NODELAY;
		break;
	default:
		return -1;
	}
	return 0;
}
int PhysicalSocket::GetError() const {
	return error_;
}

int PhysicalSocket::GetState() const {
	return state_;
}

int PhysicalSocket::GetSocket() const
{
	return s_;
}
bool PhysicalSocket::SetSocketBlock(unsigned long ln)
{
	int flag;
/*
	m_nonblocking = ln;
	int x = ioctlsocket(s_, FIONBIO, &m_nonblocking);
	if (x == -1) // 设置为阻塞方式
	{
		return false;
	}
*/
//FIXME
	flag = fcntl(s_, F_GETFL, 0);
	if (ln) {
		fcntl(s_, F_SETFL, (flag | O_NONBLOCK));
	}
	else {
		fcntl(s_, F_SETFL, (flag & (~O_NONBLOCK)));
	}
	return true;
}

int PhysicalSocket::Select(bool bread, bool bwrite, struct timeval * timeout )
{
	m_bReadFdIset = false;
	m_bWriteFdIset = false;

	if (bread)
	{
		FD_ZERO(&fdsRead);
		FD_SET(s_,&fdsRead);
	}
	if (bwrite)
	{
		FD_ZERO(&fdsWrite);
		FD_SET(s_,&fdsWrite);
	}

	fd_set fdserr;
	FD_ZERO(&fdserr);
	FD_SET(s_,&fdserr);
	int n = select(s_ + 1, bread ? &fdsRead : NULL, bwrite? &fdsWrite : NULL, &fdserr ,timeout);

	if (n > 0)
	{
		if (bread && FD_ISSET(s_,&fdsRead))
		{
			FD_CLR(s_,&fdsRead);
			m_bReadFdIset = true;
		}
		if (bwrite && FD_ISSET(s_,&fdsWrite))
		{
			FD_CLR(s_,&fdsWrite);
			m_bWriteFdIset = true;
		}
		if(FD_ISSET(s_,&fdserr))
		{
			FD_CLR(s_,&fdserr);
			n = -1;
			printf("select fdserr exist\n");
		}
	}
	else if(n == 0)
	{
		printf("select timeout\n");
	}
	else
	{
		printf("select erro,%s\n",strerror(errno));
	}
	return n;
	
}
