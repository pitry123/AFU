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

    class tcpServer {
    public:
        afu::smart_event<std::string> handler;

        tcpServer(uint16_t port, std::function<void(std::string)> func)
            : m_io_context(),
            m_acceptor(m_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
        {
            handler += func;
            start_accept();
            std::cout << "TCP Server Started on Port: " << port << "\n";
        }

        ~tcpServer() {
            m_is_running = false;
            m_io_context.stop();
            if (m_io_thread.joinable()) {
                m_io_thread.join();
            }
        }

        void start() {
            m_is_running = true;

            m_io_thread = std::thread([this]() {
                m_io_context.run(); // Start handling I/O operations.
                });
        }

    private:
        void start_accept() {
            m_acceptor.async_accept(
                [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                    if (!ec) {
                        std::cout << "New Connection Accepted.\n";
                        std::make_shared<tcpSession>(std::move(socket), handler)->start();
                    }
                    else {
                        std::cerr << "Accept Error: " << ec.message() << std::endl;
                    }
                    start_accept(); // Continue accepting new connections.
                });
        }

        class tcpSession : public std::enable_shared_from_this<tcpSession> {
        public:
            tcpSession(boost::asio::ip::tcp::socket socket, afu::smart_event<std::string>& handler)
                : m_socket(std::move(socket)),
                m_handler(handler) {}

            void start() {
                do_read();
            }

        private:
            void do_read() {
                auto self = shared_from_this();
                m_socket.async_read_some(boost::asio::buffer(m_recv_buffer),
                    [this, self](boost::system::error_code ec, std::size_t bytes_received) {
                        if (!ec) {
                            std::string msg(m_recv_buffer.data(), bytes_received);
                            m_handler.invoke(msg);
                            do_read(); // Continue reading data.
                        }
                        else {
                            std::cerr << "Session Read Error: " << ec.message() << std::endl;
                        }
                    });
            }

            boost::asio::ip::tcp::socket m_socket;
            afu::smart_event<std::string>& m_handler;

            std::array<char, 1024> m_recv_buffer;
        };

        boost::asio::io_context m_io_context;
        boost::asio::ip::tcp::acceptor m_acceptor;

        std::thread m_io_thread;

        bool m_is_running = false;
    };
    class tcpClient {
    public:
        afu::smart_event<std::string> handler;

        tcpClient(const std::string& remote_ip, uint16_t remote_port, std::function<void(std::string)> func)
            : m_io_context(),
            m_socket(m_io_context),
            m_remote_ip(remote_ip),
            m_remote_port(remote_port)
        {
            handler += func;

            connect_to_server();
            std::cout << "TCP Client Connected to Server.\n";
        }

        ~tcpClient() {
            m_is_running = false;
            m_io_context.stop();
            if (m_io_thread.joinable()) {
                m_io_thread.join();
            }
            m_socket.close();
        }

        void start() {
            m_is_running = true;

            m_io_thread = std::thread([this]() {
                m_io_context.run(); // Run the I/O context to process events.
                });

            start_receive();
        }

        bool send(const std::string& message) {
            try {
                boost::asio::write(m_socket, boost::asio::buffer(message));
            }
            catch (const std::exception& e) {
                std::cerr << "Send Error: " << e.what() << std::endl;
                return false;
            }
            return true;
        }

    private:
        void connect_to_server() {
            try {
                boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(m_remote_ip), m_remote_port);
                m_socket.connect(endpoint);
            }
            catch (const std::exception& e) {
                std::cerr << "Connect Error: " << e.what() << std::endl;
            }
        }

        void start_receive() {
            m_socket.async_read_some(boost::asio::buffer(m_recv_buffer),
                [this](const boost::system::error_code& error, std::size_t bytes_received) {
                    if (!error) {
                        std::string recv_msg(m_recv_buffer.data(), bytes_received);
                        handler.invoke(recv_msg);
                        start_receive(); // Continue receiving data.
                    }
                    else {
                        std::cerr << "Receive Error: " << error.message() << std::endl;
                    }
                });
        }

        boost::asio::io_context m_io_context;
        boost::asio::ip::tcp::socket m_socket;

        std::string m_remote_ip;
        uint16_t m_remote_port;

        std::array<char, 1024> m_recv_buffer;

        std::thread m_io_thread;

        bool m_is_running = false;
    };

}