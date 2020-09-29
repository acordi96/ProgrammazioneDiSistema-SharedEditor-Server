#include <iostream>
#include "SocketManager.h"

#define serverPort "3000"

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v4(), std::atoi(serverPort));
        SocketManager server(io_context, endpoint);
        std::cout << SocketManager::output() << "SERVER START, WAITING FOR CLIENT CONNECTIONS ON PORT " << serverPort
                  << std::endl;
        io_context.run();
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}