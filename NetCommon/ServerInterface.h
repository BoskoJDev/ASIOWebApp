#pragma once
#include "Utilities.h"
#include "Message.h"
#include "ThreadSafeQueue.h"
#include "Connection.h"

template<typename T>
class ServerInterface
{
public:
	ServerInterface(uint16_t port) : acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
	{}

	virtual ~ServerInterface()
	{
		this->Stop();
	}

	bool Start()
	{
		try
		{
			this->WaitForClientConnection();
			this->contextThread = std::thread([this]() { context.run(); });
		}
		catch (std::exception& ex)
		{
			std::cout << "Server exception: " << ex.what() << '\n';
			return false;
		}

		std::cout << "Server started!\n";
		return true;
	}

	void Stop()
	{
		this->context.stop();
		this->contextThread.join();
		std::cout << "Server stopped!\n";
	}

	// This is asynchronous method
	void WaitForClientConnection()
	{
		this->acceptor.async_accept(
			[this](asio::error_code ec, asio::ip::tcp::socket socket)
			{
				if (ec)
				{
					std::cout << "Server connection error: " << ec.message() << '\n';
					WaitForClientConnection();
					return;
				}

				std::cout << "Server accepted new connection: " << socket.remote_endpoint() << '\n';
				std::shared_ptr<Connection<T>> conn = std::make_shared<Connection<T>>(
					Connection<T>::Owner::SERVER, context, std::move(socket), messagesIn
				);

				if (!OnClientConnected(conn))
				{
					std::cout << "Connection denied!\n";
					WaitForClientConnection();
					return;
				}

				connections.push_back(std::move(conn));
				auto& newlyAddedConnection = connections.back();
				newlyAddedConnection->ConnectToClient(IDCounter++);
				std::cout << '[' << newlyAddedConnection->ID() << "] Connection approved!\n";
				WaitForClientConnection();
			}
		);
	}

	void MessageClient(std::shared_ptr<Connection<T>> client, const Message<T>& msg)
	{
		if (client && client->IsConnected())
		{
			client->SendMsg(msg);
			return;
		}

		auto end = this->connections.end();

		OnClientDisconnected(client);
		client.reset();
		this->connections.erase(std::remove(this->connections.begin(), end, client), end);
	}

	void MessageAllClients(const Message<T>& msg, std::shared_ptr<Connection<T>> ignoredClient = nullptr)
	{
		bool invalidClientExists = false;
		for (auto& client : connections)
		{
			if (client && client->IsConnected())
			{
				if (client != ignoredClient)
					client->SendMsg(msg);

				continue;
			}

			this->OnClientDisconnected(client);
			client.reset();
			invalidClientExists = true;
		}

		if (!invalidClientExists)
			return;

		auto end = this->connections.end();
		this->connections.erase(std::remove(this->connections.begin(), end, nullptr), end);
	}

	void Update(size_t numOfMaxMessages = -1)
	{
		size_t numOfMessages = 0;
		while (numOfMessages < numOfMaxMessages && !messagesIn.IsEmpty())
		{
			// Grab the front message
			auto msg = messagesIn.Front();

			// Pass to message handler
			OnMessage(msg.remoteConnection, msg.msg);

			numOfMessages++;
		}
	}

protected:
	// Here you can reject the certain connection by returning false
	virtual bool OnClientConnected(std::shared_ptr<Connection<T>> client)
	{
		return false;
	}

	virtual void OnClientDisconnected(std::shared_ptr<Connection<T>> client)
	{
		
	}

	// Called when a message arrives
	virtual void OnMessage(std::shared_ptr<Connection<T>> client, Message<T>& msg)
	{

	}

	TSQueue<OwnedMessage<T>> messagesIn;
	asio::io_context context;
	std::thread contextThread;

	// This object will be used to get sockets of connected clients
	asio::ip::tcp::acceptor acceptor;

	std::deque<std::shared_ptr<Connection<T>>> connections;

	// This is necessary because every client will be represented by numeric ID
	uint32_t IDCounter = 10000;
};