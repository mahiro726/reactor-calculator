#pragma once

#include "Protocol.hpp"
#include <iostream>
#include <string>

class Calculator
{
public:
    Response Execute(const Request &req)
    {
        Response resp;
        switch (req._oper)
        {
        case '+':
            resp._result = req._data_x + req._data_y;
            break;
        case '-':
            resp._result = req._data_x - req._data_y;
            break;
        case '*':
            resp._result = req._data_x * req._data_y;
            break;
        case '/':
        {
            if (req._data_y == 0)
                resp._code = 1; // 除零错误
            else
                resp._result = req._data_x / req._data_y;
        }
        break;
        case '%':
        {
            if (req._data_y == 0)
                resp._code = 1; // 除零错误
            else
                resp._result = req._data_x % req._data_y;
        }
        break;
        default:
            resp._code = 3; // 非法操作
            break;
        }

        return resp;
    }
};