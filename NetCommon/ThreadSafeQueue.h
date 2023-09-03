#pragma once
#include "Utilities.h"

template<typename T>
class TSQueue
{
public:
	TSQueue() = default;
	TSQueue(const TSQueue<T>&) = delete; // Don't allow copying because of the mutexes
	virtual ~TSQueue() { this->Clear(); }

	const T& Front()
	{
		std::scoped_lock lock(this->mutex);
		return this->deque.front();
	}

	const T& Back()
	{
		std::scoped_lock lock(this->mutex);
		return this->deque.back();
	}

	void PushFront(const T& item)
	{
		std::scoped_lock lock(this->mutex);
		this->deque.emplace_front(std::move(item));
	}

	void PushBack(const T& item)
	{
		std::scoped_lock lock(this->mutex);
		this->deque.emplace_back(std::move(item));
	}

	bool IsEmpty()
	{
		std::scoped_lock lock(this->mutex);
		return this->deque.empty();
	}

	size_t Size()
	{
		std::scoped_lock lock(this->mutex);
		return this->deque.size();
	}

	void Clear()
	{
		std::scoped_lock lock(this->mutex);
		this->deque.clear();
	}

	T PopFront()
	{
		std::scoped_lock lock(this->mutex);
		auto i = std::move(this->deque.front());
		this->deque.pop_front();
		return i;
	}

	T PopBack()
	{
		std::scoped_lock lock(this->mutex);
		auto i = std::move(this->deque.back());
		this->deque.pop_back();
		return i;
	}

protected:
	std::mutex mutex;
	std::deque<T> deque;
};