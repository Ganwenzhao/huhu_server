#ifndef __EPOLL_H__
#define __EPOLL_H__

#include <vector>
#include <memory>
#include <functional>

#include <sys/epoll.h>
#define MAXEVENTS 4096

namespace huhu {

class HttpRequest;
class ThreadPool;

class Epoll {
public:
    using NewConnectionCallback = std::function<void()>;
    using CloseConnectionCallback = std::function<void(HttpRequest*)>;
    using HandleRequestCallback = std::function<void(HttpRequest*)>;
    using HandleResponseCallback = std::function<void(HttpRequest*)>;

    Epoll();
    ~Epoll();
    //epoll_ctl
    int addFd(int fd, HttpRequest* request, int events);
    int modFd(int fd, HttpRequest* request, int events);
    int delFd(int fd, HttpRequest* request, int events);
    //epoll_wait
    int waitEvents(int time_ms);

    void handleEvent(int listen_fd, std::unique_ptr<ThreadPool>& thread_pool, int events_num);

    void setNewConnection(const NewConnectionCallback& cb){
        m_new_conn_cb = cb;
    }
    void setCloseConnection(const CloseConnectionCallback& cb){
        m_close_conn_cb = cb;
    }
    void setRequest(const HandleRequestCallback& cb){
        m_req_cb = cb;
    }
    void setResponse(const HandleResponseCallback& cb){
        m_res_cb = cb;
    }

private:
    using EventList = std::vector<struct epoll_event>;

    int m_epoll_fd;
    EventList m_eventlist;
    NewConnectionCallback m_new_conn_cb;
    CloseConnectionCallback m_close_conn_cb;
    HandleRequestCallback m_req_cb;
    HandleResponseCallback m_res_cb;

};//class epoll

}//namespace huhu

#endif