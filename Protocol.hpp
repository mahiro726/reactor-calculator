#pragma once

// 自定义协议部分
#include <iostream>
#include <string>
#include <jsoncpp/json/json.h>
#include "Logger.hpp"
#include <functional>

using namespace NS_LOG_MODULE;

// 1. 自己做 - 不建议！
// 2. 用别人的 - json protobuf xml

// 请求报文
class Request
{
public:
    Request()
        : _data_x(0),
          _data_y(0),
          _oper(0)
    {
    }

    Request(int data_x, int data_y, char oper)
        : _data_x(data_x),
          _data_y(data_y),
          _oper(oper)
    {
    }

    // 序列化
    bool Serialize(std::string *out)
    {
        // 结构化 -> "_data_x  _oper  _data_y"
        Json::Value root;
        root["left"] = _data_x;
        root["right"] = _data_y;
        root["oper"] = _oper;

        Json::FastWriter writer;
        *out = writer.write(root);

        return true;
    }

    // 反序列化
    bool Deserialize(std::string &in)
    {
        // "_data_x  _oper  _data_y" -> 结构化
        Json::Value root;
        Json::Reader reader;
        bool parsesuccess = reader.parse(in, root);
        if (!parsesuccess)
        {
            return false;
        }

        _data_x = root["left"].asInt();
        _data_y = root["right"].asInt();
        _oper = root["oper"].asInt();

        return true;
    }

    ~Request()
    {
    }

public:
    int _data_x;
    int _data_y;
    char _oper; // 只有一个字符的加减乘除
};

// 应答报文
class Response
{
public:
    Response()
        : _result(0),
          _code(0)
    {
    }

    // 序列化
    bool Serialize(std::string *out)
    {
        // 结构化 -> "_data_x  _oper  _data_y"
        Json::Value root;
        root["result"] = _result;
        root["code"] = _code;

        Json::FastWriter writer;
        *out = writer.write(root);

        return true;
    }

    // 反序列化
    bool Deserialize(std::string &in)
    {
        // "_data_x  _oper  _data_y" -> 结构化
        Json::Value root;
        Json::Reader reader;
        bool parsesuccess = reader.parse(in, root);
        if (!parsesuccess)
        {
            return false;
        }

        _result = root["result"].asInt();
        _code = root["right"].asInt();
        return true;
    }

    ~Response()
    {
    }

public:
    int _result; // 结果，用int存
    int _code;   // 状态码，用int存
};

const std::string gsep = "\r\n";
using HandlerRequest_t = std::function<Response(Request &)>;
using HandlerResponse_t = std::function<void (Response&)>;

class Protocol
{
public:
    Protocol(HandlerRequest_t handler)
        : _version("1.0"),
          _handler_request(handler)
    {
    }

    Protocol(HandlerResponse_t handler_resp)
    :_version("1.0"),
     _handler_response(handler_resp)
    {}

    // 封包
    std::string Packet(const std::string &json_string)
    {
        return std::to_string(json_string.size()) + gsep + json_string + gsep;
    }

    // 解包
    int Unpack(std::string &packet, std::string *json_string) // 这里的json_string是输出型参数
    {
        // 合法性检验
        if (packet.empty())
        {
            return 0;
        }
        if (json_string == nullptr)
        {
            return -1;
        }

        // 分析报文
        // 先找到len
        auto pos = packet.find(gsep);
        if (pos == std::string::npos)
        {
            return 0;
        }
        std::string lenstr = packet.substr(0, pos);
        int len = std::stoi(lenstr);
        // 算出总长度
        int total = len + lenstr.size() + 2 * gsep.size();
        // 如果没有达到总长度，就返回
        if (packet.size() < total)
        {
            return 0;
        }

        // 提取报文
        *json_string = packet.substr(pos + gsep.size(), len);
        packet.erase(0, total);
        return 1;
    }

    std::string ParseRequest(std::string &inbuffer) // parse是解析，不仅仅解析请求，执行上层计算，还将计算结果进一步封包了
    {
        std::string result;
        // 循环
        while (true)
        {
            std::string json_string;
            // 解包
            int n = Unpack(inbuffer, &json_string);
            if (n < 0)
            {
                LOG(LogLevel::DEBUG) << "未传json_string!";
                return std::string();
            }
            if (n == 0)
            {
                LOG(LogLevel::INFO) << "inbuffer: " << inbuffer << " parse done";
                return result;
            }
            LOG(LogLevel::DEBUG) << "json_string:\n" << json_string;

            // 反序列化，得到请求报文
            // 得到一个完整的报文json_string
            Request req;
            if (!req.Deserialize(json_string))
            {
                return std::string();
            }

            // 业务计算
            Response resp;
            if (_handler_request)
            {
                resp = _handler_request(req);
            }

            // 应答序列化
            std::string resp_json_string;
            resp.Serialize(&resp_json_string);

            // 添加报头
            result += Packet(resp_json_string);
        }
    }

    std::string ParseResponse(std::string &inbuffer)
    {
        // 循环
        while (true)
        {
            std::string json_string;
            // 解包
            int n = Unpack(inbuffer, &json_string);
            if (n < 0)
            {
                LOG(LogLevel::DEBUG) << "未传json_string!";
                return std::string();
            }
            if (n == 0)
            {
                LOG(LogLevel::INFO) << "parse done";
                return std::string();
            }

            // 反序列化，得到请求报文
            // 得到一个完整的报文json_string
            Response resp;
            if (!resp.Deserialize(json_string))
            {
                return std::string();
            }

            // 回调处理
            if (_handler_response)
            {
                _handler_response(resp);
            }

        }
    }

    ~Protocol()
    {
    }

private:
    std::string _version;
    HandlerRequest_t _handler_request;
    HandlerResponse_t _handler_response;
};