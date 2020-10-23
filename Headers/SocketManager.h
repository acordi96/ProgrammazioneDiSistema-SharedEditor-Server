//
// Created by Sam on 01/apr/2020.
//

#ifndef SERVERPDS_SOCKETMANAGER_H
#define SERVERPDS_SOCKETMANAGER_H

#include <iostream>
#include <string>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include "HandleRequest.h"
#include "Server.h"

using boost::asio::ip::tcp;

class SocketManager {
private:
    int countId = 0;
    tcp::acceptor acceptor_;

    void connection();

public:
    SocketManager(boost::asio::io_context &io_context, const tcp::endpoint &endpoint);

    static std::string output();
};


#endif //SERVERPDS_SOCKETMANAGER_H
