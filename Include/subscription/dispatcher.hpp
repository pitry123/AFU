#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include <vector>



namespace afu
{
	
	class dispatcher_interface :
		std::enable_shared_from_this<dispatcher_interface>
	{
	public: 

		~dispatcher_interface() = default;

		virtual bool invoke() = 0;
		
		virtual bool begin_invoke() = 0;		
	};


	class dispatcher_base : public dispatcher_interface
	{
	public:





	};
	
	
}