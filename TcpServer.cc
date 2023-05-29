#include "TcpServer.h"
#include <strings.h>
#include <functional>
#include "Logger.h"

EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option option)
    : loop_(CheckLoopNotNull(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0)
{
    // 当有新用户连接时，会执行TcpServer::newConnection回调(轮询)
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                  std::placeholders::_1, std::placeholders::_2));
}
TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        // 这个局部的shared_ptr智能指针对象，出右括号，可以自动释放new出来的TcpConnection对象资源
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

// 设置底层subloop的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听   loop.loop()
void TcpServer::start()
{
    if (started_++ == 0) // 防止一个TcpServer对象被start多次
    {
        /*
            线程池初始化--主线程（初始化mainloop）创建子线程（
            子线程调用threadInitCallback_初始化subLoop，即独立线程池**重点**）
            --创建loop--启动loop(loop.loop())--等待mainloop分发事件
            mainloop开始监听客户端事件
        */
        threadPool_->start(threadInitCallback_);
        // mainloop启动监听
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

/*
    有一个新的客户端的连接，acceptor会执行这个回调操作
    1.acceptor触发读事件，通过轮询选择一个subloop
    2.通过local获得sockaddr_in格式的地址信息并封装到InetAddress中
    创建connection对象将conn（sockfd）与  localAddr绑定
*/
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // 轮询算法，选择一个subLoop，来管理channel
    EventLoop *ioLoop = threadPool_->getNextLoop();

    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf; // name ip : port

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;

    // 往Local中填入地址
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    // 根据连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(
        ioLoop,
        connName,
        sockfd, // Socket Channel
        localAddr,
        peerAddr));

    connections_[connName] = conn;

    // 下面的回调都是用户设置给TcpServer=>TcpConnection=>Channel=>Poller=>notify channel调用回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置了如何关闭连接的回调   conn->shutDown()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 直接调用TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
             name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}