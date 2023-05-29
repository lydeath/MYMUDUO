#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb)
{
}

/*
主线程创建并启动子线程，如果自线程中要进行大量的耗时运算，
主线程往往将早于子线程结束之前结束。如果主线程想等待子线程
执行完成之后再结束，比如子线程处理一个数据，主线程要取得这
个数据中的值，就要用到 join() 方法

 join()方法的作用就是让主线程等待子线程执行结束之后再运行主线程。下面示例中t2 为主线程，需要等待子线程t1 执行完成再执行
使用场景，线程2依赖于线程1执行的返回结果
在线程2 中调用线程1的join方法，当线程调用了这个方法时，线程1会强占CPU资源，直到线程执行结果为止（谁调用join方法，谁就强占cpu资源，直至执行结束）
这里说的是强占，而不是抢占，也就是说当这个线程调用 了join方法后，线程抢占到CPU资源，它就不会再释放，直到线程执行完毕
*/

// 调用此函数会获得一个新线程，同时会把新线程的loop返回回去
//主线程创建子线程，主线程运行结束，调用子线程调用join()，使子线程执行完毕后再关闭主线程
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();  // one loop per thread
        thread_.join(); // 主线程退出以后等待底层的子线程thread_结束
    }
}


EventLoop *EventLoopThread::startLoop()
{

    //创建子线程
    thread_.start(); 
   
   EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock); // 在此等待线程创建完毕
        }
        loop = loop_;
    }
    return loop;
}

//threadFunc()即Thread.cc线程中的Func方法
// 下面这个方法，是在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的eventloop，和上面的线程是一一对应的，one loop per thread

    if (callback_) // 事件循环初始化
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop  => Poller.poll

    //loop.loop()结束后线程退出
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}