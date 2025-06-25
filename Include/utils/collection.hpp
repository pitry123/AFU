#pragma once
#include <iostream>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

namespace afu
{
	template <typename T>
	class cyclicBuffer 
	{
	private:
		std::vector<T> buffer;
		size_t head;   // Points to the next write position
		size_t tail;   // Points to the next read position
		size_t capacity;
		bool full;     // Tracks if the buffer is full

	public:
		// Constructor to initialize the buffer with a given size
		explicit cyclicBuffer(size_t size)
			: buffer(size), head(0), tail(0), capacity(size), full(false) {}

		// Push data into the buffer
		void push(const T& value) {
			if (full) {
				// If buffer is full, move the tail to the next position to overwrite
				tail = (tail + 1) % capacity;
			}

			// Write the value to the buffer and move the head
			buffer[head] = value;
			head = (head + 1) % capacity;

			// Mark the buffer as full if head catches up with the tail
			full = head == tail;
		}

		// Pop data from the buffer
		T pop() {
			if (isEmpty()) {
				throw std::out_of_range("Buffer is empty!");
			}

			// Read the value at the tail position and move the tail
			T value = buffer[tail];
			tail = (tail + 1) % capacity;

			// Buffer is no longer full after pop
			full = false;
			return value;
		}

		// Check if the buffer is empty
		bool isEmpty() const {
			return (head == tail) && !full;
		}

		// Check if the buffer is full
		bool isFull() const {
			return full;
		}

		// Get the current size of the buffer
		size_t size() const {
			if (full) {
				return capacity;
			}
			else if (head >= tail) {
				return head - tail;
			}
			else {
				return capacity + head - tail;
			}
		}

		T& operator[](size_t index) {
			if (index >= size()) {
				throw std::out_of_range("Index out of range!");
			}
			return buffer[(tail + index) % capacity];
		}

		// Const version of operator[] for read-only access
		const T& operator[](size_t index) const {
			if (index >= size()) {
				throw std::out_of_range("Index out of range!");
			}
			return buffer[(tail + index) % capacity];
		}
	};

	template <typename T>
	class threadSafeQueue
	{
	public:
		threadSafeQueue() = default;

		void push(T& val)
		{
			std::lock_guard<std::mutex> lock(m_lock_c);
			m_tsq.push(val);
			m_c.notify_all();
		}

		T pop()
		{
			std::unique_lock<std::mutex> lock(m_lock_c);
			m_c.wait(lock, [&] { return !m_tsq.empty(); });
			auto v = m_tsq.front();
			m_tsq.pop();
			return v;
		}

	private:

		std::queue<T> m_tsq;
		std::condition_variable m_c;
		std::mutex m_lock_c;
	};

} 

