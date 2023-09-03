#pragma once
#include "Utilities.h"
#include "Message.h"
#include "ThreadSafeQueue.h"

template<typename T>
class Connection : std::enable_shared_from_this<Connection<T>>
{
public:
	enum class Owner
	{
		SERVER,
		CLIENT
	};

	Connection(Owner p, asio::io_context& c, asio::ip::tcp::socket s, TSQueue<OwnedMessage<T>>& tsq)
		: context(c), owner(p), socket(std::move(s)), messagesIn(tsq)
	{}
	
	virtual ~Connection() {}

	uint32_t ID() const { return this->id; }

	void ConnectToClient(uint32_t id = 0)
	{
		if (this->owner == Owner::CLIENT)
			return;

		if (!this->socket.is_open())
			return;

		this->id = id;
		this->ReadHeader();
	}

	void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
	{
		// Only clients can connect to servers
		if (this->owner != Owner::CLIENT)
			return;

		asio::async_connect(this->socket, endpoints,
			[this](asio::error_code ec, asio::ip::tcp::endpoint endpoint)
			{
				if (ec)
				{
					std::cout << ec.message() << '\n';
					return;
				}

				this->ReadHeader();
			}
		);
	}

	void Disconnect()
	{
		if (!this->IsConnected())
			return;

		asio::post(this->context, [this]() { socket.close(); });
	}

	bool IsConnected() const { return this->socket.is_open(); }

	void SendMsg(const Message<T>& msg)
	{
		asio::post(this->context,
			[this, msg]()
			{
				/*If the queue has a message in it, then we must 
				assume that it is in the process of asynchronously being written.
				Either way add the message to the queue to be output. If no messages
				were available to be written, then start the process of writing the
				message at the front of the queue.*/
				bool isWritingMessage = !messagesOut.IsEmpty();
				messagesOut.PushBack(msg);
				if (isWritingMessage)
					return;

				WriteHeader();
			}
		);
	}

private:
	// Asynchronous method
	void WriteHeader()
	{
		/*If this function is called, we know the outgoing message queue must have
		at least one message to send. So allocate a transmission buffer to hold
		the message, and issue the work - asio, send these bytes*/
		asio::async_write(
			this->socket, asio::buffer(&this->messagesOut.Front().header, sizeof(MessageHeader<T>)),
			[this](asio::error_code ec, size_t length)
			{
				if (ec)
				{
					/*asio has now sent the bytes - if there was a problem, an error would be
					available - asio failed to write the message, we could analyse why but
					for now simply assume the connection has died by closing the
					socket. When a future attempt to write to this client fails due
					to the closed socket, it will be tidied up.*/
					std::cout << "[" << id << "] Write Header Fail.\n";
					socket.close();
					return;
				}

				if (messagesOut.Front().body.size() > 0) // If message contains body
				{
					WriteBody();
					return;
				}

				messagesOut.PopFront();

				/*If the queue is not empty, there are more messages to send, so
				make this happen by issuing the task to send the next header.*/
				if (!messagesOut.IsEmpty())
					WriteHeader();
			}
		);
	}

	// Asynchronous method
	void WriteBody()
	{
		/*If this function is called, a header has just been sent, and that header
		indicated a body existed for this message. Fill a transmission buffer
		with the body data, and send it!*/
		asio::async_write(this->socket, asio::buffer(messagesOut.Front().body.data(), messagesOut.Front().body.size()),
			[this](asio::error_code ec, size_t length)
			{
				if (ec)
				{
					// Sending failed, see WriteHeader() equivalent for description :P
					std::cout << "[" << id << "] Write Body Fail.\n";
					socket.close();
					return;
				}

				/* Sending was successful, so we are done with the messageand remove it
				from the queue*/
				messagesOut.PopFront();

				/*If the queue still has messages in it, then issue the task to
				send the next messages' header.*/
				if (!messagesOut.IsEmpty())
					WriteHeader();
			}
		);
	}

	// Asynchronous method
	void ReadHeader()
	{
		/*If this function is called, we are expecting asio to wait until it receives
		enough bytes to form a header of a message. We know the headers are a fixed
		size, so allocate a transmission buffer large enough to store it. In fact, 
		we will construct the message in a "temporary" message object as it's 
		convenient to work with.*/
		asio::async_read(this->socket, asio::buffer(&this->tempMsgIn.header, sizeof(MessageHeader<T>)),
			[this](asio::error_code ec, size_t length)
			{
				if (ec)
				{
					/*Reading form the client went wrong, most likely a disconnect
					has occurred. Close the socket and let the system tidy it up later.*/
					std::cout << "[" << id << "] Read Header Fail.\n";
					socket.close();
					return;
				}

				if (tempMsgIn.header.size == 0)
				{
					AddToIncomingMessageQueue();
					return;
				}

				tempMsgIn.body.resize(tempMsgIn.header.size);
				ReadBody();
			}
		);
	}

	// Asynchronous method
	void ReadBody()
	{
		/*If this function is called, a header has already been read, and that header
		request we read a body, The space for that body has already been allocated
		in the temporary message object, so just wait for the bytes to arrive...*/
		asio::async_read(this->socket, asio::buffer(this->tempMsgIn.body.data(), this->tempMsgIn.body.size()),
			[this](asio::error_code ec, size_t length)
			{
				if (ec)
				{
					std::cout << "[" << id << "] Read Body Fail.\n";
					socket.close();
					return;
				}

				AddToIncomingMessageQueue();
			}
		);
	}

	void AddToIncomingMessageQueue()
	{
		/*Shove it in queue, converting it to an "owned message", by initialising
		with the a shared pointer from this connection object*/
		auto conn = this->owner == Owner::SERVER ? this->shared_from_this() : nullptr;
		this->messagesIn.PushBack({ conn, this->tempMsgIn });

		/*We must now prime the asio context to receive the next message. It 
		will just sit and wait for bytes to arrive, and the message construction
		process repeats itself. Clever huh?*/
		this->ReadHeader();
	}

protected:
	asio::ip::tcp::socket socket;

	asio::io_context& context; // this will be shared with the whole asio instance

	//This queue holds all the messages to be sent to the remote side of the connection
	TSQueue<Message<T>> messagesOut;

	/*This queue will hold all the messages that have been received from the remote side of the
	connection. It's the reference since the owner of this connection is supposed to provide
	the queue*/
	TSQueue<OwnedMessage<T>>& messagesIn;

	Owner owner; // The "owner" decides how some of the connection behaves

	/*Incoming messages are constructed asynchronously, so we will
	store the part assembled message here, until it is ready*/
	Message<T> tempMsgIn;

	uint32_t id = 0;
};