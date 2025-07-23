#pragma once
// Schwartz Liran 

#include <iostream>
#include <vector>
#include <functional>
#include <mutex>
#include <subscription/subscription.hpp>


namespace afu
{
	class dispatcher_base : public dispatcher 
	{

	public:

		bool subscribe(const std::shared_ptr<subscriber>& _sub, std::function<void(const std::shared_ptr<subscription_data>&) > _callback)
		{
			if (_sub == nullptr || _callback == nullptr)
				return false;
			
			_sub->subscribe(this, _callback);
			return true;
		}
	};
}



