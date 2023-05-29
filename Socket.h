#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }

    ~Socket();

    //获得当前sockfd
    int fd() const {return sockfd_;}

    //绑定地址
    void bindAddress(const InetAddress &localaddr);

    //监听当前socketfd
    void listen();

    //accept客户端数据并创建新socket
    int accept(InetAddress *peeraddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};