#pragma once
#include "Connection.h"

template<typename T>
class ClientInterface
{
public:
	ClientInterface() : socket(context)
	{

	}

	virtual ~ClientInterface()
	{
		this->Disconnect();
	}

	bool Connect(const std::string& host, const uint16_t port)
	{
		using namespace asio::ip;

		try
		{
			this->conn = std::make_unique<Connection<T>>(
				Connection<T>::Owner::CLIENT,
				this->context,
				tcp::socket(this->context),
				this->messagesIn
			);

			tcp::resolver resolver(context);
			tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

			this->conn->ConnectToServer(endpoints);

			this->contextThread = std::thread([this]() { this->context.run(); });
		}
		catch (const std::exception& ex)
		{
			std::cout << "Client error: " << ex.what() << '\n';
			return false;
		}

		return true;
	}

	void Disconnect()
	{
		if (this->conn->IsConnected())
			this->conn->Disconnect();

		this->context.stop();
		this->contextThread.join();
		this->conn.release();
	}

	bool IsConnected()
	{
		if (!this->conn)
			return false;

		return this->conn->IsConnected();
	}

	TSQueue<OwnedMessage<T>>& Incoming() { return this->messagesIn; }

protected:
	asio::io_context context;
	std::thread contextThread;
	asio::ip::tcp::socket socket;

	// Client has only one instance of the 'connection' object, which handles the data transfer
	std::unique_ptr<Connection<T>> conn;

private:
	// Thread safe queue of incoming messages from server
	TSQueue<OwnedMessage<T>> messagesIn;
};