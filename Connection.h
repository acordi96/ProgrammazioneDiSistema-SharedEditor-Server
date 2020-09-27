//
// Created by Sam on 01/apr/2020.
//

#ifndef SERVERPDS_CONNECTION_H
#define SERVERPDS_CONNECTION_H

#include <iostream>
#include <string>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include "HandleRequest.h"
#include "Server.h"

using boost::asio::ip::tcp;

//QUESTA E' LA CLASSE DI PARTENZA!
class Connection {
private:
    int countId = 0;
    tcp::acceptor acceptor_;

    void connection();

public:
    Connection(boost::asio::io_context &io_context, const tcp::endpoint &endpoint);

    static std::string output(std::thread::id threadId);
};


#endif //SERVERPDS_CONNECTION_H
