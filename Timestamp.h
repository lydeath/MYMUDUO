#pragma once

#include <string>
using namespace std;


//时间类
class Timestamp
{
    public:
        Timestamp();
        //为了能够更好的控制函数行为，禁止隐式转换
        explicit Timestamp(int64_t microSecondsSinceEpoch_);
        static Timestamp now();
        string toString() const;

    private:
        int64_t microSecondsSInceEpoch_;
};