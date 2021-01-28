#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <string>
#include <map>
#include <iostream>

#include "Buffer.h"

#define STATIC_ROOT "../web"

namespace huhu{

class Timer;
class HttpRequest{
public:
    enum HttpRequestParseStatus{
        ExpectRequestLine,
        ExpectRequestHeader,
        ExpectRequestBody,
        GotAll
    };

    enum HttpMethod{
        Invalid, Get, Post, Head, Put, Delete
    };

    enum HttpVersion{
        Unknown, HTTP10, HTTP11
    };

    HttpRequest(int fd);
    ~HttpRequest();

    int fd() {return m_fd;}
    int readData(int* savedErrno);
    int writeData(int* savedErrno);

    void appendOutBuffer(const Buffer& buffer);
    int writableBytes();

    void setTimer(Timer* timer);
    Timer* getTimer();

    void setWorking();
    void setStopWorking();
    bool isWorking() const;

    bool parseRequest();
    bool parseFinish();
    void resetParseStatus();

    std::string getPath() const;
    std::string getQuery() const;
    std::string getHeader(const std::string& field) const;
    std::string getMethod() const;
    bool keepAlive() const;

private:
    bool __parseRequestLine(const char* begin, const char* end);
    bool __setMethod(const char* begin, const char* end);
    void __setPath(const char* begin, const char* end);
    void __setQuery(const char* begin, const char* end); 
    void __setVersion(HttpVersion version);
    void __addHeader(const char* begin, const char* colon, const char* end);

private:

    int m_fd;
    Buffer m_in_buffer;
    Buffer m_out_buffer;
    bool m_working;

    Timer* m_timer;

    HttpRequestParseStatus m_status;
    HttpMethod m_method;
    HttpVersion m_version;
    std::string m_path;
    std::string m_query;
    std::map<std::string, std::string> m_header;

};

}
#endif//__HTTP_REQUEST_H__