#pragma once

#include <iostream>
#include <fcntl.h>
#include "Logger.hpp"

using namespace NS_LOG_MODULE;

enum class EXIT_CODE
{
    SUCCESS,
    EPOLLER_CREATE_ERROR,
    EPOLLER_WAIT_FATAL
};

void SetNonBlock(int sockfd) // 将文件描述符设置为非阻塞
{
    int fl = fcntl(sockfd, F_GETFL); // 获得当前文件描述符所对应的标志位，fl 全称 flags
    if (fl < 0)
    {
        return;
    }
    fcntl(sockfd, F_SETFL, fl | O_NONBLOCK);
}