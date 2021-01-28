#include "../include/HttpRequest.h"

#include <iostream>
#include <cassert>

#include <unistd.h>

using namespace huhu;

HttpRequest::HttpRequest(int fd)
    : m_fd(fd),
      m_working(false),
      m_timer(nullptr),
      m_status(ExpectRequestLine),
      m_method(Invalid),
      m_version(Unknown)
{
    assert(m_fd >= 0);
}

HttpRequest::~HttpRequest()
{
    ::close(m_fd);
}

int HttpRequest::readData(int* savedErrno)
{
    int ret = m_in_buffer.readFd(m_fd, savedErrno);
    return ret;
}

int HttpRequest::writeData(int* savedErrno)
{
    int ret = m_out_buffer.writeFd(m_fd, savedErrno);
    return ret;
}

void HttpRequest::appendOutBuffer(const Buffer& buffer){
    m_out_buffer.append(buffer);
}

int HttpRequest::writableBytes(){
    return m_out_buffer.readableBytes();
}

void HttpRequest::setTimer(Timer* timer){
    m_timer = timer;
}

Timer* HttpRequest::getTimer(){
    return m_timer;
}

void HttpRequest::setWorking(){
    m_working = true;
}

void HttpRequest::setStopWorking(){
    m_working = false;
}

bool HttpRequest::isWorking() const{
    return m_working;
}


bool HttpRequest::parseRequest()
{
    bool ok = true;
    bool hasMore = true;

    while(hasMore) {
        if(m_status == ExpectRequestLine) {
            // handle request Line
            const char* crlf = m_in_buffer.findCRLF();
            if(crlf) {
                ok = __parseRequestLine(m_in_buffer.peek(), crlf);
                if(ok) {
                    m_in_buffer.retrieveToEnd(crlf + 2);
                    m_status = ExpectRequestHeader;
                } else {
                    hasMore = false;
                }
            } else {
                hasMore = false;
            }
        } else if(m_status == ExpectRequestHeader) {
            // handle request Header
            const char* crlf = m_in_buffer.findCRLF();
            if(crlf) {
                const char* colon = std::find(m_in_buffer.peek(), crlf, ':');
                if(colon != crlf) {
                    __addHeader(m_in_buffer.peek(), colon, crlf);
                } else {
                    m_status = GotAll;
                    hasMore = false;
                }
                m_in_buffer.retrieveToEnd(crlf + 2);
            } else {
                hasMore = false;
            } 
        } else if(m_status == ExpectRequestBody) {
            // handle request body
        }
    }

    return ok;
}

bool HttpRequest::parseFinish(){
    return m_status == GotAll;
}

void HttpRequest::resetParseStatus()
{
    m_status = ExpectRequestLine; 
    m_method = Invalid; 
    m_version = Unknown; 
    m_path = ""; 
    m_query = ""; 
    m_header.clear(); 
}

std::string HttpRequest::getPath() const{
    return m_path;
}

std::string HttpRequest::getQuery() const{
    return m_query;
}

std::string HttpRequest::getHeader(const std::string& field) const
{
    std::string res;
    auto itr = m_header.find(field);
    if(itr != m_header.end())
        res = itr->second;
    return res;
}

std::string HttpRequest::getMethod() const
{
    std::string res;
    if(m_method == Get)
        res = "GET";
    else if(m_method == Post)
        res = "POST";
    else if(m_method == Head)
        res = "HEAD";
    else if(m_method == Put)
        res = "Put";
    else if(m_method == Delete)
        res = "DELETE";
    
    return res;
}

bool HttpRequest::keepAlive() const
{
    std::string connection = getHeader("Connection");
    bool res = connection == "Keep-Alive" || 
               (m_version == HTTP11 && connection != "close");

    return res;
}

bool HttpRequest::__parseRequestLine(const char* begin, const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    if(space != end && __setMethod(start, space)) {
        start = space + 1;
        space = std::find(start, end, ' ');
        if(space != end) {
            const char* question = std::find(start, space, '?');
            if(question != space) {
                __setPath(start, question);
                __setQuery(question, space);
            } else {
                __setPath(start, space);
            }
            start = space + 1;
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            if(succeed) {
                if(*(end - 1) == '1')
                    __setVersion(HTTP11);
                else if(*(end - 1) == '0')
                    __setVersion(HTTP10);
                else
                    succeed = false;
            } 
        }
    }

    return succeed;
}

bool HttpRequest::__setMethod(const char* start, const char* end)
{
    std::string m(start, end);
    if(m == "GET")
        m_method = Get;
    else if(m == "POST")
        m_method = Post;
    else if(m == "HEAD")
        m_method = Head;
    else if(m == "PUT")
        m_method = Put;
    else if(m == "DELETE")
        m_method = Delete;
    else
        m_method = Invalid;

    return m_method != Invalid;
}

void HttpRequest::__setPath(const char* begin, const char* end){
    std::string sub_path;
    sub_path.assign(begin, end);
    if(sub_path == "/") sub_path = "/test.html";
    m_path = STATIC_ROOT + sub_path;
}

void HttpRequest::__setQuery(const char* begin, const char* end){
    m_query.assign(begin, end);
}

void HttpRequest::__setVersion(HttpVersion version){
    m_version = version;
}

void HttpRequest::__addHeader(const char* begin, const char* colon, const char* end)
{
    std::string field(begin, colon);
    ++colon;
    while(colon < end && *colon == ' ')
        ++colon;
    std::string value(colon, end);
    while(!value.empty() && value[value.size() - 1] == ' ')
        value.resize(value.size() - 1);

    m_header[field] = value;
}





