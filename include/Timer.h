#ifndef __TIMER_H__
#define __TIMER_H__

#include <iostream>
#include <cassert>
#include <functional>
#include <mutex>
#include <vector>
#include <queue>
#include <chrono>

namespace huhu{

using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using Timestamp = Clock::time_point;

class HttpRequest;

class Timer {
public:
    Timer(const Timestamp& when, const TimeoutCallBack& cb)
        :m_expire_time(when),
        m_timer_callback(cb),
        m_delete(false){}

    ~Timer(){}

    void del() {m_delete = true;}
    bool isDeleted() {return m_delete;}
    Timestamp getExpireTime() const {return m_expire_time;}
    void runCallBack() {m_timer_callback();}

private:
    Timestamp m_expire_time;
    TimeoutCallBack m_timer_callback;
    bool m_delete;

};// class Timer

struct cmp{
    //functor
    bool operator()(Timer* a, Timer* b){
        assert(a != nullptr && b != nullptr);
        return (a->getExpireTime()) > (b->getExpireTime());
    }
};

class TimerManager{
public:
    TimerManager()
        :m_now(Clock::now()){}
    ~TimerManager(){}

    void updateTime() {m_now = Clock::now();}
    void addTimer(HttpRequest* request, const int& timeout, const TimeoutCallBack& cb);
    void delTimer(HttpRequest* request);
    void handleExpireTimer();
    int getNextExpireTime();

private:
    using TimerQueue = std::priority_queue<Timer*, std::vector<Timer*>, cmp>;
    TimerQueue m_timer_queue;
    Timestamp m_now;
    std::mutex m_lock;
};//class TimerManager

}// namespace huhu

#endif //__TIMER_H__