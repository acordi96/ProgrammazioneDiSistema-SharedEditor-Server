//
// Created by Sam on 01/apr/2020.
//

#include <thread_db.h>
#include "Connection.h"

Connection::Connection(boost::asio::io_context &io_context, const tcp::endpoint &endpoint) : acceptor_(io_context,
                                                                                                       endpoint) {
    connection();
}

void Connection::connection() {
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
        if (ec) {
            //non si riesce a connettere al server per vari motivi
            std::cout << "ERRORE " << ec.message() << std::endl;
            return;
        } else {
            //LEGGO LA RICHIESTA E LA PROCESSO
            countId++;
            std::cout << Connection::output(std::this_thread::get_id()) << "CONNECTED TO NEW CLIENT: " << countId
                      << std::endl;
            std::make_shared<HandleRequest>(std::move(socket))->start(countId);
        }
        connection();
    });
}

std::string Connection::output(std::thread::id threadId) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string time = std::ctime(&now);
    std::stringstream ss;
    ss << threadId;
    return "{" + ss.str() + "} [" + time.erase(time.length() - 1) + "] - ";
}
