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
#include "Message.h"

#include <boost/thread.hpp>


#include "SocketManager.h"
#include "Server.h"
#include "Participant.h"
#include "../Libs/json.hpp"

using boost::asio::ip::tcp;
using json = nlohmann::json;

class HandleRequest : public Participant, public std::enable_shared_from_this<HandleRequest> {
private:
    //attributi per far funzionare tutto
    tcp::socket socket;
    Message read_msg_;
    message_queue write_msgs_;

    //metodi
    void do_read_header();

    void do_read_body();

    std::string handleRequestType(const json &json, const std::string &type_request);

    void sendAtClient(const std::string &response);

    void sendAtClientUsername(const std::string &response, const std::string &username);

    void sendAllOtherClientsOnFile(const std::string &response);

    void sendAllClientsOnFile(const std::string &response);

public:
    void start(int paticipantId);

    void do_write();

    explicit HandleRequest(tcp::socket socket);

    void deliver(const Message &msg);
};


#endif //SERVERPDS_HANDLEREQUEST_H
