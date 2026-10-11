#pragma once

#include <iostream>
#include "Socket.hpp"
#include "Logger.hpp"
#include "Epoller.hpp"
#include "Connection.hpp"
#include <unordered_map>

using namespace NS_LOG_MODULE;

static const int gdefaultport = 8080;
static const int grevs_num = 64;

// 看作一个可以管理链接的容器
class Reactor : public std::enable_shared_from_this<Reactor>
{
private:
    bool IsLegalConnection(int sockfd)
    {
        return _connections.find(sockfd) != _connections.end();
    }

public:
    Reactor()
        :_epoller(std::make_unique<Epoller>())
    {
    }

    void Dispatcher(int n)
    {
        for (int i = 0; i < n; i++)
        {
            int sockfd = revs[i].data.fd;
            uint32_t revents = revs[i].events;
            // 统一异常处理
            if (revents & EPOLLERR)
            {
                revents |= (EPOLLIN | EPOLLOUT);
            }
            if (revents & EPOLLHUP)
            {
                revents |= (EPOLLIN | EPOLLOUT);
            }
            if (revents & EPOLLIN && IsLegalConnection(sockfd))
            {
                _connections[sockfd]->Recver(); // 这里不用管是listensock还是普通套接字了，因为子类继承了基类，形成了多态
            } // 如果是Listener就获取新连接了，如果是普通文件描述符，就接发数据了
            if (revents & EPOLLOUT)
            {
                _connections[sockfd]->Sender(); // 被设置写事件关心的才会在这里自动调用Sender()
            }
        }
    }

    void LoopOnce(int timeout = 1000)
    {

        int n = _epoller->WaitEvents(revs, grevs_num, timeout);
        switch (n)
        {
        case 0:
            LOG(LogLevel::INFO) << "timeout ……";
            break;
        case -1:
            LOG(LogLevel::FATAL) << "Epoll error";
            break;
        default:
            LOG(LogLevel::INFO) << "event ready……";
            // 事件派发
            Dispatcher(n);
            break;
        }
    }

    // 一个链接如果超过100s没有活动，这个链接我就把他过期，就要关掉
    int CheckExpiredLink()
    {
        // 如何证明链接是活跃的？？
        int timeout = 0;
        time_t currenttime = time(nullptr);
        for (auto& conn : _connections)
        {
            time_t last_active = conn.second->LastActiveTime();
            time_t timediff = currenttime - last_active;
            if (timediff > 100000)
            {
                DelConnection(conn.second->GetSockfd());
            }
            time_t expiredtime = 100000 - timediff;
            if (timeout > expiredtime)
            {   
                timeout = expiredtime; // 保存超时最小值
            }
        }
        return timeout;
    }
    
    // 事件循环
    void Loop()
    {
        int timeout = 1000;
        while (true)
        {
            // 打印合法fd
            // DebugPrint();

            // 1. 事件派发
            LoopOnce(timeout);

            // 2. 链接管理
            // timeout = CheckExpiredLink(); // 检查过期链接
        }
    }

    void DebugPrint() // 主要是打印所有被管理起来的链接Connection
    {
        LOG(LogLevel::DEBUG) << "user in reactor: ";
        for (auto &conn : _connections)
        {
            LOG(LogLevel::DEBUG) << conn.second->GetSockfd() << ", " << conn.second->GetAddr().ToString();
        }
        LOG(LogLevel::DEBUG) << "------------------------------------------------------";
    }

    ~Reactor()
    {
    }

public:
    void AddConnection(std::shared_ptr<Connection> conn)
    {
        // 1. 新连接写透到内核中
        _epoller->AddEvents(conn->GetSockfd(), conn->GetEvents());
        // 2. 新连接托管到_connections
        _connections[conn->GetSockfd()] = conn;
        // 3. 让conn指向Reactor对象
        // conn->R = this;
        conn->R = get_self();
    }

    void EnableReadWrite(int sockfd, bool readable, bool writeable)
    {
        if (IsLegalConnection(sockfd))
        {
            uint32_t events = EPOLLIN | (readable ? EPOLLIN : 0) | (writeable ? EPOLLOUT : 0);
            _connections[sockfd]->SetEvents(events);
            // 写透到内核，其实就是用系统调用
            _epoller->ModEvents(_connections[sockfd]->GetSockfd(), _connections[sockfd]->GetEvents());
        }
    }

    void DelConnection(int sockfd) // 配合Excepter用的
    {
        if (IsLegalConnection(sockfd))
        {
            // 1. epoll中移除，穿透内核
            _epoller->DelEvents(sockfd);

            // 2. 关闭sockfd
            _connections[sockfd]->Close();
            
            // 3. _connections 移除
            _connections.erase(sockfd);
        }
    }

    std::shared_ptr<Reactor> get_self()
    {
        return shared_from_this();
    }

private:
    std::unique_ptr<Epoller> _epoller;
    struct epoll_event revs[grevs_num];
    std::unordered_map<int, std::shared_ptr<Connection>> _connections; // 定义为指针，方便直接指向子类对象，且基类不能定义对象，只能用指针
    // 一个对象IOManager，只能属于一个对象吗？？ 最大堆结构
};