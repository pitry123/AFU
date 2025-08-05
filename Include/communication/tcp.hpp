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

    class tcpClientAsync {
    public:
        afu::smart_event<std::string, size_t> handler;

        tcpClientAsync(const std::string& remote_ip, uint16_t remote_port, std::function<void(std::string, size_t)> func)
            : m_io_context(),
            m_socket(m_io_context),
            m_remote_ip(remote_ip),
            m_remote_port(remote_port)
        {
            handler += func;

            connect_to_server();
            std::cout << "TCP Client Connected to Server.\n";
        }

        ~tcpClientAsync() {
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

        bool is_conneted() {
            return m_connected;
        }

    private:
        void connect_to_server() {
            try {
                boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(m_remote_ip), m_remote_port);
                m_socket.connect(endpoint);
                m_connected = true;
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
                        handler.invoke(recv_msg, bytes_received);
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
        bool m_connected = false;
    };

    class tcpClient {
    public:
        tcpClient(const std::string& host, int port)
            : m_host(host), m_port(port), m_socket(m_io) {}

        ~tcpClient() {
            disconnect();
        }

        bool connect() {
            try {
                boost::asio::ip::tcp::resolver resolver(m_io);
                auto endpoints = resolver.resolve(m_host, std::to_string(m_port));
                boost::asio::connect(m_socket, endpoints);
                is_connected = true;
                return true;
            } catch (std::exception& e) {
                std::cerr << "Connect failed: " << e.what() << std::endl;
                is_connected = false;
                return false;
            }
        }

        void disconnect() {
            std::lock_guard<std::mutex> lock(m_mutex);
            is_connected = false;
            if (m_socket.is_open()) {
                boost::system::error_code ec;
                m_socket.close(ec);
            }
        }

        bool send(const std::string& message) {
            std::lock_guard<std::mutex> lock(m_mutex);
            try {
                boost::asio::write(m_socket, boost::asio::buffer(message));
                return true;
            } catch (...) {
                return false;
            }
        }

        std::string receive(std::size_t num_bytes) {
            std::lock_guard<std::mutex> lock(m_mutex_send);
            std::vector<char> buf(num_bytes);
            try {
                std::size_t received = boost::asio::read(m_socket, boost::asio::buffer(buf, num_bytes));
                return std::string(buf.begin(), buf.begin() + received);
            } catch (...) {
                return {};
            }
        }



    bool is_conneted()
    {
        return is_connected;
    }


    private:
        std::string m_host;
        int m_port;
        boost::asio::io_context m_io;
        boost::asio::ip::tcp::socket m_socket;
        std::mutex m_mutex;
        std::mutex m_mutex_send;

        bool is_connected = false;
    };

}
