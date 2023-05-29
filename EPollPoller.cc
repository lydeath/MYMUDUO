#include "EPollPoller.h"
#include "Logger.h"
#include "Timestamp.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

// channel未添加到poller中
const int kNew = -1; // channel的成员index_ 初始化= -1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(KInitEventListSize) // vector<epoll_event>
{
    if (epollfd_ < 0)
    {
        // epoll树创建失败
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// 注册所有channel,将其放入channelList列表中
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 实际上应该用LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());

    // 当事件发生,返回发生的事件数量
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;           // 保存错误类型
    Timestamp now(Timestamp::now()); // 现在时间

    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);  // 事件发生
        fillActiveChannels(numEvents, activeChannels); // 将通道加入channels_列表
        if (numEvents == events_.size())
        {                                       // 当事件数量到达事件集合列表的最大值
            events_.resize(events_.size() * 2); // 事件列表扩充一倍
        }
    }
    else if (numEvents == 0)
    {
        // 如果无事件发生,超时
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        /*
        如果错误为EINTR表示在读/写的时候出现了中断错误
        read（）如果读到数据为0，那么就表示文件结束了，如果在读的过程中遇到了中断那么会返回-1，同时置errno为EINTR。
        或者是write()如果写的过程中遇到中断就会返回-1 并设置errno为EINTR
        */
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; i++)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr); // 此处events.data.ptr中存放的是fd对应的channel
        channel->set_revents(events_[i].events);
        // EventLoop就拿到了它的poller给它返回的所有发生事件的channel列表了
        activeChannels->push_back(channel);
    }
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
/**
 *            EventLoop  =>   poller.poll
 *     ChannelList      Poller
 *                     ChannelMap  <fd, channel*>   epollfd
 */

/*
// channel未添加到poller中
const int kNew = -1; // channel的成员index_ 初始化= -1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;
*/
// 更新对应channel
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index(); // index默认-1
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    // 如果channel未添加或从poller中删除
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew) // 如果channel未添加到poller中
        {
            int fd = channel->fd();
            channels_[fd] = channel; // 放入channelList中
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel); // 通道上树
    }
    else // channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if (channel->isNoneEvent()) // 如果channel中没有事件
        {
            update(EPOLL_CTL_DEL, channel); // 通道下树，暂时不监控，下次事件到来再上树
            channel->set_index(kDeleted);
        }
        else
        {
            // EPOLL_CTL_MOD : 修改描述符上设定的事件，需要用到由ev所指向的结构体中的信息
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 删除对应channel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd); // 删除channellist中的channel

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew); // 将channel设置为未插入
}

// 更新channel通道
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof event);

    int fd = channel->fd();

    // 更新event
    event.events = channel->events(); // fd感兴趣的所有事件
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) // 上树
    {                                                     // 如果上树失败
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}