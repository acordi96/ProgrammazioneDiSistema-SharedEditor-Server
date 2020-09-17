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
    Room room;
    tcp::acceptor acceptor_;
    void connection();
    static std::map<std::string, Room*> roomsOfFile;
public:
    Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint);
    static std::string getTime();
   /* static std::map<std::string, Room*> getRoomsOfFile();
    static void insertParticipantIntoRoomsOfFile(const std::string&, participant_ptr);
    static void createRoomsOfFile(std::string, participant_ptr);*/
};


#endif //SERVERPDS_SERVER_H
