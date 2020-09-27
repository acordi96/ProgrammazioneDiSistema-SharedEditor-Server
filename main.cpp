#include <iostream>
#include "Connection.h"

#define serverPort "3000"

int main() {
    const auto cores = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    try {
        boost::asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v4(), std::atoi(serverPort));
        Connection server(io_context, endpoint);
        std::cout << Connection::output(std::this_thread::get_id()) << "SERVER START, " << std::to_string(cores)
                  << " CORES"
                  << std::endl;
        for (int i = 0; i < cores; i++) {
            threads.emplace_back([&io_context, &i]() {
                std::cout << Connection::output(std::this_thread::get_id())
                          << "THREAD CREATED, WAITING FOR CLIENT CONNECTIONS ON PORT " << serverPort
                          << std::endl;
                io_context.run();
            });
        }
        for (int i = 0; i < cores; i++) {
            threads[i].join();
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}