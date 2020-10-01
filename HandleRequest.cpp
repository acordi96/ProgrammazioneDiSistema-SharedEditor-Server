//
// Created by Sam on 01/apr/2020.
//

#include <fstream>
#include "Headers/HandleRequest.h"
#include <boost/filesystem.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <QtGui/QImage>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"

#define maxBuffer 5000

HandleRequest::HandleRequest(tcp::socket socket) : socket(std::move(socket)) {}

void HandleRequest::start(int participantId) {
    //dai un id al partecipante
    shared_from_this()->setSiteId(participantId);
    Server::getInstance().join(shared_from_this());
    do_read_header();
}

void HandleRequest::do_read_header() {
    auto self(shared_from_this());
    memset(read_msg_.data(), 0, read_msg_.length());
    boost::asio::async_read(socket, boost::asio::buffer(read_msg_.data(), Message::header_length),
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
                                    std::cout << SocketManager::output()
                                              << "REQUEST FROM CLIENT " << this->getId()
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
    Message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), j_string.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    std::cout << SocketManager::output() << "RESPONSE TO CLIENT " << this->getId() << " ("
              << this->getUsername()
              << "): " << msg.body() << std::endl;
    shared_from_this()->deliver(msg);

}

void HandleRequest::sendAllClient(const std::string &j_string, const int &id) {
    std::size_t len = j_string.size();
    Message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), j_string.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    std::cout << SocketManager::output() << "RESPONSE TO ALL CLIENTS ON FILE "
              << this->getCurrentFile() << ": "
              << msg.body()
              << std::endl;
    Server::getInstance().deliverToAllOnFile(msg, shared_from_this());
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

#ifdef Q_OS_LINUX //linux
    std::string pathFilesystem = boost::filesystem::current_path().string();
    pathFilesystem = pathFilesystem.substr(0, pathFilesystem.find_last_of('/')); //esce da cartella cmake
    pathFilesystem += "/Filesystem";
#else   //winzoz
    std::string pathFilesystem = boost::filesystem::current_path().string();
    pathFilesystem = pathFilesystem.substr(0, pathFilesystem.find_last_of('\\')); //esce da cartella cmake
    pathFilesystem += "\\Filesystem";
#endif

    if (type_request == "request_login") {
        //prendi username e psw
        std::string username, password;
        username = js.at("username").get<std::string>();
        password = js.at("password").get<std::string>();
        QString colorParticiapant = "#00ffffff";
        std::string resDB = ManagementDB::getInstance().handleLogin(username, password, colorParticiapant);
        //al log in voglio avere lista di tutti i file nel db con relativo autore
        std::multimap<std::string, std::string> risultato;
        std::list<std::string> fileWithUser;
        if (resDB == "LOGIN_SUCCESS") {
            shared_from_this()->setUsername(username);
            shared_from_this()->setColor(colorParticiapant.toStdString());
            shared_from_this()->setCurrentFile("");
            //recupero tutti i file dell'utente
            risultato = ManagementDB::getInstance().takeFiles(username);
            std::string string;
            std::string underscore = "_";
            for (const auto &p : risultato) {
                string = p.first + underscore + p.second;
                fileWithUser.push_back(string);
            }
            fileWithUser.sort();
            //send icon if exist //TODO: gestione icona sospesa
            /*std::string path = boost::filesystem::current_path().string();
            path = path.substr(0, path.find_last_of('/')); //esce da cartella cmake
            path += "/Icons/" + js.at("username").get<std::string>() + ".png";
            std::ifstream iconFile(path, std::ios::binary);
            std::string icon;
            if (iconFile.good()) {
                icon = std::string(std::istreambuf_iterator<char>(iconFile), {});
            }*/
            json j = json{{"response",  resDB},
                          {"username",  username},
                          {"colorUser", colorParticiapant.toStdString()},
                          {"files",     fileWithUser}/*,
                          {"icon",      icon}*/};
            sendAtClient(j.dump());
            return j.dump();
        }
        json j = json{{"response", resDB},
                      {"username", username}};
        sendAtClient(j.dump());
        return j.dump();
    } else if (type_request == "request_signup") {
        //prendi username e psw
        std::string username, email, password;
        username = js.at("username").get<std::string>();
        password = js.at("password").get<std::string>();
        email = js.at("email").get<std::string>();
        std::string colorParticiapant = ManagementDB::getInstance().handleSignup(email, username, password);

        //se la registrazione va bene, creo la cartella personale per il nuovo utente
        if (colorParticiapant != "SIGNUP_ERROR_INSERT_FAILED" && colorParticiapant != "CONNESSION_ERROR") {
            std::cout << SocketManager::output() << "CLIENT " << this->getId() << " ("
                      << username
                      << ") SIGNUP SUCCESS, COLOR GENERATED: " << colorParticiapant << std::endl;

#ifdef Q_OS_LINUX //linux
            boost::filesystem::create_directory(pathFilesystem + "/" + username);
#else //winzoz
            boost::filesystem::create_directory(pathFilesystem + "\\" + username);
#endif
            shared_from_this()->setUsername(username);
            json j = json{{"response",  "SIGNUP_SUCCESS"},
                          {"username",  username},
                          {"colorUser", colorParticiapant}};
            sendAtClient(j.dump());
            return j.dump();
        } else {
            json j = json{{"response", colorParticiapant},
                          {"username", username},};
            sendAtClient(j.dump());
            return j.dump();
        };
    } else if (type_request == "req_logout") {
        if (!shared_from_this()->getCurrentFile().empty()) {
            std::vector<int> othersOnFile = Server::getInstance().closeFile(shared_from_this());
            if (othersOnFile.empty()) { //nessun altro sul file
                std::cout << SocketManager::output() << "CLIENT "
                          << shared_from_this()->getId()
                          << " ("
                          << shared_from_this()->getUsername() << ") LOGOUT AND FREE FILE: "
                          << shared_from_this()->getCurrentFile() << std::endl;
            } else { //altri sul file
                std::vector<std::string> colors = Server::getInstance().getColors(othersOnFile);
                std::vector<std::string> usernames = Server::getInstance().getUsernames(othersOnFile);
                json j = json{{"response",   "update_participants"},
                              {"idList",     othersOnFile},
                              {"usernames",  usernames},
                              {"colorsList", colors},
                              {"usernames",  usernames}};
                sendAtClient(j.dump());
                sendAllClient(j.dump(), shared_from_this()->getId());
                std::cout << SocketManager::output() << "CLIENT "
                          << shared_from_this()->getId()
                          << " ("
                          << shared_from_this()->getUsername() << ") LOGOUT, STILL OPENED FILE: "
                          << shared_from_this()->getCurrentFile() << std::endl;
            }
        }
        //rimuovi partipipant
        Server::getInstance().removeParticipant(shared_from_this());
        json j = json{{"response", "logout"}};
        sendAtClient(j.dump());
        return j.dump();
    } else if (type_request == "close_file") {
        std::vector<int> othersOnFile = Server::getInstance().closeFile(shared_from_this());
        if (othersOnFile.empty()) { //nessun altro sul file
            std::cout << SocketManager::output() << "CLIENT " << shared_from_this()->getId()
                      << " ("
                      << shared_from_this()->getUsername() << ") CLOSE AND FREE FILE: "
                      << shared_from_this()->getCurrentFile() << std::endl;
        } else { //altri sul file
            std::vector<std::string> colors = Server::getInstance().getColors(othersOnFile);
            std::vector<std::string> usernames = Server::getInstance().getUsernames(othersOnFile);
            json j = json{{"response",   "update_participants"},
                          {"usernames",  usernames},
                          {"idList",     othersOnFile},
                          {"colorsList", colors}};
            sendAtClient(j.dump());
            sendAllClient(j.dump(), shared_from_this()->getId());
            std::cout << SocketManager::output() << "CLIENT " << shared_from_this()->getId()
                      << " ("
                      << shared_from_this()->getUsername() << ") CLOSE FILE: " << shared_from_this()->getCurrentFile()
                      << std::endl;
        }
        shared_from_this()->setCurrentFile("");
        json j = json{{"response", "file_closed"}};
        sendAtClient(j.dump());
        return j.dump();
    } else if (type_request == "insert") {
        std::pair<int, char> message = js.at("corpo").get<std::pair<int, char >>();
        std::string currentFile = this->getCurrentFile();
        MessageSymbol messS = Server::getInstance().insertSymbol(currentFile, message.first, message.second,
                                                                 shared_from_this()->getId());
        std::pair<int, char> corpo(messS.getNewIndex(), message.second);
        //salvataggio su file
        Server::getInstance().modFile(shared_from_this()->getCurrentFile(), false);

        json j = json{{"response",    "insert_res"},
                      {"participant", shared_from_this()->getId()},
                      {"corpo",       corpo}};
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

        json j = json{{"response",    "remove_res"},
                      {"participant", shared_from_this()->getId()},
                      {"start",       startIndex},
                      {"end",         endIndex}};
        sendAllClient(j.dump(), shared_from_this()->getId());
        return j.dump();
    } else if (type_request == "request_new_file") {

        if (js.at("name").empty()) {
            json j = json{{"response", "invalid_name"}};
            sendAtClient(j.dump());
            return j.dump();
        }

#ifdef Q_OS_LINUX //linux
        boost::filesystem::path personalDir(pathFilesystem + "/" + js.at("username").get<std::string>());
#else //winzoz
        boost::filesystem::path personalDir(pathFilesystem + "\\" + js.at("username").get<std::string>());
#endif
        if (!boost::filesystem::exists(personalDir)) { //anche se gia' fatto in signup
            //creazione cartella personale
            boost::filesystem::create_directory(personalDir);
            std::cout << "Cartella personale " << js.at("username") << " creata" << std::endl;
        }
#ifdef Q_OS_LINUX //linux
        std::string filename = pathFilesystem + "/" + js.at("username").get<std::string>() + "/" + js.at("name").get<std::string>() + ".txt";
#else //winzoz
        std::string filename = pathFilesystem + "\\" + js.at("username").get<std::string>() + "\\" + js.at("name").get<std::string>() + ".txt";
#endif

        if (!boost::filesystem::exists(filename)) { //creo finicamente il file
            std::ofstream newFile;
            newFile.open(filename, std::ofstream::out);
            newFile.close();
        } else { //il file esiste gia'
            json j = json{{"response", "new_file_already_exist"}};
            sendAtClient(j.dump());
            return j.dump();
        }

        // settare il current file
        this->setCurrentFile(filename);

        //metto il file nel db con lo user
        std::string resDB = ManagementDB::getInstance().handleNewFile(js.at("username").get<std::string>(),
                                                                      js.at("name").get<std::string>());
        if (resDB == "FILE_INSERT_SUCCESS") {
            std::cout << SocketManager::output() << "NEW FILE CREATED FOR CLIENT "
                      << this->getId() << " ("
                      << this->getUsername() << "): " << filename << std::endl;

            //stanzio le strutture
            Server::getInstance().openFile(shared_from_this());

            json j = json{{"response", "new_file_created"}};
            sendAtClient(j.dump());
            //mando lista participant sul file (solo lui per ora)
            std::vector<int> participantsOnFileId(shared_from_this()->getId());
            std::vector<std::string> colors = Server::getInstance().getColors(participantsOnFileId);
            std::vector<std::string> usernames = Server::getInstance().getUsernames(participantsOnFileId);
            j = json{{"response",   "update_participants"},
                     {"idList",     participantsOnFileId},
                     {"usernames",  usernames},
                     {"colorsList", colors}};
            sendAtClient(j.dump());
            sendAtClient(j.dump());
            return j.dump();
        } else {
            json j = json{{"response", "errore_salvataggio_file_db"}};
            return j.dump();
        }
    } else if (type_request == "open_file") {

        std::string resDB = ManagementDB::getInstance().handleOpenFile(js.at("username").get<std::string>(),
                                                                       shared_from_this()->getUsername(),
                                                                       js.at("name").get<std::string>());
        if (resDB == "FILE_OPEN_SUCCESS") {
#ifdef Q_OS_LINUX //linux
            std::string filename = pathFilesystem + "/" + js.at("username").get<std::string>() + "/" + js.at("name").get<std::string>() + ".txt";
#else //winzoz
            std::string filename = pathFilesystem + "\\" + js.at("username").get<std::string>() + "\\" + js.at("name").get<std::string>() + ".txt";
#endif
            this->setCurrentFile(filename);

            if (Server::getInstance().isFileInFileSymbols(filename)) { //il file era gia' stato aperto (e' nella mappa)

                //aggiungo participant a lista participants del file
                Server::getInstance().insertParticipantInFile(shared_from_this());

                std::vector<Symbol> symbols = Server::getInstance().getSymbolsPerFile(filename);
                int dim = symbols.size();
                std::cout << SocketManager::output() << "CLIENT " << this->getId() << " ("
                          << this->getUsername() << ") OPEN (NOT NEW) FILE (" << dim << " char): " << filename
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
                Server::getInstance().openFile(shared_from_this());
                std::ifstream file;
                file.open(filename);
                std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
                int dim = in.tellg();
                std::cout << SocketManager::output() << "CLIENT " << this->getId() << " ("
                          << this->getUsername() << ") OPEN (NEW) FILE (" << dim << " char): " << filename << std::endl;
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
                        MessageSymbol messageInsertedSymbol = Server::getInstance().insertSymbol(filename,
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
                    MessageSymbol messageInsertedSymbol = Server::getInstance().insertSymbol(filename,
                                                                                             (of * maxBuffer) + k,
                                                                                             toWriteString[k],
                                                                                             shared_from_this()->getId());
                }
                sendAtClient(j.dump());
            }
            //aggiorna su ogni client lista di participant su questo file
            std::vector<participant_ptr> participantsOnFile = Server::getInstance().getParticipantsInFile(filename);
            std::vector<int> participantsOnFileId;
            for (const auto &participant : participantsOnFile) {
                participantsOnFileId.push_back(participant->getId());
            }
            std::vector<std::string> colors = Server::getInstance().getColors(participantsOnFileId);
            std::vector<std::string> usernames = Server::getInstance().getUsernames(participantsOnFileId);
            json j = json{{"response",   "update_participants"},
                          {"idList",     participantsOnFileId},
                          {"usernames",  usernames},
                          {"colorsList", colors}};
            sendAtClient(j.dump());
            sendAllClient(j.dump(), shared_from_this()->getId());
            return j.dump();
        } else {
            json j = json{{"response", "errore_apertura_file"}};
            sendAtClient(j.dump());
            return j.dump();
        }
    } else if (type_request == "get_invitation") {
        /*json{     {"operation", "get_invitation"},
                    {"filename", filename};*/
        std::string invitationCode = ManagementDB::getInstance().getInvitation(shared_from_this()->getUsername(),
                                                                               js.at("filename"));
        if (invitationCode != "QUERY_REFUSED" && invitationCode != "CONNESSION_ERROR_") {
            json j = json{{"response",        "get_invitation_success"},
                          {"invitation_code", invitationCode}};
            sendAtClient(j.dump());
            return j.dump();
        } else {
            json j = json{{"response", "error_get_invitation"}};
            sendAtClient(j.dump());
            return j.dump();
        }
    } else if (type_request == "validate_invitation") {
        std::pair<std::string, std::string> resp = ManagementDB::getInstance().validateInvitation(
                shared_from_this()->getUsername(),
                js.at("invitation_code"));
        if (resp.first != "REFUSED") {
            json j = json{{"response", "invitation_success"},
                          {"owner",    resp.first},
                          {"filename", resp.second}
            };
            sendAtClient(j.dump());
            return j.dump(); //TODO: client refresh file list
        } else {
            json j = json{{"response", "invitation_error"}};
            sendAtClient(j.dump());
            return j.dump();
        }
    } else if (type_request == "update_icon") {
        /*std::string path = boost::filesystem::current_path().string();
        path = path.substr(0, path.find_last_of('/')); //esce da cartella cmake
        path += "/Icons/" + js.at("username").get<std::string>() + ".png";
        std::string imageString = js.at("icon");
        std::ofstream file;
        file.open(path);
        file << imageString;
        file.close();
        json j = json{{"response", "icon-updated-successfully"}};
        sendAtClient(j.dump());
        return j.dump();*/
    } else if (type_request == "request_new_name") {

        //vado a cambiare nome file nel DB
        //bisognerebbe anche gestire la concorrenza, altri utenti potrebbero aver il file
        //già aperto, in questo caso non è possibile cambiare il nome
        std::cout << "\n entrato in request_new_name \n";

#ifdef Q_OS_LINUX //linux
        std::string new_path = pathFilesystem + "/" + js.at("username").get<std::string>() + "/" + js.at("new_name").get<std::string>() + ".txt";
        std::string old_path = pathFilesystem + "/" + js.at("username").get<std::string>() + "/" + js.at("old_name").get<std::string>() + ".txt";
#else //winzoz
        std::string new_path = pathFilesystem + "\\" + js.at("username").get<std::string>() + "\\" + js.at("new_name").get<std::string>() + ".txt";
        std::string old_path = pathFilesystem + "\\" + js.at("username").get<std::string>() + "\\" + js.at("old_name").get<std::string>() + ".txt";
#endif

        //controllo se il file è in uso da qualcuno
        if (Server::getInstance().isFileInFileSymbols(old_path)) {
            //file aperto da qualche utente, impossibile rinominare
            json j = json{{"response", "FILE_IN_USE"}};
            sendAtClient(j.dump());
            return j.dump();
        }

        if (boost::filesystem::exists(new_path)) {

            //il nome file esiste gia'
            json j = json{{"response", "NEW_NAME_ALREADY_EXIST"}};
            sendAtClient(j.dump());
            return j.dump();
        }

        std::string resDB = ManagementDB::getInstance().handleRenameFile(js.at("username").get<std::string>(),
                                                                         js.at("old_name").get<std::string>(),
                                                                         js.at("new_name").get<std::string>());
        std::cout << "\n res db per rename = " << resDB;
        if (resDB == "FILE_RENAME_SUCCESS") {
            std::cout << "\nFile rinominato correttamente" << std::endl;


            boost::filesystem::rename(old_path, new_path);


            json j = json{{"response", "FILE_RENAMED"},
                          {"newName",  js.at("new_name").get<std::string>()},
                          {"oldName",  js.at("old_name").get<std::string>()}};
            sendAtClient(j.dump());
            return j.dump();
        } else {
            json j = json{{"response", "ERRORE_RINOMINA_FILE"}};
            sendAtClient(j.dump());
            return j.dump();
        }


    } else if (type_request == "delete_file") {
#ifdef Q_OS_LINUX //linux
        std::string filename = pathFilesystem + "/" + js.at("username").get<std::string>() + "/" + js.at("name").get<std::string>() + ".txt";
#else //winzoz
        std::string filename = pathFilesystem + "\\" + js.at("username").get<std::string>() + "\\" + js.at("name").get<std::string>() + ".txt";
#endif

        if (Server::getInstance().isFileInFileSymbols(filename)) {
            //file aperto da qualche utente, impossibile rinominare
            json j = json{{"response", "FILE_IN_USE_D"}};
            sendAtClient(j.dump());
            return j.dump();
        }

        std::string resDB = ManagementDB::getInstance().handleDeleteFile(js.at("username").get<std::string>(),
                                                                         js.at("name").get<std::string>());
        if (resDB == "FILE_DELETE_SUCCESS") {
            boost::filesystem::remove(filename);
            json j = json{{"response", "FILE_DELETED"},
                          {"name",     js.at("name").get<std::string>()},
                          {"username", js.at("username").get<std::string>()}};
            sendAtClient(j.dump());
            return j.dump();
        } else {
            json j = json{{"response", "ERRORE_ELIMINAZIONE_FILE"}};
            sendAtClient(j.dump());
            return j.dump();
        }

    } else {
        std::cout << "nessun match col tipo di richiesta" << std::endl;
        json j = json{{"response", "general_error"}};
        return j.dump();
    }
}

void HandleRequest::deliver(const Message &msg) {
    bool write_in_progress = !write_msgs_.empty();
    //aggiungo un nuovo elemento alla fine della coda
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
        do_write();
    }
}
