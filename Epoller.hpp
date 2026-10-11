#pragma once

#include <iostream>
#include <cstdlib>
#include <sys/epoll.h>
#include "Common.hpp"
#include "Logger.hpp"

using namespace NS_LOG_MODULE;

// #define READ_EVENT EPOLLIN

class Epoller
{
private:
    // User -> Kernel
    bool EpollCtl(int sockfd, int oper, uint32_t events)
    {
        if (oper == EPOLL_CTL_DEL) // 区分一下删除和其他情况
        {
            int n = epoll_ctl(_epfd, oper, sockfd, nullptr);
            (void)n;
            (void)events;
        }
        else 
        {
            struct epoll_event ev; // 所以这个是一次性的？
            ev.data.fd = sockfd;
            ev.events = events;
            int m = epoll_ctl(_epfd, oper, sockfd, &ev);
            (void)m;
        }
        return true;
    }

public:
    Epoller()
    {
        _epfd = epoll_create(256);
        if(_epfd < 0)
        {
            LOG(LogLevel::FATAL) << "epoll_create error";
            // 退出码
            exit(static_cast<int>(EXIT_CODE::EPOLLER_CREATE_ERROR));
        }
        LOG(LogLevel::INFO) << "epoll_create success, epfd: " << _epfd;
    }

    // Kernel -> User
    int WaitEvents(struct epoll_event revs[], int event_num, int timeout = 1000)
    {
        return epoll_wait(_epfd, revs, event_num, timeout);
    }

    bool AddEvents(int sockfd, uint32_t events)
    {
        return EpollCtl(sockfd, EPOLL_CTL_ADD, events);
    }

    bool ModEvents(int sockfd, uint32_t events)
    {
        return EpollCtl(sockfd, EPOLL_CTL_MOD, events);
    }

    bool DelEvents(int sockfd)
    {
        return EpollCtl(sockfd, EPOLL_CTL_DEL, 0);
    } 
    
    ~Epoller()
    {
        if(_epfd > 0)
        {
            close(_epfd);
        }
    }

private:
    int _epfd;
};