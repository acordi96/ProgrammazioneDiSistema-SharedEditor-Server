//
// Created by Sam on 01/apr/2020.
//

#include <fstream>
#include "HandleRequest.h"
#include <boost/filesystem.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>


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
                }
                else if(requestType == "remove"){
                    sendAllClient(response,partecipantId);
                }
                else if(requestType=="request_login"  || requestType=="request_signup" || requestType=="request_new_file" || requestType=="open_file"){
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
    std::cout <<"Risposta al client: "<< msg.body() << std::endl;
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
    if (type_request == "request_login") {
        //prendi username e psw
        std::string username, password;
        username = js.at("username").get<std::string>();
        password = js.at("password").get<std::string>();
        QString colorParticiapant = "#00ffffff";
        std::string resDB = manDB.handleLogin(username, password, colorParticiapant);
        std::list<std::string> risultato;
        if (resDB == "LOGIN_SUCCESS") {
            shared_from_this()->setUsername(username);
            shared_from_this()->setColor(colorParticiapant.toStdString());
            //recupero tutti i file dell'utente
            risultato = manDB.takeFiles(username);
        }
        json j = json{{"response",  resDB},
                      {"username",  username},
                      {"colorUser", colorParticiapant.toStdString()},
                      {"files",     risultato}};
        std::string j_string = j.dump();
        return j_string;
    } else if (type_request == "request_signup") {
        //prendi username e psw
        std::string username, email, password;
        username = js.at("username").get<std::string>();
        std::cout << "\n username ricevuto per la registrazione: " + username;
        password = js.at("password").get<std::string>();
        email = js.at("email").get<std::string>();
        QString colorParticiapant = "#00ffffff";
        std::string resDB = manDB.handleSignup(email, username, password);

        //se la registrazione va bene, creo la cartella personale per il nuovo utente
        if (resDB == "SIGNUP_SUCCESS") {
            std::cout << "\n creazione nuova cartella";
            std::string path = "C:/Users/gabriele/Desktop/PDS/Server/serverPDS/fileSystem/" + username;
            boost::filesystem::create_directory(path);
            shared_from_this()->setUsername(username);
        }
        json j = json{{"response",  resDB},
                      {"username",  username},
                      {"colorUser", colorParticiapant.toStdString()}};
        //json j = json{{"response", resDB}};
        std::string j_string = j.dump();
        return j_string;
    } else if (type_request == "R_LOGOUT") {
        json j = json{{"response", "logout"}};
        std::string j_string = j.dump();
        return j_string;
    } else if (type_request == "insert") {
        std::pair<int, char> message = js.at("corpo").get<std::pair<int, char>>();
        MessageSymbol messS = localInsert(message.first, message.second);
        room_.send(messS);
        room_.dispatchMessages();
        std::pair<int, char> corpo(messS.getNewIndex(), message.second);
        //scrivere sul file, problema recuperare il file su cui scrivere

        json j = json{{"response", "insert_res"},
                      {"corpo",    corpo}};
        std::string j_string = j.dump();
        return j_string;

    } else if (type_request == "remove") {
        /*
         int index = js.at("corpo").get<int>();
         MessageSymbol messS = localErase(index);
         room_.send(messS);
         room_.dispatchMessages();
         json j = json{{"response", "remove_res"}, {"corpo", index}};
         std::string j_string = j.dump();
         return j_string;
     */
        int startIndex = js.at("start").get<int>();
        int endIndex = js.at("end").get<int>();
        MessageSymbol messS = localErase(startIndex, endIndex);
        room_.send(messS);
        room_.dispatchMessages();
        json j = json{{"response", "remove_res"},
                      {"start",    startIndex},
                      {"end",      endIndex}};
        std::string j_string = j.dump();
        return j_string;
    } else if (type_request == "request_new_file") {
        std::cout << "Nuovo file\n";

        // creo il file nuovo
        std::string nomeFile =
                "C:/Users/gabriele/Desktop/PDS/Server/serverPDS/fileSystem/" + js.at("username").get<std::string>() +
                "/" + js.at("name").get<std::string>() + ".txt";
        if (boost::filesystem::exists(nomeFile)) {
            std::cout << "\nil file esiste già";
            json j = json{{"response", "new_file_already_exist"}};
            std::string j_string = j.dump();
            return j_string;
        } else {
            //creo il nuovo file
            boost::filesystem::ofstream oFile(nomeFile);

            //metto il file nel db con lo user
            std::cout << "\n username del nuovo file : " << js.at("username").get<std::string>();
            std::string resDB = manDB.handleNewFile(js.at("username").get<std::string>(),
                                                    js.at("name").get<std::string>());
            if (resDB == "FILE_INSERT_SUCCESS") {
                std::cout << "\nfile inserito nel db correttamente\n";

                // settare il current file
                this->setCurrentFile(nomeFile);

                json j = json{{"response", "new_file_created"}};
                std::string j_string = j.dump();
                return j_string;
            } else {
                json j = json{{"response", "errore_salvataggio_file_db"}};
                std::string j_string = j.dump();
                return j_string;
            }


        }
    } else if (type_request == "open_file") {
        std::string nomeFile =
                "C:/Users/gabriele/Desktop/PDS/Server/serverPDS/fileSystem/" + js.at("username").get<std::string>() +
                "/" + js.at("name").get<std::string>() + ".txt";

        std::string resDB = manDB.handleOpenFile(js.at("username").get<std::string>(),
                                                js.at("name").get<std::string>());
        if (resDB == "FILE_OPEN_SUCCESS") {
            std::cout << "\nfile aperto correttamente\n";

            // settare il current file
            this->setCurrentFile(nomeFile);

            json j = json{{"response", "file_opened"}};
            std::string j_string = j.dump();
            return j_string;
        } else {
            json j = json{{"response", "errore_apertura_file"}};
            std::string j_string = j.dump();
            return j_string;
        }




    }   else {
        std::cout << "nessun match col tipo di richiesta";
        json j = json{{"response","general_error"}};
        std::string j_string = j.dump();
        return j_string;
        //return type_request;
    }
}
void HandleRequest::deliver(const message& msg)
{
    bool write_in_progress = !write_msgs_.empty();
    //aggiungo un nuovo elemento alla fine della coda
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
        do_write();
    }
}

std::string HandleRequest::generateRandomString(int length) {
    std::string chars(
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "1234567890"
            "!@#$%^&*()"
            "`~-_=+[{]{\\|;:'\",<.>/?");
    std::string salt;
    boost::random::random_device rng;
    boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
    for(int i = 0; i < length; ++i) {
        salt.append(1,chars[index_dist(rng)]);
    }
    //std::cout << std::endl;
    //return std::__cxx11::string();
    return salt;
}
