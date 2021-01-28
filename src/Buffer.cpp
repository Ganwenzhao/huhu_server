#include "../include/Buffer.h"

#include <cstring> // perror
#include <iostream>
#include <errno.h>
#include <unistd.h> // write
#include <sys/uio.h> // readv

using namespace huhu;

Buffer::Buffer()
    :m_buffer(INIT_SIZE), \
    m_read_index(0), \
    m_write_index(0){
        assert(readableBytes() == 0);
        assert(writableBytes() == INIT_SIZE);
}

Buffer::~Buffer(){}

size_t Buffer::readableBytes() const{
    return m_write_index - m_read_index;
}

size_t Buffer::writableBytes() const{
    return m_buffer.size() - m_write_index;
}

size_t Buffer::prependableBytes() const{
    return m_read_index;
}
//first readable position
const char* Buffer::peek() const {
    return __begin() + m_read_index;
}

void Buffer::retrieveLen(size_t len){
    assert(len <= readableBytes());
    m_read_index += len;
}

void Buffer::retrieveToEnd(const char* end){
    assert(peek() <= end);
    assert(end <= beginWrite());

    retrieveLen(end - peek());
}

void Buffer::retrieveAll(){
    m_read_index = 0;
    m_write_index = 0;
}

std::string Buffer::retrieveAllAsString(){
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
}

void Buffer::append(const std::string& str){
    append(str.data(), str.size());
}

void Buffer::append(const char* data, size_t len){
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
}

void Buffer::append(const void* data, size_t len){
    append(static_cast<const char*>(data), len);
}

void Buffer::append(const Buffer& other_buff){
    append(other_buff.peek(), other_buff.readableBytes());
}

void Buffer::ensureWritableBytes(size_t len){
    if(writableBytes() < len){
        __makeSpace(len);
    }
    assert(writableBytes() >= len);
}

char* Buffer::beginWrite(){
    return __begin() + m_write_index;
}

const char* Buffer::beginWrite() const{
    return __begin() + m_write_index;
}

void Buffer::hasWritten(size_t len){
    m_write_index += len;
}

ssize_t Buffer::readFd(int fd, int* savedErrno){
    // ref muduo buffer.p204[chenshuo]
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = __begin() + m_write_index;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    const ssize_t n = ::readv(fd, vec, 2);
    if(n < 0) {
        printf("[Buffer:readFd]fd = %d readv : %s\n", fd, strerror(errno));
        *savedErrno = errno;
    } 
    else if(static_cast<size_t>(n) <= writable)
        m_write_index += n;
    else {
        m_write_index = m_buffer.size();
        append(extrabuf, n - writable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno){
    size_t nLeft = readableBytes();
    char* bufPtr = __begin() + m_read_index;
    ssize_t n;
    if((n = ::write(fd, bufPtr, nLeft)) <= 0) {
        if(n < 0 && n == EINTR)
            return 0;
        else {
            printf("[Buffer:writeFd]fd = %d write : %s\n", fd, strerror(errno));
            *savedErrno = errno;
            return -1;
        }
    } else {
        m_read_index += n;
        return n;
    }
}

const char* Buffer::findCRLF() const{
    const char CRLF[] = "\r\n";
    const char* crlf = std::search(peek(), beginWrite(), CRLF, CRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
}

const char* Buffer::findCRLF(const char* start) const{
    assert(peek() <= start && start <= beginWrite());

    const char CRLF[] = "\r\n";
    const char* crlf = std::search(start, beginWrite(), CRLF, CRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
}

char* Buffer::__begin(){
    return &*m_buffer.begin();
}

const char* Buffer::__begin() const{
    return &*m_buffer.begin();
}

void Buffer::__makeSpace(size_t len){
    if(writableBytes() + prependableBytes() < len){
        m_buffer.resize(m_write_index + len);
    }
    else{
        size_t readable = readableBytes();
        std::copy(__begin() + m_read_index, \
                    __begin() + m_write_index, \
                    __begin());
        m_read_index = 0;
        m_write_index = m_read_index + readable;
        assert(readable == readableBytes());
    }
}


