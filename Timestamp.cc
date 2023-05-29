#include "Timestamp.h"

#include <time.h>
using namespace std;

Timestamp::Timestamp()
:microSecondsSInceEpoch_(0)
{
}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch_)
:microSecondsSInceEpoch_ (microSecondsSInceEpoch_)
{
}

Timestamp Timestamp::now()
{
    // time_t ti = time(NULL);
    return Timestamp(time(NULL));
}

string Timestamp::toString() const
{
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSInceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
            return buf;
}
