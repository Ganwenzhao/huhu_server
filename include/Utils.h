#ifndef __UTILS_H__
#define __UTILS_H__

#define LISTENQ_LEN 1024

namespace huhu {
namespace utils{
    int createListenFd(int port);
    int setNonBlocking(int fd);
}

}

#endif //__UTILS_H__