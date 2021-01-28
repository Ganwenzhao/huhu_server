#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cassert>

#define INIT_SIZE 4096

namespace huhu{
    
class Buffer{
public:
    Buffer();
    ~Buffer();

    //default copy constructor and assign func.

    size_t readableBytes() const;
    size_t writableBytes() const;
    size_t prependableBytes() const;
    //first readable pos 
    const char* peek() const;

    void retrieveLen(size_t len);
    void retrieveToEnd(const char* end);
    void retrieveAll();
    std::string retrieveAllAsString();

    void append(const std::string& str);
    void append(const char* data, size_t len);
    void append(const void* data, size_t len);
    void append(const Buffer& other_buff);

    void ensureWritableBytes(size_t len);
    char* beginWrite();
    const char* beginWrite() const;
    void hasWritten(size_t len);

    ssize_t readFd(int fd, int* savedErrno);
    ssize_t writeFd(int fd, int* saveErrno);

    const char* findCRLF() const;
    const char* findCRLF(const char* start) const;


private:
    char* __begin();
    const char* __begin() const;
    void __makeSpace(size_t len);

private:
    std::vector<char> m_buffer;
    size_t m_read_index;
    size_t m_write_index;
};//class Buffer

}//namespace huhu

#endif//__BUFFER_H__