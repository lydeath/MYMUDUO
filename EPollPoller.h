#pragma once

#include "Poller.h"
#include <vector>
#include <sys/epoll.h>

class Channel;

/**
 * epoll的使用
 * epoll_create
 * epoll_ctl   add/mod/del
 * epoll_wait
 */
// 类不写继承方式，默认为private继承,struct继承默认为public继承
// 外部对象要调用poll中的newDefaultPoller函数,所以必须用public继承
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    // 注册所有channel
    //如果派生类在虚函数声明时使用了override描述符，
    //那么该函数必须重载其基类中的同名函数，否则代码将无法通过编译。

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    // 更新对应channel
    void updateChannel(Channel *channel) override;
    // 删除对应channel
    void removeChannel(Channel *channel) override;

private:
    static const int KInitEventListSize = 16;

    using EventList = std::vector<epoll_event>;

        // 填写活跃的连接
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

        // 更新channel通道
        void update(int operation, Channel *channel);

        int epollfd_;
        EventList events_;
};