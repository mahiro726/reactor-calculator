#pragma once
#include <iostream>
#include "InetAddr.hpp"
#include "Logger.hpp"
#include "Common.hpp"
#include <memory>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>

using namespace NS_LOG_MODULE;

static const int gbacklog = 16;

enum
{
    OK,
    SOCKET_ERROR,
    BIND_ERROR,
    LISTEN_ERROR
};

class Socket
{
public:
    virtual ~Socket() {}

protected:
    virtual void CreateSocketOrDie() = 0;
    virtual void BindSocketOrDie(uint16_t port) = 0;
    virtual void ListenSocketOrDie() = 0;
    
public:
    // virtual std::shared_ptr<Socket> Accept(InetAddr& clientaddr) = 0;
    virtual int Accept(InetAddr& clientaddr, int* errorcode) = 0;
    virtual int GetSockfd() = 0;
    virtual int Recv(std::string* out) = 0;
    virtual int Send(const std::string& in) = 0;
    virtual void Close() = 0;
    virtual bool Connect(InetAddr& serveraddr) = 0;

public:
    void BuildTcpSocketMethod(uint16_t port)  // 固定套路，模板方法
    {
        CreateSocketOrDie();
        BindSocketOrDie(port);
        ListenSocketOrDie();
    }

    void BuildTcpClientSockMethod()
    {
        CreateSocketOrDie();
    }

    // void BuildUdpSocketMethod(uint16_t port)
    // {
    //     CreateSocketOrDie();
    //     BindSocketOrDie(port);
    //     ListenSocketOrDie();
    // }
};

class TcpSocket : public Socket
{
public:
    TcpSocket(int sockfd = -1)
    :_sockfd(sockfd)
    {}

    ~TcpSocket()
    {
        if (_sockfd >= 0)
        {
            close(_sockfd);
        }
    }

    int Recv(std::string * out) override
    {
        char inbuffer[1024]; // 缓冲区
        ssize_t n = recv(_sockfd, inbuffer, sizeof(inbuffer)-1, 0);
        if (n > 0)
        {
            inbuffer[n] = 0;
            *out += inbuffer; 
        }
        return n;
    }

    int Send(const std::string& in) override 
    {
        ssize_t n = send(_sockfd, in.c_str(), in.size(), 0);
        return n;
    }    

    int GetSockfd() override
    {
        return _sockfd;
    }

    void Close() override
    {
        // 判断文件描述符是否合法
        if (_sockfd > 0)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }

    // std::shared_ptr<Socket> Accept(InetAddr& clientaddr) override // 输出型参数
    int Accept(InetAddr& clientaddr, int* errorcode) override // 改成直接返回裸的文件描述符，方便select
    {
        struct sockaddr_in address;
        socklen_t len = sizeof(address);
        int newsockfd = accept(_sockfd, (struct sockaddr*)&address, &len);
        *errorcode = errno;
        if (newsockfd < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
                LOG(LogLevel::ERROR) << "服务端 accept 失败: " << std::strerror(errno);
            }
            // return nullptr;
            return -1;
        }
        // 设置为非阻塞
        SetNonBlock(newsockfd); // 将新得到的链接设置为非阻塞
        
        LOG(LogLevel::DEBUG) << "服务端 accept 成功";
        clientaddr = address;
        // return std::make_shared<TcpSocket>(newsockfd);
        return newsockfd;
    }

    void CreateSocketOrDie() override
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0)
        {
            LOG(LogLevel::FATAL) << "创建 socket 失败";
            exit(SOCKET_ERROR);
        }
        LOG(LogLevel::DEBUG) << "创建 socket 成功";
        int opt = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 地址复用
        SetNonBlock(_sockfd);
    }

    void BindSocketOrDie(uint16_t port) override
    {
        InetAddr addr(port);

        if(bind(_sockfd, addr.NetAddress(), addr.GetLen()) != 0)
        {
            LOG(LogLevel::FATAL) << "bind 失败: " << std::strerror(errno);
            exit(BIND_ERROR);
        }
        LOG(LogLevel::DEBUG) << "bind 成功";
    }

    void ListenSocketOrDie() override
    {
        if (listen(_sockfd, gbacklog) != 0)
        {
            LOG(LogLevel::FATAL) << "listen 失败";
            exit(LISTEN_ERROR);
        }
        LOG(LogLevel::DEBUG) << "listen 成功";
    }

    bool Connect(InetAddr& serveraddr) override
    {   
        int n = connect(_sockfd, serveraddr.NetAddress(), serveraddr.GetLen());
        if (n < 0)
        {
            return false;
        }
        return true;
    }   

private:
    int _sockfd;
};



// class UdpSocket : public Socket
// {
// public:
//     UdpSocket()
//     {}

//     ~UdpSocket()
//     {}

//     virtual void CreateSocketOrDie() = 0;
//     virtual void BindSocketOrDie(uint16_t port) = 0;
//     virtual void ListenSocketOrDie() override
//     {}

// private:
//     int _sockfd;
// };