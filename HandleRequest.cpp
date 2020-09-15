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

#define maxBuffer 3000

HandleRequest::HandleRequest(tcp::socket socket, Room &room) : socket(std::move(socket)), room_(room) {

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
    boost::asio::async_read(socket, boost::asio::buffer(read_msg_.data(), message::header_length),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec && read_msg_.decode_header()) {
                                    do_read_body();
                                } else {
                                    room_.leave(shared_from_this());
                                }
                            });
}

void HandleRequest::do_read_body() {
    auto self(shared_from_this());
    boost::asio::async_read(socket, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec) {
                                    std::cout << Server::getTime() << "REQUEST from client " << this->getId() - 1
                                              << " (" << this->getUsername() << "): " << read_msg_.body() << std::endl;
                                    json messageFromClient;
                                    try {
                                        std::string readBody = read_msg_.body();
                                        readBody.erase(readBody.find('}') + 1);
                                        messageFromClient = json::parse(readBody);
                                    } catch (...) {
                                        std::cout << "Json parse exception, ignoring request: " << read_msg_.body()
                                                  << std::endl;
                                        sendAtClient(json{{"response", "json_parse_error"}}.dump());
                                        do_read_header();
                                        return;
                                    }
                                    std::string requestType = messageFromClient.at("operation").get<std::string>();
                                    int partecipantId = shared_from_this()->getId();
                                    //nella gestione devo tenere traccia del partecipante che invia la richiesta, quindi a chi devo rispondere e a chi no
                                    std::string response = handleRequestType(messageFromClient, requestType,
                                                                             partecipantId);
                                    if (requestType == "insert") {
                                        sendAllClient(response, partecipantId);
                                    } else if (requestType == "remove") {
                                        sendAllClient(response, partecipantId);
                                    } else if (requestType == "request_login" || requestType == "request_signup" ||
                                               requestType == "request_new_file") {
                                        sendAtClient(response);
                                    }
                                    //l'apertura file viene spedita direttamente man mano che legge per ottimizzare
                                    do_read_header();
                                } else {
                                    room_.leave(shared_from_this());
                                }
                            });
}

void HandleRequest::sendAtClient(std::string j_string) {
    std::size_t len = j_string.size();
    message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), j_string.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    std::cout << Server::getTime() << "RESPONSE to client  " << this->getId() - 1 << " (" << this->getUsername()
              << "): " << msg.body() << std::endl;
    shared_from_this()->deliver(msg);

}

void HandleRequest::sendAllClient(std::string j_string, const int &id) {
    std::size_t len = j_string.size();
    message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), j_string.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    std::cout << Server::getTime() << "WRITE to all clients on FILE " << this->getCurrentFile() << ": " << msg.body()
              << std::endl;
    room_.deliverToAll(msg, id);
}

void HandleRequest::do_write() {
    //scorro la coda dal primo elemento inserito
    boost::asio::async_write(socket, boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
                             [this](boost::system::error_code ec, std::size_t /*length*/) {
                                 if (!ec) {
                                     //rimuove il primo elemento dato che lo ha inviato
                                     write_msgs_.pop_front();
                                     //controlla se ce ne sono altri, se si scrivi ancora
                                     if (!write_msgs_.empty()) {
                                         do_write();
                                     }
                                 } else {
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
        //al log in voglio avere lista di tutti i file nel db con relativo autore
        std::multimap<std::string, std::string> risultato;
        //std::list<std::string> risultato;
        std::list<std::string> fileWithUser;
        if (resDB == "LOGIN_SUCCESS") {
            shared_from_this()->setUsername(username);
            shared_from_this()->setColor(colorParticiapant.toStdString());
            //recupero tutti i file dell'utente
            risultato = manDB.takeFiles(username);
            std::string string;
            std::string underscore = "_";
            for (auto p : risultato) {
                string = p.first + underscore + p.second;
                fileWithUser.push_back(string);
            }
            fileWithUser.sort();
        }
        json j = json{{"response",  resDB},
                      {"username",  username},
                      {"colorUser", colorParticiapant.toStdString()},
                      {"files",     fileWithUser}};
        std::string j_string = j.dump();
        return j_string;
    } else if (type_request == "request_signup") {
        //prendi username e psw
        std::string username, email, password;
        username = js.at("username").get<std::string>();
        password = js.at("password").get<std::string>();
        email = js.at("email").get<std::string>();
        QString colorParticiapant = "#00ffffff";
        std::string resDB = manDB.handleSignup(email, username, password);

        //se la registrazione va bene, creo la cartella personale per il nuovo utente
        if (resDB == "SIGNUP_SUCCESS") {
            std::cout << Server::getTime() << "SIGNUP SUCCESS client " << this->getId() << ": " << username
                      << std::endl;
            std::string path = boost::filesystem::current_path().string();
            path = path.substr(0, path.find_last_of("/")); //esce da cartella cmake
            path += "/files/" + js.at("username").get<std::string>();
            boost::filesystem::create_directory(path);
            shared_from_this()->setUsername(username); //TODO: cos'e'??
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
        json j = json{{"response", "insert_res"},
                      {"corpo",    corpo}};
        std::string j_string = j.dump();
        //salvataggio su file
        std::string currentFile = this->getCurrentFile();
        std::fstream file;
        file.open(currentFile);
        file.seekp(message.first);
        file << message.second;
        file.close();
        //TODO: STRUTTURA DATI CONDIVISA NON FUNZIONA
        /*if(this->room_.files.at(currentFile)->is_open()) {
            std::cout<<"TENTO DI SCRIVERE"<<std::endl;
            this->room_.files.at(currentFile)->seekp(message.first);
            *this->room_.files.at(currentFile)<<message.second;
            std::cout<<"SCRITTO!!!"<<std::endl;
        } else {
            std::cout<<"errore, file non aperto"<<std::endl;
        }*/

        return j_string;

    } else if (type_request == "remove") {
        int startIndex = js.at("start").get<int>();
        int endIndex = js.at("end").get<int>();
        MessageSymbol messS = localErase(startIndex, endIndex);
        room_.send(messS);
        room_.dispatchMessages();
        json j = json{{"response", "remove_res"},
                      {"start",    startIndex},
                      {"end",      endIndex}};
        std::string j_string = j.dump();
        //salvataggio su file
        std::string currentFile = this->getCurrentFile();
        std::fstream file;
        file.open(currentFile);
        file.seekg(endIndex);
        char c;
        std::string text;
        file.seekg(0);
        for (int i = 0; i < startIndex; i++) {
            file.get(c);
            text += c;
        }
        file.seekg(endIndex);
        while (file.get(c)) {
            if (c == EOF)
                break;
            text += c;
        }
        file.close();
        std::ofstream fileRiscritto;
        fileRiscritto.open(currentFile);
        fileRiscritto << text;
        fileRiscritto.close();
        return j_string;
    } else if (type_request == "request_new_file") {

        std::string path = boost::filesystem::current_path().string();
        path = path.substr(0, path.find_last_of("/")); //esce da cartella cmake
        path += "/files/" + js.at("username").get<std::string>();
        boost::filesystem::path personalDir(path);
        if (!boost::filesystem::exists(personalDir)) { //anche se gia' fatto in signup
            //creazione cartella personale
            boost::filesystem::create_directory(personalDir);
            std::cout << "Cartella personale " << js.at("username") << " creata" << std::endl;
        }
        std::string nomeFile = path + "/" + js.at("name").get<std::string>() + ".txt";
        // creo il file nuovo
        if (boost::filesystem::exists(nomeFile)) {
            std::cout << "File gia' esistente" << std::endl;
            json j = json{{"response", "new_file_already_exist"}};
            std::string j_string = j.dump();
            return j_string;
        }
        std::ofstream newFile;
        newFile.open(nomeFile, std::ofstream::out);
        newFile.close();
        //this->room_.files.insert(std::pair<std::string, std::ofstream *>(nomeFile, &newFile));

        //metto il file nel db con lo user
        std::string resDB = manDB.handleNewFile(js.at("username").get<std::string>(),
                                                js.at("name").get<std::string>());
        if (resDB == "FILE_INSERT_SUCCESS") {
            std::cout << Server::getTime() << "NEW FILE CREATED for client " << this->getId() << " ("
                      << this->getUsername() << "): " << nomeFile << std::endl;

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
    } else if (type_request == "open_file") {

        std::string resDB = manDB.handleOpenFile(js.at("username").get<std::string>(),
                                                 js.at("name").get<std::string>());
        if (resDB == "FILE_OPEN_SUCCESS") {
            std::string path = boost::filesystem::current_path().string();
            path = path.substr(0, path.find_last_of("/")); //esce da cartella cmake
            path += "/files/" + js.at("username").get<std::string>();
            std::string nomeFile = path + "/" + js.at("name").get<std::string>() + ".txt";
            std::ifstream file;
            file.open(nomeFile);
            std::ifstream in(nomeFile, std::ifstream::ate | std::ifstream::binary);
            int dim = in.tellg();
            std::cout << Server::getTime() << "OPEN FILE (" << dim << " char) for client " << this->getId() - 1 << " ("
                      << this->getUsername() << "): " << nomeFile << std::endl;
            this->setCurrentFile(nomeFile);
            char toWrite[maxBuffer + 1];
            int of = dim / maxBuffer;
            int i = 0;
            while ((i + 1) * maxBuffer < dim) {
                file.read(toWrite, maxBuffer);
                std::string toWriteString = toWrite;
                toWriteString.erase(maxBuffer);
                json j = json{{"response",      "open_file"},
                              {"maxBuffer", maxBuffer},
                              {"partToWrite",   i},
                              {"ofPartToWrite", of},
                              {"toWrite",       toWriteString}};
                for (int k = 0; k < toWriteString.length(); k++) {
                    MessageSymbol messS = localInsert(toWriteString[k], k);
                    room_.send(messS);
                    room_.dispatchMessages();
                }
                sendAtClient(j.dump());
                i++;
            }
            char toWrite2[(dim % maxBuffer) + 1];
            file.read(toWrite2, dim % maxBuffer);
            std::string toWriteString = toWrite2;
            toWriteString.erase((dim % maxBuffer));
            json j = json{{"response",      "open_file"},
                          {"maxBuffer", maxBuffer},
                          {"partToWrite",   of},
                          {"ofPartToWrite", of},
                          {"toWrite",       toWriteString}};
            for (int k = 0; k < toWriteString.length(); k++) {
                MessageSymbol messS = localInsert(toWriteString[k], k);
                room_.send(messS);
                room_.dispatchMessages();
            }
            sendAtClient(j.dump());
            return "";
        } else {
            json j = json{{"response", "errore_apertura_file"}};
            std::string j_string = j.dump();
            return j_string;
        }
    } else if (type_request == "request_new_name") {
        //vado a cambiare nome file nel DB
        //bisognerebbe anche gestire la concorrenza, altri utenti potrebbero aver il file
        //già aperto, in questo caso o non è possibile cambiare il nome oppure bisogna notificarlo
        //a tutti gli utenti in quel momento attivi
        std::string resDB = manDB.handleRenameFile(js.at("username").get<std::string>(),
                                                   js.at("oldName").get<std::string>(),
                                                   js.at("newName").get<std::string>());
        if (resDB == "FILE_RENAME_SUCCESS") {
            std::cout << "File rinominato correttamente" << std::endl;

            json j = json{{"response", "file_renamed"},
                          {"newName",  js.at("newName").get<std::string>()},
                          {"oldName",  js.at("oldName").get<std::string>()}};
            std::string j_string = j.dump();
            return j_string;
        } else {
            json j = json{{"response", "errore_rinomina_file"}};
            std::string j_string = j.dump();
            return j_string;
        }


    } else {
        std::cout << "nessun match col tipo di richiesta" << std::endl;
        json j = json{{"response", "general_error"}};
        std::string j_string = j.dump();
        return j_string;
    }
}

void HandleRequest::deliver(const message &msg) {
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
    for (int i = 0; i < length; ++i) {
        salt.append(1, chars[index_dist(rng)]);
    }
    //std::cout << std::endl;
    //return std::__cxx11::string();
    return salt;
}
