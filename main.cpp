#include <iostream>
#include "Server.h"
int main() {
    try
    {
        boost::asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v4(), std::atoi("3000"));
        Server server(io_context, endpoint);
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}