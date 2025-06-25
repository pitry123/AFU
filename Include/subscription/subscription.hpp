#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "dispatcher.hpp"


namespace afu
{
	
	using byte_vector = std::vector<unsigned char>;



	class subscription_data :
		public std::enable_shared_from_this<subscription_data>
	{

	private:

		byte_vector m_buffer;
		
		subscription_data() = default;
		
		subscription_data(size_t _data_size) :
			m_buffer(_data_size)
		{}

		subscription_data(const byte_vector& _data) :
			m_buffer(_data)
		{}

		subscription_data(byte_vector&& _data) noexcept :
			m_buffer(std::move(_data))
		{
		}

		// Copy from raw pointer + size
		subscription_data(const unsigned char* data, size_t size) :
			m_buffer(data, data + size)
		{
		}


		// just for update (override data)
		template<typename T>
		void write(const T& _val)
		{
			static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

			if (m_buffer.size() != sizeof(T))
			{
				throw std::invalid_argument(
					"Buffer size (" + std::to_string(m_buffer.size()) +
					") does not match size of T (" + std::to_string(sizeof(T)) + ")");
			}

			std::memcpy(m_buffer.data(), &_val, sizeof(T));
		}

		byte_vector& get_ref_buffer()
		{
			return m_buffer;
		}

		const byte_vector& get_buffer()
		{
			return m_buffer;
		}

		size_t size() const noexcept { return m_buffer.size(); }

		void clear() noexcept { m_buffer.clear(); }


	public:

		friend class subscriber_interface;

		//copyble read 
		void read(void* _buffer, size_t _size)
		{
			if (_buffer == nullptr)
				throw std::invalid_argument("buffer is null");

			if (_size == 0 || _size != m_buffer.size())
				throw std::invalid_argument("size incorrect");

			std::memcpy(_buffer, m_buffer.data(), _size);
		}

		template<typename T>
		T& read()
		{
			if(m_buffer.size() <= 0)
				throw std::runtime_error("size 0");

			if (m_buffer.size() != sizeof(T))
				throw std::invalid_argument("Error type");

			return *(reinterpret_cast<T*>((m_buffer.data())));
		}

		template<typename T>
		void read(T& val)
		{
			val = read<T>();
		}

		std::shared_ptr<subscription_data> get_shared() {
			return shared_from_this();
		}
	};




	class subscriber_interface :
		public std::enable_shared_from_this<subscriber_interface>

	{
	public:

		~subscriber_interface() = default;

		virtual void subscribe(dispatcher_interface* _disp, std::function<void(const subscription_data&)>) = 0;
		virtual void unsubscribe(dispatcher_interface* _disp) = 0;
		
	};



	class  subscriber : public subscriber_interface
	{
	public:






	};







	
}