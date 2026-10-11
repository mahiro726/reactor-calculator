#pragma once

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdint>
#include <sys/epoll.h>
#include "InetAddr.hpp"

class Reactor;

class Connection
{
public:
    Connection(int sockfd = -1, uint32_t events = EPOLLIN)
    :_sockfd(sockfd),
     _events(events),
     _active_time(time(nullptr))
    {}

    void Active()
    {
        _active_time = time(nullptr); // 更新的时间
    }
    
    virtual int Sender() = 0;
    virtual int Recver() = 0;
    virtual int Excepter() = 0;

    int GetSockfd()
    {
        return _sockfd;
    }

    time_t LastActiveTime()
    {
        return _active_time;
    }

    void Close()
    {
        if(_sockfd >= 0)
        {
            close(_sockfd);
            _sockfd = -1; // 为了安全起见
        }
    }

    uint32_t GetEvents()
    {
        return _events;
    }

    void SetEvents(uint32_t events)
    {
        _events = events; // 这个是自己写的，可以和老师的对照一下看看差别
    }

    InetAddr GetAddr()
    {
        return _clientaddr;
    }

    void InitAddr(const InetAddr &addr)
    {
        _clientaddr = addr;
    }

    virtual ~Connection()
    {}

protected: // 只影响继承，protected可以被子类继承，private不行
    int _sockfd;
    uint32_t _events; // 我要关心什么事件就设置进来
    time_t _active_time; // 活跃的时间

    std::string _inbuffer; // 接受缓冲区
    std::string _outbuffer; // 发送缓冲区

    InetAddr _clientaddr; // 客户端地址

public:
    // Reactor *R; // 回调指针
    std::shared_ptr<Reactor> R;
};