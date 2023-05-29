#include "InetAddress.h"
#include <string.h>


    InetAddress::InetAddress(uint16_t port, string ip){
        memset(&addr_, 0, sizeof addr_);
        addr_.sin_family = AF_INET;
        //htons() 是主机字节序到网络字节序之间转换的函数
        addr_.sin_port = htons(port);
        // inet_addr函数将网络主机地址cp从 IPv4 的数字点表示形式转换为以网络字节顺序的二进制形式
        addr_.sin_addr.s_addr = inet_addr(ip.c_str());  //转成网络字节序存储下来
    }


    // 获取IP
    string InetAddress::toIp() const
    {
        //  addr_
        char buf[64] = {0};
        //inet_ntop函数是将网络字节序二进制值转换成点分十进制串
        ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);   
        return buf;
    }

    // 获取IPPort的信息
    string InetAddress::toIpPort() const
    {
        //  ip:port
        char buf[64] = {0};
        ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
        size_t end = strlen(buf);
        //网络字节序到主机字节序的转换
        uint16_t port = ntohs(addr_.sin_port);
        sprintf(buf+end, ":%u", port);
        return buf;
    }

    // 获取Port端口号
    uint16_t InetAddress::toPort() const
    {
        return ntohs(addr_.sin_port);
    }

    int main()
    {
        InetAddress addr(8080);
        cout << addr.toIpPort() << endl;
        return 0;
    }