#pragma once

#include "noncopyable.h"
#include "Callbacks.h"
#include "Timestamp.h"
#include "InetAddress.h"
#include "Buffer.h"

#include <memory>
#include <atomic>
#include <string>

class EventLoop;
class Socket;
class Channel;

/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * =》 TcpConnection 设置回调 =》 Channel =》 Poller =》 Channel的回调操作
 *
 * 若一个类T继承std::enable_shared_from_this<T>，则会为该类T提供成员函数：shared_from_this()。
 * 当T类型对象t被一个为名为pt的std::shared_ptr<T>类对象管理时，调用T::shared_from_this成员函数，
 * 将会返回一个新的std::shared_ptr<T>对象，它与pt共享t的所有权。
 */
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    // 发送数据
    void send(const std::string &buf);
    // 关闭连接
    void shutdown();

    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    // 设置状态为连接（kConnected）
    bool connected() const { return state_ == kConnected; }

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    // 设置conn_socket连接状态
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *message, size_t len);
    void shutdownInLoop();

    EventLoop *loop_; // 这里绝对不是baseLoop， 因为TcpConnection都是在subLoop里面管理的
    const std::string name_;
    std::atomic_int state_; // conn_socket连接状态
    bool reading_;

    // 这里和Acceptor类似   Acceptor=》mainLoop    TcpConenction=》subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_;

    CloseCallback closeCallback_;

    size_t highWaterMark_;

    Buffer inputBuffer_;  // 接收数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
};