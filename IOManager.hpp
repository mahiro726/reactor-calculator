#pragma once

#include "Connection.hpp"
#include <functional>

using on_message_t = std::function<std::string(std::string &)>; // 用于处理message的函数，等待上层注册

static const int buffersize = 1024;

class IOManager : public Connection
{
public:
    IOManager(int sockfd, uint32_t events, on_message_t on_message)
        : Connection(sockfd, events),
          _on_message(on_message)
    {
        // 第一次活跃
        Active();
    }

    int Recver() override
    {
        Active();
        LOG(LogLevel::DEBUG) << "事件派发到了IOManager, Recver()";
        // 实现非阻塞循环读写
        char buffer[buffersize];
        while (true)
        {
            ssize_t n = recv(_sockfd, buffer, buffersize - 1, 0); // 非阻塞读取了
            if (n > 0)
            {
                buffer[n] = 0;
                _inbuffer += buffer;
            }
            else if (n == 0)
            {
                LOG(LogLevel::INFO) << "client: " << _clientaddr.ToString() << " quit";
                Excepter(); // 异常处理怎么处理，你先别管，跟你没关系，先放一个函数占位
                return -1;
            }
            else
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    break;
                }
                if (errno == EINTR) // 信号中断
                {
                    continue;
                }
                else
                {
                    LOG(LogLevel::INFO) << "recv client: " << _clientaddr.ToString() << " error";
                    Excepter();
                    return -2;
                }
            }
        }

        // 2. 尝试回调处理
        // 只有一种情况
        LOG(LogLevel::DEBUG) << "本轮数据读完，inbuffer: " << _inbuffer;
        // 本轮数据读完了 == 一定有一个完整的请求报文？？，有没有完整的请求报文，由应用层协议决定！！
        // inbuffer尝试交给协议处理：1. 怎么交给上层  2. 上层协议是谁？？
        if (_on_message) // 判是否被注册
        {
            _outbuffer += _on_message(_inbuffer); // 将读取到的数据交给上层处理，嗯……需要先判空，毕竟函数不一定被注册了
        }
        // 对于新的文件fd，读事件默认不就绪，但是写事件默认是就绪的
        // 写事件不能常设置，写事件需要按需设置
        // 都不能直接读，写能直接写吗？能直接写！！
        // 对于写事件的处理，直接写，直到写失败，开启写事件关心
        if (!_outbuffer.empty())
        {
            Sender(); // 最佳实践，读完了就直接发，一直发到不行了才关心读事件
        }
//---------------------------------------------------------------------------------------------------------
        if (!_outbuffer.empty())
        {
            R->EnableReadWrite(_sockfd, true, true); // 我不直接发，而是打开写事件关心
        }

        return _inbuffer.size();
    }

    int Excepter() override
    {
        // 归一化异常处理
        LOG(LogLevel::WARNING) << "errno: " << errno << "message: " << strerror(errno);
        R->DelConnection(_sockfd);

        return 0;
    }

    int Sender() override
    {
        Active();
        LOG(LogLevel::DEBUG) << "事件派发到了IOManager, Sender()";
        while (true)
        {
            ssize_t n = send(_sockfd, _outbuffer.c_str(), _outbuffer.size(), 0); // 记住_outbuffer已经是应用层处理过后的应答了
            if (n > 0)
            {
                _outbuffer.erase(0, n);
                if (_outbuffer.empty())
                {
                    break;
                }
            }
            else
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    break;
                }
                else if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    LOG(LogLevel::INFO) << "send client: " << _clientaddr.ToString() << " error";
                    Excepter();
                    return -1;
                }
            }
        }

        // 走到这里，说明把缓冲区写满了或者_outbuffer发送完了
        
        if (!_outbuffer.empty())
        {
            // _outbuffer没有发送完成 && 缓冲区写满了
            // 写条件不满足 -> 开启socket对写事件的关心 -> 按需设置
            R->EnableReadWrite(_sockfd, true, true);
            LOG(LogLevel::INFO) << "发送条件不满足，write event 托管给epoll: " << _sockfd;
        }
        else
        {
            // _outbuffer发送完了
            R->EnableReadWrite(_sockfd, true, false); // 不需要关心读事件
            LOG(LogLevel::INFO) << "发送完毕，关闭写事件关系：" << _sockfd;
        }
        return 0;
    }

    ~IOManager()
    {
    }

private:
    on_message_t _on_message;
};