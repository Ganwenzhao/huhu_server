#include "../include/HttpServer.h"
#include "../include/HttpRequest.h"
#include "../include/HttpResponse.h"
#include "../include/Utils.h"
#include "../include/Epoll.h"
#include "../include/ThreadPool.h"
#include "../include/Timer.h"

#include <iostream>
#include <functional> // bind
#include <cassert> // assert
#include <cstring> // bzero
 
#include <unistd.h> // close, read
#include <sys/socket.h> // accept
#include <arpa/inet.h> // sockaddr_in

using namespace huhu;

HttpServer::HttpServer(int port, int thread_num) 
    : m_port(port),
      m_listen_fd(utils::createListenFd(m_port)),
      m_listen_request(new HttpRequest(m_listen_fd)),
      m_epoll(new Epoll()),
      m_threadpool(new ThreadPool(thread_num)),
      m_timer_manager(new TimerManager()){
    assert(m_listen_fd >= 0);
}

HttpServer::~HttpServer(){}

void HttpServer::runHuHu(){
    m_epoll->addFd(m_listen_fd, m_listen_request.get(), (EPOLLIN | EPOLLET));
    m_epoll->setNewConnection(std::bind(&HttpServer::__acceptConnection, this));
    m_epoll->setCloseConnection(std::bind(&HttpServer::__closeConnection, this, \
    std::placeholders::_1));
    m_epoll->setRequest(std::bind(&HttpServer::__doRequest, this, \
    std::placeholders::_1));
    m_epoll->setResponse(std::bind(&HttpServer::__doResponse, this, \
    std::placeholders::_1));

    // event loop
    while(1) {
        int time_ms = m_timer_manager->getNextExpireTime();

        int events_num = m_epoll->waitEvents(time_ms);

        if(events_num > 0) {
            // event dispatch
            m_epoll->handleEvent(m_listen_fd, m_threadpool, events_num);
        }
        
        m_timer_manager->handleExpireTimer(); 
    }
}

// ET acceptConn, LT IO
void HttpServer::__acceptConnection(){
    while(1) {
        int accept_fd = ::accept4(m_listen_fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if(accept_fd == -1) {
            if(errno == EAGAIN)
                break;
            printf("[HttpServer::__acceptConnection] accept : %s\n", strerror(errno));
            break;
        }
        HttpRequest* request = new HttpRequest(accept_fd);
        m_timer_manager->addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpServer::__closeConnection, this, request));
        //EPOLLIN, level trigger, oneshot
        m_epoll->addFd(accept_fd, request, (EPOLLIN | EPOLLONESHOT));
    }
}

void HttpServer::__closeConnection(HttpRequest* request){
    int fd = request->fd();
    if(request->isWorking()) {
        return;
    }
    m_timer_manager->delTimer(request);
    m_epoll->delFd(fd, request, 0);
    if(request != nullptr){
        delete request;
        request = nullptr;
    }
}

// LT
void HttpServer::__doRequest(HttpRequest* request){
    m_timer_manager->delTimer(request);
    assert(request != nullptr);
    int fd = request->fd();

    int read_error;
    int n_read = request->readData(&read_error);

    // client disconnection
    if(n_read == 0) {
        request->setStopWorking();
        __closeConnection(request);
        return; 
    }

    // connection error
    if(n_read < 0 && (read_error != EAGAIN)) {
        request->setStopWorking();
        __closeConnection(request);
        return; 
    }

    // EAGAIN error, m_epoll->mod(...)
    if(n_read < 0 && read_error == EAGAIN) {
        m_epoll->modFd(fd, request, (EPOLLIN | EPOLLONESHOT));
        request->setStopWorking();
        m_timer_manager->addTimer(request, CONNECT_TIMEOUT, \
        std::bind(&HttpServer::__closeConnection, this, request));

        return;
    }

    // parse request
    if(!request->parseRequest()) {
        
        HttpResponse response(400, "", false);
        request->appendOutBuffer(response.makeResponse());

        int write_error;
        request->writeData(&write_error);
        request->setStopWorking();
        __closeConnection(request); 
        return; 
    }

    if(request->parseFinish()) {
        HttpResponse response(200, request->getPath(), request->keepAlive());
        request->appendOutBuffer(response.makeResponse());
        m_epoll->modFd(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
    }
}

// LT
void HttpServer::__doResponse(HttpRequest* request){
    m_timer_manager->delTimer(request);
    assert(request != nullptr);
    int fd = request->fd();

    int to_write = request->writableBytes();

    if(to_write == 0) {
        m_epoll->modFd(fd, request, (EPOLLIN | EPOLLONESHOT));
        request->setStopWorking();
        m_timer_manager->addTimer(request, CONNECT_TIMEOUT, \
        std::bind(&HttpServer::__closeConnection, this, request));
        return;
    }

    int write_error;
    int ret = request->writeData(&write_error);

    if(ret < 0 && write_error == EAGAIN) {
        m_epoll->modFd(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
        return;
    }

    if(ret < 0 && (write_error != EAGAIN)) {
        request->setStopWorking();
        __closeConnection(request);
        return;
    }

    if(ret == to_write) {
        if(request->keepAlive()) {
            request->resetParseStatus();
            m_epoll->modFd(fd, request, (EPOLLIN | EPOLLONESHOT));
            request->setStopWorking();
            m_timer_manager->addTimer(request, CONNECT_TIMEOUT, \
            std::bind(&HttpServer::__closeConnection, this, request));
        } else {
            request->setStopWorking();
            __closeConnection(request);
        }
        return;
    }

    m_epoll->modFd(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
    request->setStopWorking();
    m_timer_manager->addTimer(request, CONNECT_TIMEOUT, \
    std::bind(&HttpServer::__closeConnection, this, request));
    return;
}
