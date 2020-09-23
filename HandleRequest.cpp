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

#define maxBuffer 5000

HandleRequest::HandleRequest(tcp::socket socket) : socket(std::move(socket)) {

}

void HandleRequest::start(int participantId) {
    //dai un id al partecipante
    shared_from_this()->setSiteId(participantId);
    Server::getInstance().join(shared_from_this());
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
                                    Server::getInstance().leave(shared_from_this());
                                }
                            });
}

void HandleRequest::do_read_body() {
    auto self(shared_from_this());
    boost::asio::async_read(socket, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec) {
                                    std::cout << Connection::getTime() << "REQUEST FROM CLIENT " << this->getId()
                                              << " (" << this->getUsername() << "): " << read_msg_.body() << std::endl;
                                    json messageFromClient;
                                    try {
                                        std::string message = read_msg_.body();
                                        if (message.find_first_of('}') < (message.size() + 1))
                                            message[message.find_first_of('}') + 1] = '\0';
                                        messageFromClient = json::parse(message);
                                    } catch (...) {
                                        sendAtClient(json{{"response", "json_parse_error"}}.dump());
                                        do_read_header();
                                    }
                                    std::string requestType = messageFromClient.at("operation").get<std::string>();
                                    std::string response;
                                    try {
                                        response = handleRequestType(messageFromClient, requestType);
                                    } catch (...) {
                                        std::cout << "handleRequest ERROR: " << errno << std::endl;
                                        sendAtClient(json{{"response", "handleRequest ERROR"}}.dump());
                                    }

                                    do_read_header();
                                } else {
                                    Server::getInstance().leave(shared_from_this());
                                }
                            });
}

void HandleRequest::sendAtClient(const std::string &j_string) {
    std::size_t len = j_string.size();
    message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), j_string.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    std::cout << Connection::getTime() << "RESPONSE TO CLIENT  " << this->getId() << " (" << this->getUsername()
              << "): " << msg.body() << std::endl;
    shared_from_this()->deliver(msg);

}

void HandleRequest::sendAllClient(const std::string &j_string, const int &id) {
    std::size_t len = j_string.size();
    message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), j_string.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    std::cout << Connection::getTime() << "RESPONSE TO ALL CLIENTS ON FILE " << this->getCurrentFile() << ": "
              << msg.body()
              << std::endl;
    Server::getInstance().deliverToAllOnFile(this->getCurrentFile(), msg, id);
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

std::string HandleRequest::handleRequestType(const json &js, const std::string &type_request) {
    if (type_request == "request_login") {
        //prendi username e psw
        std::string username, password;
        username = js.at("username").get<std::string>();
        password = js.at("password").get<std::string>();
        QString colorParticiapant = "#00ffffff";
        std::string resDB = manDB.handleLogin(username, password, colorParticiapant);
        //al log in voglio avere lista di tutti i file nel db con relativo autore
        std::multimap<std::string, std::string> risultato;
        std::list<std::string> fileWithUser;
        if (resDB == "LOGIN_SUCCESS") {
            shared_from_this()->setUsername(username);
            shared_from_this()->setColor(colorParticiapant.toStdString());
            //recupero tutti i file dell'utente
            risultato = manDB.takeFiles(username);
            std::string string;
            std::string underscore = "_";
            for (const auto &p : risultato) {
                string = p.first + underscore + p.second;
                fileWithUser.push_back(string);
            }
            fileWithUser.sort();
        }
        json j = json{{"response",  resDB},
                      {"username",  username},
                      {"colorUser", colorParticiapant.toStdString()},
                      {"files",     fileWithUser}};
        sendAtClient(j.dump());
        return j.dump();
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
            std::cout << Connection::getTime() << "CLIENT " << this->getId() << " (" << username
                      << ") SIGNUP SUCCESS " << std::endl;
            std::string path = boost::filesystem::current_path().string();
            path = path.substr(0, path.find_last_of('/')); //esce da cartella cmake
            path += "/files/" + js.at("username").get<std::string>();
            boost::filesystem::create_directory(path);
            shared_from_this()->setUsername(username); //TODO: cos'e'??
        }
        json j = json{{"response",  resDB},
                      {"username",  username},
                      {"colorUser", colorParticiapant.toStdString()}};
        sendAtClient(j.dump());
        return j.dump();
    } else if (type_request == "R_LOGOUT") {
        json j = json{{"response", "logout"}};
        sendAtClient(j.dump());
        return j.dump();
    } else if (type_request == "CLOSE_FILE") {
        if (Server::getInstance().removeParticipantInFile(this->getCurrentFile(),
                                                          shared_from_this()->getId())) { //era l'ultimo sul file
            std::cout << Connection::getTime() << "CLIENT " << shared_from_this()->getId() << " ("
                      << this->getUsername() << "), CLOSE AND FREE " << this->getCurrentFile() << std::endl;
        } else {
//ci sono altri sul file, li informo della mia uscita
            std::vector<participant_ptr> participantsOnFile = Server::getInstance().getParticipantsInFile(
                    this->getCurrentFile());
            std::vector<int> participantsOnFileId;
            for (const auto &participant : participantsOnFile) {
                participantsOnFileId.push_back(participant->getId());
            }
            json j = json{{"response", "update_participants"},
                          {"idList",   participantsOnFileId}};
            sendAllClient(j.dump(), shared_from_this()->getId());
            std::cout << Connection::getTime() << "CLIENT " << shared_from_this()->getId() << " ("
                      << this->getUsername() << ") CLOSE FILE:" << std::endl;
        }
        json j = json{{"response", "logout"}};
        sendAtClient(j.dump());
        return j.dump();
    } else if (type_request == "insert") {
        std::pair<int, char> message = js.at("corpo").get<std::pair<int, char>>();
        std::string currentFile = this->getCurrentFile();
        MessageSymbol messS = Server::getInstance().insertSymbol(currentFile, message.first, message.second,
                                                                 shared_from_this()->getId());
        std::pair<int, char> corpo(messS.getNewIndex(), message.second);
        //salvataggio su file
        Server::getInstance().modFile(shared_from_this()->getCurrentFile(), false);

        json j = json{{"response", "insert_res"},
                      {"corpo",    corpo}};
        sendAllClient(j.dump(), shared_from_this()->getId());
        return j.dump();

    } else if (type_request == "remove") {
        int startIndex = js.at("start").get<int>();
        int endIndex = js.at("end").get<int>();
        std::string currentFile = this->getCurrentFile();
        MessageSymbol messS = Server::getInstance().eraseSymbol(currentFile, startIndex, endIndex,
                                                                shared_from_this()->getId());

        //salvataggio su file
        Server::getInstance().modFile(shared_from_this()->getCurrentFile(), false);

        json j = json{{"response", "remove_res"},
                      {"start",    startIndex},
                      {"end",      endIndex}};
        sendAllClient(j.dump(), shared_from_this()->getId());
        return j.dump();
    } else if (type_request == "request_new_file") {

        std::string path = boost::filesystem::current_path().string();
        path = path.substr(0, path.find_last_of('/')); //esce da cartella cmake
        path += "/files/" + js.at("username").get<std::string>();
        boost::filesystem::path personalDir(path);
        if (!boost::filesystem::exists(personalDir)) { //anche se gia' fatto in signup
            //creazione cartella personale
            boost::filesystem::create_directory(personalDir);
            std::cout << "Cartella personale " << js.at("username") << " creata" << std::endl;
        }
        std::string nomeFile = path + "/" + js.at("name").get<std::string>() + ".txt";

        if (!boost::filesystem::exists(nomeFile)) {
            //creo il file
            std::ofstream newFile;
            newFile.open(nomeFile, std::ofstream::out);
            newFile.close();
            Server::getInstance().openFile(nomeFile, shared_from_this());
        } else { //il file esiste gia'
            json j = json{{"response", "new_file_already_exist"}};
            sendAtClient(j.dump());
            return j.dump();
        }

        //metto il file nel db con lo user
        std::string resDB = manDB.handleNewFile(js.at("username").get<std::string>(),
                                                js.at("name").get<std::string>());
        if (resDB == "FILE_INSERT_SUCCESS") {
            std::cout << Connection::getTime() << "NEW FILE CREATED FOR CLIENT " << this->getId() << " ("
                      << this->getUsername() << "): " << nomeFile << std::endl;

            // settare il current file
            this->setCurrentFile(nomeFile);

            json j = json{{"response", "new_file_created"}};
            sendAtClient(j.dump());
            //mando lista participant su quel file (solo p=lui per ora)
            std::vector<int> participantsOnFileId(shared_from_this()->getId());
            j = json{{"response", "update_participants"},
                     {"idList",   participantsOnFileId}};
            sendAtClient(j.dump());
            return j.dump();
        } else {
            json j = json{{"response", "errore_salvataggio_file_db"}};
            return j.dump();
        }
    } else if (type_request == "open_file") {

        std::string resDB = manDB.handleOpenFile(js.at("username").get<std::string>(),
                                                 js.at("name").get<std::string>());
        if (/*resDB == "FILE_OPEN_SUCCESS"*/ true) { //TODO: da problemi nella seconda apertura file
            std::string path = boost::filesystem::current_path().string();
            path = path.substr(0, path.find_last_of('/')); //esce da cartella cmake
            path += "/files/" + js.at("username").get<std::string>();
            std::string nomeFile = path + "/" + js.at("name").get<std::string>() + ".txt";
            this->setCurrentFile(nomeFile);

            if (Server::getInstance().isFileInFileSymbols(nomeFile)) { //il file era gia' stato aperto (e' nella mappa)

                //aggiungo participant a lista participants del file
                Server::getInstance().insertParticipantInFile(nomeFile, shared_from_this());

                std::vector<Symbol> symbols = Server::getInstance().getSymbolsPerFile(nomeFile);
                int dim = symbols.size();
                std::cout << Connection::getTime() << "CLIENT " << this->getId() << " ("
                          << this->getUsername() << ") OPEN (NOT NEW) FILE (" << dim << " char): " << nomeFile
                          << std::endl;
                int of = dim / maxBuffer;
                int i = 0;
                std::string toWrite;
                while ((i + 1) * maxBuffer < dim) {
                    toWrite.clear();
                    for (int k = 0; k < maxBuffer; k++) {
                        toWrite.push_back(symbols[(i * maxBuffer) + k].getCharacter());
                    }
                    json j = json{{"response",      "open_file"},
                                  {"maxBuffer", maxBuffer},
                                  {"partToWrite",   i},
                                  {"ofPartToWrite", of},
                                  {"toWrite",       toWrite}};
                    sendAtClient(j.dump());
                    i++;
                }
                toWrite.clear();
                for (int k = 0; k < (dim % maxBuffer); k++) {
                    toWrite.push_back(symbols[(of * maxBuffer) + k].getCharacter());
                }
                json j = json{{"response",      "open_file"},
                              {"maxBuffer", maxBuffer},
                              {"partToWrite",   of},
                              {"ofPartToWrite", of},
                              {"toWrite",       std::string(toWrite)}};
                sendAtClient(j.dump());
            } else { //prima volta che il file viene aperto (lettura da file)
                Server::getInstance().openFile(nomeFile, shared_from_this());
                std::ifstream file;
                file.open(nomeFile);
                std::ifstream in(nomeFile, std::ifstream::ate | std::ifstream::binary);
                int dim = in.tellg();
                std::cout << Connection::getTime() << "CLIENT " << this->getId() << " ("
                          << this->getUsername() << ") OPEN (NEW) FILE (" << dim << " char): " << nomeFile << std::endl;
                char toWrite[maxBuffer];
                int of = dim / maxBuffer;
                int i = 0;
                while ((i + 1) * maxBuffer < dim) {
                    file.read(toWrite, maxBuffer);
                    std::string toWriteString = toWrite;
                    json j = json{{"response",      "open_file"},
                                  {"maxBuffer", maxBuffer},
                                  {"partToWrite",   i},
                                  {"ofPartToWrite", of},
                                  {"toWrite",       toWriteString}};
                    // scrive Symbol nel file nella mappa della room
                    for (int k = 0; k < toWriteString.length(); k++) {
                        MessageSymbol messageInsertedSymbol = Server::getInstance().insertSymbol(nomeFile,
                                                                                                 (i * maxBuffer) + k,
                                                                                                 toWriteString[k],
                                                                                                 shared_from_this()->getId());
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
                // scrive Symbol nel file nella mappa della room
                for (int k = 0; k < toWriteString.length(); k++) {
                    MessageSymbol messageInsertedSymbol = Server::getInstance().insertSymbol(nomeFile,
                                                                                             (of * maxBuffer) + k,
                                                                                             toWriteString[k],
                                                                                             shared_from_this()->getId());
                }
                sendAtClient(j.dump());
            }
            //aggiorna su ogni client lista di participant su questo file
            std::vector<participant_ptr> participantsOnFile = Server::getInstance().getParticipantsInFile(nomeFile);
            std::vector<int> participantsOnFileId;
            for (const auto &participant : participantsOnFile) {
                participantsOnFileId.push_back(participant->getId());
            }
            json j = json{{"response", "update_participants"},
                          {"idList",   participantsOnFileId}};
            sendAllClient(j.dump(), shared_from_this()->getId());
            return "";
        } else {
            json j = json{{"response", "errore_apertura_file"}};
            return j.dump();
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
            return j.dump();
        } else {
            json j = json{{"response", "errore_rinomina_file"}};
            return j.dump();
        }


    } else {
        std::cout << "nessun match col tipo di richiesta" << std::endl;
        json j = json{{"response", "general_error"}};
        return j.dump();
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
