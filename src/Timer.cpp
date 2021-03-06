#include "../include/Timer.h"
#include "../include/HttpRequest.h"

#include <cassert>

using namespace huhu;
    

void TimerManager::addTimer(HttpRequest* request, 
                     const int& timeout, 
                     const TimeoutCallBack& cb){
    std::unique_lock<std::mutex> lock(m_lock);
    assert(request != nullptr);

    updateTime();
    Timer* timer = new Timer(m_now + MS(timeout), cb);
    m_timer_queue.push(timer);

    // Call addTimer twice for the same request, you need to remove the previous timer
    if(request->getTimer() != nullptr)
        delTimer(request);

    request->setTimer(timer);
}

//handleExpireTimers->runCallBack->__closeConnection->delTimer
void TimerManager::delTimer(HttpRequest* request){
    assert(request != nullptr);

    Timer* timer = request->getTimer();
    if(timer == nullptr)
        return;
    //lazy delete
    timer->del();

    request->setTimer(nullptr);
}

void TimerManager::handleExpireTimer(){
    updateTime();
    while(!m_timer_queue.empty()) {
        Timer* timer = m_timer_queue.top();
        assert(timer != nullptr);
        
        if(timer->isDeleted()) {
            m_timer_queue.pop();
            if(timer != nullptr){
                delete timer;
            }
            continue;
        }

        // check pri_queue front，if not timeout, return
        if(std::chrono::duration_cast<MS>(timer->getExpireTime() - m_now).count() > 0) {
            return;
        }
        // timeout
        timer->runCallBack();
        
    }
}

int TimerManager::getNextExpireTime(){
    //std::unique_lock<std::mutex> lock(m_lock);
    updateTime();
    int res = -1;
    while(!m_timer_queue.empty()) {
        Timer* timer = m_timer_queue.top();
        if(timer->isDeleted()) {
            m_timer_queue.pop();
            if(timer != nullptr){
                delete timer;
                timer = nullptr;
            }
            continue;
        }
        res = std::chrono::duration_cast<MS>(timer->getExpireTime() - m_now).count();
        res = (res < 0) ? 0 : res;
        break;
    }
    return res;
}
