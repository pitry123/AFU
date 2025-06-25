#include <iostream>
#include <communication/tcp.hpp>


// if N > 0 is Client tester else Server
static constexpr int TEST_CASE_N = 1;

int main()
{
	
	if (TEST_CASE_N)
	{

		afu::tcpClient client("127.0.0.1", 8080, [](const std::string& msg) {
			std::cout << "A Client Received: " << msg << std::endl;
			});


		client.handler += [](const std::string& msg) {
			std::cout << "B Client Received: " << msg << std::endl;
			};
		client.start();
		client.send("Hello, Server!");

		std::chrono::milliseconds _duration(200);
		while (true)
		{
			std::this_thread::sleep_for(_duration);
		}
		std::cout << "bye\n";
	}
	else
	{

		afu::tcpServer server(8080, [](const std::string& msg) {
			std::cout << "Server Received: " << msg << std::endl;
			});
		server.start();

		std::chrono::milliseconds _duration(200);
		while (true)
		{
			std::this_thread::sleep_for(_duration);
		}
		std::cout << "bye\n";
	}




	std::cout  << "hiiiiii";
	return 0;
}