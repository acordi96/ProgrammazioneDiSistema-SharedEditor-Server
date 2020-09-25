#include <iostream>
#include "Connection.h"

#define serverPort "3000"
int main() {
    try
    {
        boost::asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v4(), std::atoi(serverPort));
        Connection server(io_context, endpoint);
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}