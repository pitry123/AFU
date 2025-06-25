#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <mutex>

// Schwartz Liran 
// Convertors : util hpp file to convert between any obj
namespace afu
{

	template<typename... Args>
	class smart_event
	{
	public:
		using GenericCallBack =  std::function<void(Args...)>;

		void operator+=(GenericCallBack _func)
		{
			std::lock_guard<std::mutex> lock(m_lock);
			m_func.push_back(std::make_shared<GenericCallBack>(_func));
		}

		void operator-=(const GenericCallBack& _func) {
			std::lock_guard<std::mutex> lock(m_lock);
			m_func.erase(std::remove_if(m_func.begin(), m_func.end(),
				[&_func](const auto& func) {
					return func.target_type() == _func.target_type() &&
						func.target<void(Args...)>() == _func.target<void(Args...)>();
				}),
				m_func.end());
		}


		void invoke(Args... args)
		{
			std::lock_guard<std::mutex> lock(m_lock);
			for (const auto& f : m_func)
			{
				try 
				{
					(*f)(args...);
				}
				catch (const std::exception& e) {
					std::cerr << "Error in callback: " << e.what() << std::endl;
				}
			}
		}

		smart_event() = default;


	private:
		std::vector<std::shared_ptr<GenericCallBack>> m_func;
		std::mutex m_lock;

	};
}



