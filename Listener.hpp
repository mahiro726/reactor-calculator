#pragma once

#include <iostream>
#include <memory>
#include <sys/epoll.h>

#include "Socket.hpp"
#include "IOManager.hpp"
#include "Connection.hpp"

static const uint16_t defaultport = 8080;

class Listener : public Connection
{
public:
    Listener(on_message_t on_message_helper, uint16_t port = defaultport)
        :_on_message_helper(on_message_helper), 
         _port(port),
         _listensock(std::make_unique<TcpSocket>())
    {
        _listensock->BuildTcpSocketMethod(_port);
        _sockfd = _listensock->GetSockfd();
        _events = EPOLLIN | EPOLLET; // 这里将Listener设置为了ET，因为是初始化，所以默认就是ET了
        Active();
    }

    void RegisterService(on_message_t on_message_helper)
    {
        _on_message_helper = on_message_helper;
    }

    int Recver() override
    {
        LOG(LogLevel::DEBUG) << "事件派发到了Listener";
        while (true)
        {
            // 获取新连接，必须循环获取
            InetAddr clientaddr;
            int errorcode = 0;                                        // 输出型参数，将错误码带出来，挺重要的
            int sockfd = _listensock->Accept(clientaddr, &errorcode); // 能保证只会来一个链接吗？
            if (sockfd < 0)
            {
                if (errorcode == EWOULDBLOCK || EAGAIN)
                {
                    LOG(LogLevel::INFO) << "accepter all client done";
                    break;
                }
                else if (errorcode == EINTR) // 表示被信号中断了
                {
                    continue;
                }
                else 
                {
                    LOG(LogLevel::WARNING) << "accepter Error";
                    break;
                }
            }

            // 1. 获取了新连接，然后我们该怎么办？？包装成为一个connection
            std::shared_ptr<Connection> conn = std::make_unique<IOManager>(sockfd, EPOLLIN | EPOLLET, _on_message_helper);

            conn->InitAddr(clientaddr); // 这里是为了将clientaddr设置到新创建的connection内部

            // 2. IOManager 要设置到Reactor中
            R->AddConnection(conn);
            LOG(LogLevel::INFO) << "获取一个新链接，新链接sockfd: " << sockfd << " client addr: " << clientaddr.ToString();
        }

        return 0;
    }

    int Sender() override
    {
        return 0;
    }

    int Excepter() override
    {
        return 0;
    }

    ~Listener()
    {
    }

private:
    uint16_t _port;
    std::unique_ptr<Socket> _listensock;
    on_message_t _on_message_helper;
};