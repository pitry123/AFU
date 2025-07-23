#pragma once
// Schwartz Liran 
namespace afu
{

	// Using for builder
	class worker_interface
	{
	public:

		worker_interface() = default;
		
		virtual void init() = 0;

		virtual void start() = 0;

		virtual void stop() = 0;

	};
}



