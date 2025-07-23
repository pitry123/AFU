#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include <map>
#include <vector>
#include <utils/collection.hpp>
#include "dispatcher.hpp"


namespace afu
{
	
	using byte_vector = std::vector<unsigned char>;



	class subscription_data :
		public std::enable_shared_from_this<subscription_data>
	{
	private:

		byte_vector m_buffer;


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


	class rowdata_async_action:
		public afu::async_action_context
	{

		std::function<void(const std::shared_ptr<subscription_data>& _data)> m_callback;
		std::shared_ptr<subscription_data> m_data;

	public:
		rowdata_async_action() = default;

		rowdata_async_action(std::function<void(const std::shared_ptr<subscription_data>& _data)> _callback, std::shared_ptr<subscription_data> _data) :
			m_callback(_callback),
			m_data(_data)
		{}




		virtual void run_action() override
		{
			try
			{
				m_callback(m_data);
			}
			catch (const std::exception&)
			{
				throw std::runtime_error("callback exception\n");
			}	
		}
	};


	class  subscriber : 
		public std::enable_shared_from_this<subscriber>
	{
	private:

		static constexpr int POOL_BUFFER_SIZE = 10;

		std::shared_ptr<afu::cyclicBuffer<subscription_data>> m_pool_buffer;
		std::queue<std::shared_ptr<subscription_data>> m_data_to_send;

		size_t m_data_size;

		std::map<std::thread::id, std::pair< std::shared_ptr<afu::dispatcher>, std::function<void(const std::shared_ptr<subscription_data>&)>>> m_subscription_map;

		std::condition_variable m_data_notify_cv;
		std::mutex m_data_notify_mutex;
		std::thread m_data_notify_th;
		
		bool m_is_runnning;
		bool m_need_update;


	public:

		subscriber(size_t _data_size):
			m_pool_buffer(new afu::cyclicBuffer<subscription_data>(POOL_BUFFER_SIZE)),
			m_data_size(_data_size),
			m_is_runnning(false),
			m_need_update(false)

		{
			m_data_notify_th = std::thread([&]()
				{
					m_is_runnning = true;
					while (m_is_runnning)
					{
						notify();
					}
				}
			);
		}

		virtual void subscribe(std::shared_ptr<afu::dispatcher> _disp, std::function<void(const std::shared_ptr<subscription_data>&)> _func)
		{
			if (_disp == nullptr)
				throw std::runtime_error("_disp == nullptr");

			if (_func == nullptr)
				throw std::runtime_error("_disp == nullptr");

			auto disp_id = _disp->get_id();
			if (m_subscription_map.find(disp_id) == m_subscription_map.end())
			{
				m_subscription_map[disp_id] = { _disp, _func };
			}
			
		}
		

		template<typename T>
	    void write(const T& _val)
		{
			if (sizeof(T) != m_data_size)
				throw std::invalid_argumenta("sizeof(T) != m_data_size");
			
			auto v = std::make_shared<subscription_data>(sizeof(T));
			v->write(_val);
			{
				std::lock_guard<std::mutex> lock(m_data_notify_mutex);
				m_pool_buffer->push(*v);
				m_data_to_send.push(v)
			}
			m_need_update = true;
			m_data_notify_cv.notify_one();
		}

		void notify()
		{
			std::unique_lock<std::mutex> lock(m_data_notify_mutex);
			m_data_notify_cv.wait(lock, [&]()
				{
					return m_need_update && (m_data_to_send.size() > 0);
				});
			
			lock.unlock();
			while (!m_data_to_send.empty())
			{
				auto subData =  m_data_to_send.front();
				m_data_to_send.pop();
				for (const auto& disp : m_subscription_map)
				{
					rowdata_async_action ac(disp.second.second, subData);
					disp.second.first->begin_invoke(ac);
				}
			}

			m_need_update = false;
		}

		template<typename T>
		const T& get_last()
		{
			if (sizeof(T) != m_data_size)
				throw std::invalid_argumenta("sizeof(T) != m_data_size");
			
			return m_pool_buffer->front().read<T>();
		}


		template<typename T>
		const T& get_last_i()
		{
			if (sizeof(T) != m_data_size)
				throw std::invalid_argumenta("sizeof(T) != m_data_size");

			return m_pool_buffer->front().read<T>();
		}



	};







	
}