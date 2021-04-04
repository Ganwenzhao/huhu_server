#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <memory>
#include <mutex>

#define EPOLL_TIMEOUT_MS  -1//epoll_wait timeout
#define CONNECT_TIMEOUT  1500
#define WORKERS_NUM  4

namespace huhu{

class HttpRequest;
class Epoll;
class ThreadPool;
class TimerManager;

class HttpServer{
public:
    HttpServer(int port, int thread_num);
    ~HttpServer();
    void runHuHu();

private:
    void __acceptConnection();
    void __closeConnection(HttpRequest* request);
    void __doRequest(HttpRequest* request);
    void __doResponse(HttpRequest* request);

private:
    using ListenRequestPtr = std::unique_ptr<HttpRequest>;
    using EpollPtr = std::unique_ptr<Epoll>;
    using ThreadPoolPtr = std::unique_ptr<ThreadPool>;
    using TimerManagerPtr = std::unique_ptr<TimerManager>;

    int m_port;
    int m_listen_fd;
    std::mutex m_mtx;
    ListenRequestPtr m_listen_request;
    EpollPtr m_epoll;
    ThreadPoolPtr m_threadpool;
    TimerManagerPtr m_timer_manager;

};//class HttpServer

}//namespace huhu

#endif