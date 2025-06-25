#pragma once
#include <iostream>
#include <queue>
#include <array>
#include <mutex>
#include <string>
#include <thread>
#include <utils/event.hpp>
#include <boost/asio.hpp>
#include <condition_variable>



namespace afu
{
	
	class udpCommunication
	{
	public:

		afu::smart_event<std::string> handler;

		udpCommunication(uint32_t _local_port, std::function<void(std::string)> _func) :
			m_io_context(),
			m_socket(m_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), _local_port)),
			m_local_port(_local_port)
		{
			handler += _func;

			std::cout << "Connected.\n";
		}

		udpCommunication(uint32_t _local_port, std::string _remote_ip, uint32_t _remote_port,  std::function<void(std::string)> _func) :
			m_io_context(), 
			m_socket(m_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), _local_port)),
			m_remote_ip(_remote_ip),
			m_remote_port(_remote_port),
			m_local_port(_local_port)
		{
			handler += _func;
			std::cout << "Connected.\n";
		}

		~udpCommunication()
		{
			m_is_running = false;
			m_io_context.stop();
			m_io_context_thread->join();
			m_dispatching_thread->join();
			m_socket.close();
		}

		void start()
		{
			m_io_context_thread = std::make_shared<std::thread>([&]()
				{
					m_is_running = true;
					while (m_is_running)
					{
						start_receive();
						m_io_context.run();
					}
				}
			);

			m_dispatching_thread = std::make_shared<std::thread>([&]()
				{
					m_is_running = true;
					while (m_is_running)
					{
						std::unique_lock<std::mutex> lock(m_cv_mutex);
						m_cv.wait(lock,[&]()
							{
								return (!m_recving_queue.empty());
							});
						while (!m_recving_queue.empty())
						{
							std::string val = m_recving_queue.front();
							handler.invoke(val);
							m_recving_queue.pop();

						}

					}
				}
			);
		}

		bool send(const std::string& msg, std::string _remote_ip = "", int _remote_port = -1)
		{
			try
			{
				if (!_remote_ip.empty())
					m_remote_ip = _remote_ip;

				if (_remote_port >= 0)
					m_remote_port = _remote_port;

				boost::asio::ip::udp::endpoint client(
					boost::asio::ip::address::from_string((m_remote_ip.empty() ? "127.0.0.1" : m_remote_ip)),
					m_remote_port);

				m_socket.send_to(boost::asio::buffer(msg), client);
			}
			catch (std::exception& e) {
				std::cerr << "Client exception: " << e.what() << std::endl;
				return false;
			}
			return true;
 		}


		void async_send(const std::string& message)
		{
			std::lock_guard<std::mutex> lock(m_sender_queue_mutex); // Lock for thread safety
			m_sender_queue.push(message);
			if (!m_sender_queue.empty()) {
				m_io_context.dispatch([this]() {countiue_send(); });
			}
		}

		std::string recv(bool echo = false)
		{
			try
			{
				char msg[1024] = { 0 };
				boost::asio::ip::udp::endpoint recv_client;
				std::string res;

				size_t len = m_socket.receive_from(boost::asio::buffer(msg), recv_client);
				if (len > 0)
				{
					res = msg;
					if (m_print)
					{
						std::cout << "Received: " << std::string(msg, len)
							<< " from " << recv_client.address().to_string() << ":" << recv_client.port() << std::endl;
					}

					if (echo)
						m_socket.send_to(boost::asio::buffer(res), recv_client);

					return res;
				}
			}
			catch (const std::exception& e)
			{
				std::cerr << "Exception: " << e.what() << std::endl;
			}
 			return "";
 		}

	private:

		void countiue_send()
		{
			std::lock_guard<std::mutex> lock(m_sender_queue_mutex); // Lock for thread safety

			if (!m_sender_queue.empty())
			{
				// Get the message from the front of the queue
				const std::string message = m_sender_queue.front();
				m_sender_queue.pop();

				boost::asio::ip::udp::endpoint m_endpoint(boost::asio::ip::address::from_string(m_remote_ip), m_remote_port);

				// Send the message asynchronously
				m_socket.async_send_to(boost::asio::buffer(message), m_endpoint,
					[this](const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
					{
						if (!error)
							// Continue sending if there are more messages in the queue
							countiue_send();
						else
							std::cerr << "Send Error: " << error.message() << std::endl;

					});
			}
		}

		void start_receive() 
		{
			m_socket.async_receive_from(boost::asio::buffer(m_recv_buffer), m_recv_endpoint,
				[&](const boost::system::error_code& error, std::size_t bytes_received)
				{
					if (!error) 
					{
						std::string recv_msg(m_recv_buffer.data(), bytes_received);
						{
							std::lock_guard < std::mutex > lk(m_cv_mutex);
							m_recving_queue.push(recv_msg);
						}
						m_cv.notify_all();
						start_receive();
					}
					else
					{
						std::cerr << "Error on receive: " << error.message() << std::endl;
					}
				});
		}


		// Getter setter Attributes

		std::string getRemoteIp() { return m_remote_ip; }

		uint32_t getRemotePort() { return m_remote_port; }

		void setRemoteIp(const std::string& _ip) { m_remote_ip = _ip; }

		void setRemotePort(uint32_t _port) { m_remote_port = _port; }

		uint32_t getLocalPort() { return m_local_port; }



	private:
		boost::asio::io_context m_io_context;
		boost::asio::ip::udp::socket m_socket;
		std::string m_remote_ip = "";
		uint32_t m_remote_port = 0;
		uint32_t m_local_port = 0;

		std::shared_ptr<std::thread> m_io_context_thread;
		std::shared_ptr<std::thread> m_dispatching_thread;

		// async send
		std::queue<std::string> m_sender_queue;
		std::mutex m_sender_queue_mutex;


		//async recv
		std::mutex m_cv_mutex;
		std::condition_variable m_cv;
		std::array<char, 1024> m_recv_buffer;
		std::queue<std::string> m_recving_queue;
		boost::asio::ip::udp::endpoint m_recv_endpoint;
		


		bool m_print = true;


		bool m_is_running = false;
	};
}