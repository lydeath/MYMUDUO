#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0)
{
}
EventLoopThreadPool::~EventLoopThreadPool()
{
    // 线程底层创建的事件循环对象loop在线程栈上自动析构
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) // cb为初始化loop函数callback_
{
    started_ = true;

    for (int i = 0; i < numThreads_; i++)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);

        // 创建子线程
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // t->startLoop() 创建事件循环loop，并放入loops_列表
        loops_.push_back(t->startLoop());
    }

    // 整个服务端只有一个线程，运行着baseloop
    if(numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

// 如果工作在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseLoop_;

    //若不止一个loop
    if(!loops_.empty()) // 通过轮询获取下一个处理事件的loop
    {   
        loop = loops_[next_];   //next_初始化为0
        ++next_;
        if(next_ >= loops_.size())  //如果next_到集合尾端，重置其置列表开头
        {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if(loops_.empty())
    {
        return std::vector<EventLoop*>(1, baseLoop_);//初始化1个元素，值为baseLoop_
    }
    else
    {
        loops_;
    }
}