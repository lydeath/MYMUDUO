#pragma once // 防止头文件被重复包含

/*
    noncopyable被继承以后，派生类对象可以正常的构造和析构，但是派生类对象
    无法进行拷贝构造和赋值构造
*/
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;


};