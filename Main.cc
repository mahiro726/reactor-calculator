#include "Calculator.hpp"
#include "Protocol.hpp"
#include "Reactor.hpp"
#include "Listener.hpp"
#include <iostream>
#include <memory>

using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " port";
        exit(1);
    }

    ENABLE_CONSOLE_LOG_STRATEGY();

    uint16_t server_port = std::stoi(argv[1]);
    // 创建网络版本计算器对象
    std::unique_ptr<Calculator> cal = std::make_unique<Calculator>();
    
    // 定义协议对象
    std::unique_ptr<Protocol> protocol = std::make_unique<Protocol>(
        [&cal](Request& req)->Response{
            return cal->Execute(req);
        } 
    );

    // 1. 创建listener对象 -> base connection
    std::shared_ptr<Connection> listener = std::make_unique<Listener>([&protocol](std::string &inbuffer)->std::string{
        return protocol->ParseRequest(inbuffer);
    } 
    , server_port);

    // 2. 创建一个TcpServer
    std::unique_ptr<Reactor> R = std::make_unique<Reactor>(server_port);

    // 3. 把链接添加到TcpServer中
    R->AddConnection(listener);

    // 4. 事件派发逻辑
    R->Loop();

}