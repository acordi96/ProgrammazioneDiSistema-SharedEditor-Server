//
// Created by Sam on 01/apr/2020.
//

#ifndef SERVERPDS_SERVER_H
#define SERVERPDS_SERVER_H

#include <iostream>
#include <string>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include "HandleRequest.h"
#include "Room.h"
using boost::asio::ip::tcp;

//QUESTA E' LA CLASSE DI PARTENZA!
class Server {
private:
    int count = 1;
    tcp::acceptor acceptor_;
    void connection();
public:
    Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint);
    static std::string getTime();
};


#endif //SERVERPDS_SERVER_H
