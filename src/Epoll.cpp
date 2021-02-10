#include "../include/Epoll.h"
#include "../include/HttpRequest.h"
#include "../include/ThreadPool.h"

#include <iostream>
#include <cassert>
#include <cstring> // perror

#include <unistd.h> // close

using namespace huhu;

Epoll::Epoll() 
    : m_epoll_fd(::epoll_create1(EPOLL_CLOEXEC)),
      m_eventlist(MAXEVENTS){
    assert(m_epoll_fd >= 0);
}

Epoll::~Epoll(){
    ::close(m_epoll_fd);
}

int Epoll::addFd(int fd, HttpRequest* request, int events){
    struct epoll_event event;
    event.data.ptr = static_cast<void*>(request); 
    event.events = events;
    int ret = ::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
    return ret;
}

int Epoll::modFd(int fd, HttpRequest* request, int events){
    struct epoll_event event;
    event.data.ptr = static_cast<void*>(request); 
    event.events = events;
    int ret = ::epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event);
    return ret;
}

int Epoll::delFd(int fd, HttpRequest* request, int events){
    struct epoll_event event;
    event.data.ptr = static_cast<void*>(request);
    event.events = events;
    int ret = ::epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, &event);
    return ret;
}

int Epoll::waitEvents(int time_ms){
    int events_num = ::epoll_wait(m_epoll_fd, &*m_eventlist.begin(), \
    static_cast<int>(m_eventlist.size()), time_ms);
    if(events_num == 0) {
        // printf("[Epoll::wait] nothing happen, epoll timeout\n");
    } else if(events_num < 0) {
        printf("[Epoll::wait] epoll : %s\n", strerror(errno));
    }
    
    return events_num;
}

void Epoll::handleEvent(int listen_fd, std::shared_ptr<ThreadPool>& thread_pool, int events_num){
    assert(events_num > 0);
    for(int i = 0; i < events_num; ++i) {
        HttpRequest* request = static_cast<HttpRequest*>(m_eventlist[i].data.ptr); 
        int fd = request->fd();

        if(fd == listen_fd) {
            m_new_conn_cb();
        } else {
            // check error
            if((m_eventlist[i].events & EPOLLERR) ||
               (m_eventlist[i].events & EPOLLHUP) ||
               (!m_eventlist[i].events & EPOLLIN)) {
                request->setStopWorking();
                m_close_conn_cb(request);

            } else if(m_eventlist[i].events & EPOLLIN) {
                request->setWorking();
                thread_pool->addTask(std::bind(m_req_cb, request));

            } else if(m_eventlist[i].events & EPOLLOUT) {
                request->setWorking();
                thread_pool->addTask(std::bind(m_res_cb, request));
                
            } else {
                printf("[Epoll::handleEvent] unexpected event\n");
            }
        }
    }
    return;
}
