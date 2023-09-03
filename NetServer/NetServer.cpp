#include "Message.h"
#include "ServerInterface.h"

enum class CustomMsgType : uint32_t
{
    SERVER_ACCEPT,
    SERVER_DENY,
    SERVER_PING,
    MESSAGE_ALL,
    SERVER_MESSAGE
};

class Server : public ServerInterface<CustomMsgType>
{
public:
    Server(uint16_t port) : ServerInterface<CustomMsgType>(port) {}

protected:
    bool OnClientConnected(std::shared_ptr<Connection<CustomMsgType>> client) override
    {
        Message<CustomMsgType> msg;
        msg.header.id = CustomMsgType::SERVER_MESSAGE;
        client->SendMsg(msg);

        return true;
    }
    
    void OnClientDisconnected(std::shared_ptr<Connection<CustomMsgType>> client) override
    {

    }

    void OnMessage(std::shared_ptr<Connection<CustomMsgType>> client, Message<CustomMsgType>& msg) override
    {

    }
};

int main()
{
    Server server(6000);
    server.Start();

    while (true)
    {
        server.Update();
    }

    

    return 0;
}