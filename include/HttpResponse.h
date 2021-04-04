#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include <map>
#include <string>
#include <mutex>
#define CONNECT_TIMEOUT  1500

namespace huhu{

class Buffer;
class HttpResponse{
public:
    static const std::map<int, std::string> statuscode_to_message;
    static const std::map<std::string, std::string> suffix_to_type;

    HttpResponse(int statuscode, std::string path, bool keep_alive)
        :m_statuscode(statuscode),
        m_path(path),
        m_keep_alive(keep_alive){}

    ~HttpResponse(){}

    Buffer makeResponse();
    void doErrorResponse(Buffer& output, std::string message);
    void doStaticRequest(Buffer& output, long file_size);
    
private:
    std::string __getFileType();

private:
    std::map<std::string, std::string> m_headers;
    int m_statuscode;
    std::string m_path;
    bool m_keep_alive;

};//class HttpResponse

}//namespace huhu


#endif//__HTTP_RESPONSE_H__