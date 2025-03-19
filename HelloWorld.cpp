#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <unordered_map>

class Server {
public:
    explicit Server(unsigned short port, std::size_t thread_pool_size = std::thread::hardware_concurrency())
        : io_context_(thread_pool_size),
          acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
          thread_pool_size_(thread_pool_size) {
        std::cout << "Server starting on port " << port << " with " << thread_pool_size_ << " threads.\n";
        startAccept();
    }

    void run() {
        // Launch the thread pool
        for (std::size_t i = 0; i < thread_pool_size_; ++i) {
            thread_pool_.emplace_back([this]() { io_context_.run(); });
        }

        // Join all threads
        for (auto& thread : thread_pool_) {
            thread.join();
        }
    }

private:
    void startAccept() {
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
        acceptor_.async_accept(*socket, boost::asio::bind_executor(strand_,
            [this, socket](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "New client connected.\n";
                    handleSession(socket);
                }
                startAccept(); // Accept the next connection
            }));
    }

    void handleSession(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
        auto buffer = std::make_shared<std::string>("Hello, Client!\n");
        boost::asio::async_write(*socket, boost::asio::buffer(*buffer),
            boost::asio::bind_executor(strand_,
                [socket, buffer](boost::system::error_code ec, std::size_t) {
                    if (!ec) {
                        std::cout << "Message sent to client.\n";
                    }
                }));
    }

    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_{io_context_.get_executor()};
    std::size_t thread_pool_size_;
    std::vector<std::thread> thread_pool_;
};

int main() {
    try {
        unsigned short port = 8080;
        std::size_t thread_pool_size = std::thread::hardware_concurrency();

        Server* server = new Server(port, thread_pool_size);
        server->run();

        delete server;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}
