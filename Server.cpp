//
// Created by Sam on 01/apr/2020.
//

#include "Server.h"

Server::Server(boost::asio::io_context &io_context, const tcp::endpoint &endpoint) : acceptor_(io_context, endpoint) {
    connection();
}

void Server::connection() {
    std::cout << Server::getTime() << "WAITING for client CONNECTION" << std::endl;
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
        if (ec) {
            //non si riesce a connettere al server per vari motivi
            std::cout << "ERRORE " << ec.message() << std::endl;
            return;
        } else {
            //LEGGO LA RICHIESTA E LA PROCESSO
            std::cout << Server::getTime() << "CONNECTED to new client: " << count << std::endl;
            std::make_shared<HandleRequest>(std::move(socket), room)->start(++count);
        }
        connection();
    });
}

std::string Server::getTime() {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string time = std::ctime(&now);
    return "[" + time.erase(time.length() - 1) + "] - ";
}
