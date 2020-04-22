//
// Created by Sam on 01/apr/2020.
//

#include "Server.h"

Server::Server(boost::asio::io_context &io_context, const tcp::endpoint &endpoint): acceptor_(io_context, endpoint)  {
    connection();
}

void Server::connection() {
    std::cout << "Sono in attesa di connessione... " << std::endl;
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket){
        if(ec){
            //non si riesce a connettere al server per vari motivi
            std::cout << "ERRORE " << ec.message() << std::endl;
            return ;
        }else {
            //LEGGO LA RICHIESTA E LA PROCESSO
            std::cout << "CONNESSO AL CLIENT CON ID "<< count << std::endl;
            std::make_shared<HandleRequest>(std::move(socket), room)->start(++count);
        }
        connection();
    });
}