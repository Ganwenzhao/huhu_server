#include "../include/HttpResponse.h"
#include "../include/Buffer.h"

#include <string>
#include <iostream>
#include <cassert>
#include <cstring>
#include <errno.h>
#include <fcntl.h> // open
#include <unistd.h> // close
#include <sys/stat.h> // stat
#include <sys/mman.h> // mmap, munmap

using namespace huhu;

const std::map<int, std::string> HttpResponse::statuscode_to_message = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"}
};

const std::map<std::string, std::string> HttpResponse::suffix_to_type = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".json", "applicaton/json"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"}
};

Buffer HttpResponse::makeResponse(){
    Buffer output;

    if(m_statuscode == 400) {
        doErrorResponse(output, "huhu can't parse the message");
        return output;
    }

    struct stat sbuf;
    // find file error
    if(::stat(m_path.data(), &sbuf) < 0) {
        m_statuscode = 404;
        doErrorResponse(output, "huhu can't find the file");
        return output;
    }
    // permission error
    if(!(S_ISREG(sbuf.st_mode) || !(S_IRUSR & sbuf.st_mode))) {
        m_statuscode = 403;
        doErrorResponse(output, "huhu can't read the file");
        return output;
    }

    doStaticRequest(output, sbuf.st_size);
    return output;
}


void HttpResponse::doStaticRequest(Buffer& output, long file_size){
    assert(file_size >= 0);

    auto itr = statuscode_to_message.find(m_statuscode);
    if(itr == statuscode_to_message.end()) {
        m_statuscode = 400;
        doErrorResponse(output, "Unknown status code");
        return;
    }

    // response line
    output.append("HTTP/1.1 " + std::to_string(m_statuscode) + " " + itr->second + "\r\n");
    // response header
    if(m_keep_alive) {
        output.append("Connection: Keep-Alive\r\n");
        output.append("Keep-Alive: timeout=" + std::to_string(CONNECT_TIMEOUT) + "\r\n");
    } else {
        output.append("Connection: close\r\n");
    }
    output.append("Content-type: " + __getFileType() + "\r\n");
    output.append("Content-length: " + std::to_string(file_size) + "\r\n");
    
    output.append("Server: huhu\r\n");
    output.append("\r\n");

    // response body
    int src_fd = ::open(m_path.data(), O_RDONLY, 0);
    //mmap
    void* mem = ::mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    ::close(src_fd);
    if(mem == (void*) -1) {
        munmap(mem, file_size);
        output.retrieveAll();
        m_statuscode = 404;
        doErrorResponse(output, "huhu can't find the file");
        return;
    }
    char* src_addr = static_cast<char*>(mem);
    output.append(src_addr, file_size);
    //cancel mmap
    munmap(src_addr, file_size);
}

std::string HttpResponse::__getFileType(){
    int idx = m_path.find_last_of('.');
    std::string suffix;
    // can`t find file suffix
    if(idx == std::string::npos) {
        return "text/plain";
    }
        
    suffix = m_path.substr(idx);
    auto itr = suffix_to_type.find(suffix);
    // unknown file suffix
    if(itr == suffix_to_type.end()) {
        return "text/plain";
    }   
    return itr->second;
}

void HttpResponse::doErrorResponse(Buffer& output, std::string message) {
    std::string body;

    auto itr = statuscode_to_message.find(m_statuscode);
    if(itr == statuscode_to_message.end()) {
        return;
    }

    body += "<html><title>huhu Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    body += std::to_string(m_statuscode) + " : " + itr->second + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>huhu web server</em></body></html>";

    
    output.append("HTTP/1.1 " + std::to_string(m_statuscode) + " " + itr->second + "\r\n");
    
    output.append("Server: huhu\r\n");
    output.append("Content-type: text/html\r\n");
    output.append("Connection: close\r\n");
    output.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    
    output.append(body);
}
