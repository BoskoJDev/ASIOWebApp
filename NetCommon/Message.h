#pragma once
#include "Utilities.h"

/*Message header is sent at the start of all messages. Template allows us to use 'enum class'
to ensure that messages are valid at compile time*/
template<typename T>
struct MessageHeader
{
	T id;
	uint32_t size = 0;
};

template<typename T>
struct Message
{
	MessageHeader<T> header{};
	std::vector<uint8_t> body;

	size_t size() const { return sizeof(MessageHeader<T>) + body.size(); }

	friend std::ostream& operator<<(std::ostream& os, const Message<T>& message)
	{
		os << "Message ID: " << int(message.header.id) << " | Message size: " << message.header.size << '\n';
		return os;
	}

	template<typename Type>
	friend Message<T>& operator<<(Message<T>& message, const Type& data)
	{
		// Checking if the data is trivially copyable
		static_assert(std::is_standard_layout<Type>::value, "Data is too complex!!");

		// Data will be inserted from this point
		size_t currentBodySize = message.body.size();

		// Vector is resized by the size of data being pushed
		message.body.resize(message.body.size() + sizeof(Type));

		// Physically copy the data into the newly allocated vector
		std::memcpy(message.body.data() + currentBodySize, &data, sizeof(Type));

		message.header.size = message.size();
		return message;
	}

	template<typename Type>
	friend Message<T>& operator>>(Message<T>& message, Type& data)
	{
		// Checking if the data is trivially copyable
		static_assert(std::is_standard_layout<Type>::value, "Data is too complex!!");

		// Cache the location towards the end of the vector where the pulled data starts
		size_t i = message.body.size() - sizeof(data);

		// Physically copy the data from vector into the user variable
		std::memcpy(&data, message.body.data() + i, sizeof(Type));

		message.body.resize(i);
		message.header.size = message.size();
		return message;
	}
};

template<typename T>
class Connection;

template<typename T>
struct OwnedMessage
{
	std::shared_ptr<Connection<T>> remoteConnection = nullptr;
	Message<T> msg;

	friend std::ostream& operator<<(std::ostream& os, const OwnedMessage<T>& msg)
	{
		os << msg.msg;
		return os;
	}
};