#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <iostream>
using namespace std;

// 封装socket地址类型
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {
    }

    // 获取IP
    string toIp() const;

    // 获取IPPort的信息
    string toIpPort() const;

    // 获取Port端口号
    uint16_t toPort() const;

    // 获取成员变量
    const sockaddr_in *getSockAddr() const
    {
        return &addr_;
    };

    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};