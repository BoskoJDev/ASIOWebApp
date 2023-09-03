#include "Message.h"
#include "ClientInterface.h"

enum class CustomMsgType : uint32_t
{
	SERVER_ACCEPT,
	SERVER_DENY,
	SERVER_PING,
	MESSAGE_ALL,
	SERVER_MESSAGE
};

class Client : public ClientInterface<CustomMsgType>
{

};

int main()
{
	Client c;
	if (!c.Connect("127.0.0.1", 50000))
		return 1;
	bool key[3] = { false, false, false };
	bool old_key[3] = { false, false, false };

	bool bQuit = false;
	while (!bQuit)
	{
		if (GetForegroundWindow() == GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}

		if (key[2] && !old_key[2]) bQuit = true;

		for (int i = 0; i < 3; i++) old_key[i] = key[i];

		if (c.IsConnected())
		{
			if (!c.Incoming().IsEmpty())
			{
				auto msg = c.Incoming().PopFront().msg;
				switch (msg.header.id)
				{
				case CustomMsgType::SERVER_ACCEPT:
				{
					// Server has responded to a ping request				
					std::cout << "Server Accepted Connection\n";
				}
				break;


				case CustomMsgType::SERVER_PING:
				{
					// Server has responded to a ping request
					std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
					std::chrono::system_clock::time_point timeThen;
					msg >> timeThen;
					std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
				}
				break;

				case CustomMsgType::SERVER_MESSAGE:
				{
					// Server has responded to a ping request	
					uint32_t clientID;
					msg >> clientID;
					std::cout << "Hello from [" << clientID << "]\n";
				}
				break;
				}
			}
		}
		else
		{
			std::cout << "Server Down\n";
			bQuit = true;
		}

	}

	return 0;
}