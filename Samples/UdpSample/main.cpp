#include <iostream>
#include <communication/udp.hpp>	
#include <chrono>
#include <utils/event.hpp>
#include <subscription/subscription.hpp>

int main()
{


	std::cout << "Start\n";
	afu::udpCommunication _udp_(6333, [&](std::string _a) 
		{
			std::cout << "A1 Msg:" << _a << std::endl;

		});

	_udp_.handler += [&](std::string _ss)
		{
			std::cout << "B1 Msg:" << _ss << std::endl;
		};
	_udp_.start();

	std::chrono::milliseconds _duration(200);
	while (true)
	{
		std::this_thread::sleep_for(_duration);
	}
	std::cout  << "bye\n";
	return 0;
}