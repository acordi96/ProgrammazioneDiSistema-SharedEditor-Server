//
// Created by Sam on 01/apr/2020.
//

#include "HandleRequest.h"
#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"
HandleRequest::HandleRequest(tcp::socket socket, Room& room) : socket(std::move(socket)), room_(room){

}

void HandleRequest::start(int participantId) {
    //dai un id al partecipante
    shared_from_this()->setSiteId(participantId);
    room_.join(shared_from_this());
    do_read_header();
}
void HandleRequest::do_read_header() {
    auto self(shared_from_this());
    memset(read_msg_.data(), 0, read_msg_.length());
    boost::asio::async_read(socket,boost::asio::buffer(read_msg_.data(), message::header_length),[this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec && read_msg_.decode_header())
            {
                do_read_body();
            }
            else
            {
                room_.leave(shared_from_this());
            }
        });
}
void HandleRequest::do_read_body() {
    auto self(shared_from_this());
    boost::asio::async_read(socket,boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec)
            {
                std::cout << "Richiesta arrivata dal client: " << read_msg_.body() << std::endl;
                json messageFromClient = json::parse(read_msg_.body());
                std::string requestType = messageFromClient.at("operation").get<std::string>();
                int partecipantId = shared_from_this()->getId();
                //nella gestione devo tenere traccia del partecipante che invia la richiesta, quindi a chi devo rispondere e a chi no
                std::string response = handleRequestType(messageFromClient, requestType, partecipantId);
                if(requestType == "insert"){
                    sendAllClient(response, partecipantId);
                }else if(requestType=="request_login"  || requestType=="request_signup"){
                    sendAtClient(response);
                }


                do_read_header();
            }
            else
            {
                room_.leave(shared_from_this());
            }
        });
}

void HandleRequest::sendAtClient(std::string j_string){
    std::size_t len = j_string.size();
    message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), j_string.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    std::cout <<"Risposta al sono client: "<< msg.body() << std::endl;
    shared_from_this()->deliver(msg);

}
void HandleRequest::sendAllClient(std::string j_string, const int &id) {
    std::size_t len = j_string.size();
    message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), j_string.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    std::cout << "Risposta indiriizzata a tutti i client: " << msg.body() << std::endl;
    room_.deliverToAll(msg, id);
}

void HandleRequest::do_write()
{
    //scorro la coda dal primo elemento inserito
    boost::asio::async_write(socket,boost::asio::buffer(write_msgs_.front().data(),write_msgs_.front().length()),[this](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec)
        {
            //rimuove il primo elemento dato che lo ha inviato
            write_msgs_.pop_front();
            //controlla se ce ne sono altri, se si scrivi ancora
            if (!write_msgs_.empty()) {
                do_write();
            }
        }
        else
        {
            std::cout << ec.message();
            socket.close();
        }
    });
}
std::string HandleRequest::handleRequestType(const json &js, const std::string &type_request, int partecipantId) {
    if(type_request=="request_login"){
        //prendi username e psw
        std::string username, password;
        username = js.at("username").get<std::string>();
        password = js.at("password").get<std::string>();
        QString colorParticiapant = "#00ffffff";
        std::string resDB = manDB.handleLogin(username, password, colorParticiapant);
        if(resDB == "LOGIN_SUCCESS"){
            shared_from_this()->setUsername(username);
            shared_from_this()->setColor(colorParticiapant.toStdString());
        }
        json j = json{{"response", resDB}, {"username", username}, {"colorUser", colorParticiapant.toStdString()}};
        std::string j_string = j.dump();
        return  j_string;
    }
    if(type_request=="request_signup"){
        //prendi username e psw
        std::string username, password, email;
        username = js.at("username").get<std::string>();
        password = js.at("password").get<std::string>();
        email = js.at("email").get<std::string>();
        std::string resDB = manDB.handleSignup(email, username, password);
        json j = json{{"response", resDB}};
        std::string j_string = j.dump();
        return  j_string;
    }
    if(type_request=="R_LOGOUT"){

    }
    if(type_request=="insert"){
        std::pair<int, char> message = js.at("corpo").get<std::pair<int, char>>();
        MessageSymbol messS = localInsert(message.first, message.second);
        room_.send(messS);
        room_.dispatchMessages();
        std::pair<int, char> corpo(messS.getNewIndex(), message.second);
        json j = json{{"response", "insert_res"}, {"corpo", corpo}};
        std::string j_string = j.dump();
        return j_string;
    }
    if(type_request=="remove"){
        int index = js.at("corpo").get<int>();
        MessageSymbol messS = localErase(index);
        room_.send(messS);
        room_.dispatchMessages();
        json j = json{{"response", "insert_res"}, {"corpo", index}};
        std::string j_string = j.dump();
        return j_string;
    }
    return type_request;
}
void HandleRequest::deliver(const message& msg)
{
    bool write_in_progress = !write_msgs_.empty();
    //aggiungo un nuovo elemento alla fine della cosa
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
        do_write();
    }
}