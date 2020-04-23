//
// Created by Sam on 01/apr/2020.
//

#ifndef SERVERPDS_HANDLEREQUEST_H
#define SERVERPDS_HANDLEREQUEST_H

#include <boost/asio.hpp>
#include <iostream>
#include <set>
#include <QtCore/QRegExp>
#include "ManagementDB.h"
#include "message.h"
#define MAXBUFL 255
#include <boost/thread.hpp>

#include "Server.h"
#include "Room.h"
#include "Participant.h"
#include "lib/json.hpp"
using boost::asio::ip::tcp;
using json = nlohmann::json;

class HandleRequest : public Participant, public std::enable_shared_from_this<HandleRequest>{
private:
    //attributi per far funzionare tutto
    Room& room_;
    tcp::socket socket;
    message read_msg_;
    message_queue write_msgs_;
    //gestione db
    ManagementDB manDB;
    //metodi
    void do_read_header();
    void do_read_body();
    std::string handleRequestType(const json& json, const std::string& type_request, int partecipantId);
    void sendAtClient(std::string response);
    void sendAllClient(std::string responde, const int &id);
public:
    void start(int paticipantId);
    void do_write();
    explicit HandleRequest(tcp::socket socket, Room& room);
    void deliver(const message& msg);
};


#endif //SERVERPDS_HANDLEREQUEST_H