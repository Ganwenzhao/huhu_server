#include "../include/Utils.h"

#include <iostream>
#include <cstring> // bzero

#include <stdio.h> // perror
#include <unistd.h> // fcntl, close
#include <fcntl.h> // fcntl
#include <sys/socket.h> // socket, setsockopt, bind, listen
#include <arpa/inet.h> // htonl, htons

using namespace huhu;

int utils::createListenFd(int port){
    // check port
    port = ((port <= 1024) || (port >= 65535)) ? 8888 : port;

    int listen_fd = 0;
    if((listen_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
        printf("[Utils::createlisten_fd]fd = %d socket : %s\n", listen_fd, strerror(errno));
        return -1;
    }

    // avoid "Address already in use"
    int optval = 1;
    if(::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) == -1) {
        printf("[Utils::createlisten_fd]fd = %d setsockopt : %s\n", listen_fd, strerror(errno));
        return -1;
    }

    // bind
    struct sockaddr_in serverAddr;
    ::bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    serverAddr.sin_port = ::htons((unsigned short)port);
    if(::bind(listen_fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        printf("[Utils::createlisten_fd]fd = %d bind : %s\n", listen_fd, strerror(errno));
        return -1;
    }

    // listen
    if(::listen(listen_fd, LISTENQ_LEN) == -1) {
        printf("[Utils::createlisten_fd]fd = %d listen : %s\n", listen_fd, strerror(errno));
        return -1;
    }

    if(listen_fd == -1) {
        ::close(listen_fd);
        return -1;
    }

    return listen_fd;
}

int utils::setNonBlocking(int fd){
    // getfl
    int flag = ::fcntl(fd, F_GETFL, 0);
    if(flag == -1) {
        printf("[Utils::setNonBlocking]fd = %d fcntl : %s\n", fd, strerror(errno));
        return -1;
    }
    // set NONBLOCK
    flag |= O_NONBLOCK;
    if(::fcntl(fd, F_SETFL, flag) == -1) {
        printf("[Utils::setNonBlocking]fd = %d fcntl : %s\n", fd, strerror(errno));
        return -1;
    }

    return 0;
}

