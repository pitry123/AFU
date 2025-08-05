#pragma once
// Schwartz Liran 


#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <chrono>
#include <interface/worker_interface.h>




namespace afu
{

	class async_action_context 
	{
	public:
		async_action_context() = default;

		~async_action_context() = default;

		void begin_invoke()
		{
			run_action();
		}

	protected:

		virtual void run_action() = 0;

	};


	class dispatcher : public worker_interface
	{
	public:


		dispatcher() = default;


		virtual void add_action(const std::shared_ptr<async_action_context>& _action)
		{
			std::lock_guard<std::mutex> lock(m_lock_invoke_thread);
			m_action_q.push(_action);
		}

		virtual void add_list_action(const std::vector<std::shared_ptr<async_action_context>>& _action)
		{
			std::lock_guard<std::mutex> lock(m_lock_invoke_thread);
			for(const auto& ac : _action)
				m_action_q.push(ac);
			
		}

		
		~dispatcher()
		{
			m_still_running = false;
			if (m_invoke_thread.joinable())
			{
				m_invoke_thread.join();
			}
		}

		void begin_invoke()
		{
			if (m_invoke_thread.joinable() && 
				m_still_running == true &&
				m_action_q.empty() == false)
			{
				m_invoke_thread_cv.notify_all();
			}
		}

		void begin_invoke(const std::shared_ptr<async_action_context>& _action)
		{
			add_action(_action);
			if (m_invoke_thread.joinable() && 
				m_still_running == true && 
				m_action_q.empty() == false)
			{
				m_invoke_thread_cv.notify_all();
			}
		}
		virtual void init() override
		{
			// TBD
		}

		virtual void stop() override
		{
			m_still_running = false;
			if (m_invoke_thread.joinable())
			{
				m_invoke_thread.join();
			}
		}


		virtual void start() override
		{
			try
			{
				m_invoke_thread = std::thread([&]()
					{
						m_id = std::this_thread::get_id();
						m_still_running = true;
						while (m_still_running)
						{
							std::unique_lock<std::mutex> lock(m_lock_invoke_thread);
							m_invoke_thread_cv.wait_for(lock, std::chrono::seconds(5), [&]()
								{
									return (m_action_q.empty() == false);
								}
							);

							while (!m_action_q.empty())
							{
								auto& action = m_action_q.front();
								invoke(action);
								m_action_q.pop();
							}
						}
					});
			}
			catch (const std::exception&)
			{

			}
		}

        virtual void join()
        {
            if (m_invoke_thread.joinable())
                m_invoke_thread.join();
        }

		std::thread::id get_id()
		{
			if (m_invoke_thread.joinable())
				return m_id;

			return std::thread::id();
		}

	private:


		bool invoke(const std::shared_ptr<afu::async_action_context>& _action)
		{
			try
			{
				_action->begin_invoke();
			}
			catch (const std::exception&)
			{
				return false;
			}
			
			return true;
		}

		std::thread m_invoke_thread;
		std::mutex m_lock_invoke_thread;
		std::condition_variable m_invoke_thread_cv;
		std::queue<std::shared_ptr<async_action_context>> m_action_q;

		std::atomic_bool m_still_running;
		std::thread::id m_id;

	};
	
	
}
