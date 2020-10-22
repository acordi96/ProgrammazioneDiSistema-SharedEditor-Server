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

#define maxBufferSymbol 150 //numero massimo di symbols mandati contemporaneamente
#define debugOutput false

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
                                    if (Server::getInstance().isParticipantIn(shared_from_this()->getId())) {
                                        handleRequestType(json{{"operation", "req_logout_and_close"},
                                                               {"username",  shared_from_this()->getUsername()}},
                                                          "req_logout_and_close");
                                        Server::getInstance().leave(shared_from_this());
                                    }
                                }
                            });
}

void HandleRequest::do_read_body() {
    auto self(shared_from_this());
    boost::asio::async_read(socket, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec) {
                                    if (debugOutput) {
                                        std::cout << SocketManager::output()
                                                  << "REQUEST FROM CLIENT " << this->getId();
                                        if (shared_from_this()->getUsername() != "")
                                            std::cout << " ("
                                                      << shared_from_this()->getUsername() << ")";
                                        std::cout << ": " << read_msg_.body() << std::endl;
                                    }
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
                                    std::string requestType;
                                    std::string response;
                                    try {
                                        requestType = messageFromClient.at("operation").get<std::string>();
                                        response = handleRequestType(messageFromClient, requestType);
                                    } catch (...) {
                                        std::cout << "handleRequest ERROR: " << errno << std::endl;
                                        sendAtClient(json{{"response", "handleRequest ERROR"}}.dump());
                                    }

                                    do_read_header();
                                } else {
                                    if (Server::getInstance().isParticipantIn(shared_from_this()->getId())) {
                                        handleRequestType(json{{"operation", "req_logout_and_close"},
                                                               {"username",  shared_from_this()->getUsername()}},
                                                          "req_logout_and_close");
                                        Server::getInstance().leave(shared_from_this());
                                    }
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
    if (debugOutput) {
        std::cout << SocketManager::output() << "RESPONSE TO CLIENT " << this->getId();
        if (shared_from_this()->getUsername() != "")
            std::cout << " ("
                      << shared_from_this()->getUsername() << ")";
        std::cout << ": " << msg.body() << std::endl;
    }
    shared_from_this()->deliver(msg);

}

void HandleRequest::sendAllOtherClientsOnFile(const std::string &response) {
    std::size_t len = response.size();
    Message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), response.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    if (debugOutput)
        std::cout << SocketManager::output() << "RESPONSE TO ALL OTHER CLIENTS ON FILE "
                  << this->getCurrentFile() << ": "
                  << msg.body()
                  << std::endl;
    Server::getInstance().deliverToAllOtherOnFile(msg, shared_from_this());
}

void HandleRequest::sendAllClientsOnFile(const std::string &response) {
    std::size_t len = response.size();
    Message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), response.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    if (debugOutput)
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

#ifdef __linux__
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
        std::string username = js.at("username").get<std::string>();
        std::string password = js.at("password").get<std::string>();
        if (Server::getInstance().isParticipantIn(username)) {
            json j = json{{"response", "user_already_logged"},
                          {"username", username}};
            sendAtClient(j.dump());
            return j.dump();
        }
        QString colorParticiapant = "#00ffffff";
        QString email;
        std::string resDB = ManagementDB::getInstance().handleLogin(username, password, colorParticiapant, email);
        //al log in voglio avere lista di tutti i file nel db con relativo autore
        std::multimap<std::pair<std::string, std::string>, std::string> risultato;
        if (resDB == "LOGIN_SUCCESS") {
            shared_from_this()->setUsername(username);
            shared_from_this()->setColor(colorParticiapant.toStdString());
            shared_from_this()->setCurrentFile("");
            //recupero tutti i file dell'utente
            risultato = ManagementDB::getInstance().takeFiles(username);
            std::vector<std::string> owners;
            std::vector<std::string> filenames;
            std::vector<std::string> invitations;
            std::multimap<std::pair<std::string, std::string>, std::string>::iterator iter;
            for (iter = risultato.begin(); iter != risultato.end(); ++iter) {
                owners.emplace_back(iter->first.first);
                filenames.emplace_back(iter->first.second);
                invitations.emplace_back(iter->second);
            }
            //send icon if exist //TODO: gestione icona sospesa
            /*std::string path = boost::filesystem::current_path().string();
            path = path.substr(0, path.find_last_of('/')); //esce da cartella cmake
            path += "/Icons/" + js.at("username").get<std::string>() + ".png";
            std::ifstream iconFile(path, std::ios::binary);
            std::string icon;
            if (iconFile.good()) {
                icon = std::string(std::istreambuf_iterator<char>(iconFile), {});
            }*/
            json j = json{{"response",    resDB},
                          {"maxBufferSymbol", maxBufferSymbol},
                          {"username",    username},
                          {"email",       email.toStdString()},
                          {"colorUser",   colorParticiapant.toStdString()},
                          {"owners",      owners},
                          {"filenames",   filenames},
                          {"invitations", invitations}/*,
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

#ifdef __linux__
            boost::filesystem::create_directory(pathFilesystem + "/" + username);
#else //winzoz
            boost::filesystem::create_directory(pathFilesystem + "\\" + username);
#endif
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
            std::vector<std::string> othersOnFile = Server::getInstance().closeFile(
                    shared_from_this()->getCurrentFile(), shared_from_this()->getUsername());
            if (othersOnFile.empty()) { //nessun altro sul file
                std::cout << SocketManager::output() << "CLIENT "
                          << shared_from_this()->getId();
                if (shared_from_this()->getUsername() != "")
                    std::cout << " ("
                              << shared_from_this()->getUsername() << ")";
                if (js.at("username") != "")
                    std::cout << " LOGOUT ";
                else
                    std::cout << " CLOSE ";
                std::cout << "AND FREE FILE: "
                          << shared_from_this()->getCurrentFile() << std::endl;
            } else { //altri sul file
                std::vector<std::string> colors = Server::getInstance().getColors(othersOnFile);
                json j = json{{"response",   "update_participants"},
                              {"colorsList", colors},
                              {"usernames",  othersOnFile}};
                sendAllOtherClientsOnFile(j.dump());
                std::cout << SocketManager::output() << "CLIENT "
                          << shared_from_this()->getId();
                if (shared_from_this()->getUsername() != "")
                    std::cout << " ("
                              << shared_from_this()->getUsername() << ")";
                if (js.at("username") != "")
                    std::cout << " LOGOUT, ";
                else
                    std::cout << " CLOSE, ";
                std::cout << "STILL OPENED FILE: "
                          << shared_from_this()->getCurrentFile() << std::endl;
            }
            shared_from_this()->setCurrentFile("");
        } else {
            std::cout << SocketManager::output() << "CLIENT "
                      << shared_from_this()->getId();
            if (shared_from_this()->getUsername() != "")
                std::cout << " ("
                          << shared_from_this()->getUsername() << ")";
            if (js.at("username") != "")
                std::cout << " LOGOUT" << std::endl;
            else
                std::cout << " CLOSE" << std::endl;
        }
        shared_from_this()->setUsername("");
        return "logged_out";
    } else if (type_request == "req_logout_and_close") {
        if (!shared_from_this()->getCurrentFile().empty()) {
            std::vector<std::string> othersOnFile = Server::getInstance().closeFile(
                    shared_from_this()->getCurrentFile(), shared_from_this()->getUsername());
            if (othersOnFile.empty()) { //nessun altro sul file
                std::cout << SocketManager::output() << "CLIENT "
                          << shared_from_this()->getId();
                if (shared_from_this()->getUsername() != "")
                    std::cout << " ("
                              << shared_from_this()->getUsername() << ")";
                std::cout << " CLOSE AND FREE FILE: "
                          << shared_from_this()->getCurrentFile() << std::endl;
            } else { //altri sul file
                std::vector<std::string> colors = Server::getInstance().getColors(othersOnFile);
                json j = json{{"response",   "update_participants"},
                              {"colorsList", colors},
                              {"usernames",  othersOnFile}};
                sendAllOtherClientsOnFile(j.dump());
                std::cout << SocketManager::output() << "CLIENT "
                          << shared_from_this()->getId();
                if (shared_from_this()->getUsername() != "")
                    std::cout << " ("
                              << shared_from_this()->getUsername() << ")";
                std::cout << " CLOSE, STILL OPENED FILE: "
                          << shared_from_this()->getCurrentFile() << std::endl;
            }
            shared_from_this()->setCurrentFile("");
        } else {
            std::cout << SocketManager::output() << "CLIENT "
                      << shared_from_this()->getId();
            if (shared_from_this()->getUsername() != "")
                std::cout << " ("
                          << shared_from_this()->getUsername() << ")";
            std::cout << " CLOSE" << std::endl;
        }
        //rimuovi partipipant
        Server::getInstance().leave(shared_from_this());
    } else if (type_request == "close_file") {
        std::vector<std::string> othersOnFile = Server::getInstance().closeFile(shared_from_this()->getCurrentFile(),
                                                                                shared_from_this()->getUsername());
        if (othersOnFile.empty()) { //nessun altro sul file
            std::cout << SocketManager::output() << "CLIENT " << shared_from_this()->getId()
                      << " ("
                      << shared_from_this()->getUsername() << ") CLOSE AND FREE FILE: "
                      << shared_from_this()->getCurrentFile() << std::endl;
        } else { //altri sul file
            std::vector<std::string> colors = Server::getInstance().getColors(othersOnFile);
            json j = json{{"response",   "update_participants"},
                          {"usernames",  othersOnFile},
                          {"colorsList", colors}};
            sendAllClientsOnFile(j.dump());
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
        //prendo il vettore di symbol
        std::vector<std::string> usernameToInsert = js.at("usernameToInsert").get<std::vector<std::string>>();
        std::vector<wchar_t> charToInsert = js.at("charToInsert").get<std::vector<wchar_t>>();
        std::vector<std::vector<int>> crdtToInsert = js.at("crdtToInsert").get<std::vector<std::vector<int>>>();
        for (int i = 0; i < usernameToInsert.size(); i++) {
            //ricreo il simbolo
            Symbol symbolToInsert(charToInsert[i], usernameToInsert[i], crdtToInsert[i]);
            int index = Server::getInstance().generateIndexCRDT(symbolToInsert, shared_from_this()->getCurrentFile(), 0,
                                                                -1, -1);
            //aggiungo al crdt
            Server::getInstance().insertSymbolIndex(symbolToInsert, index, shared_from_this()->getCurrentFile());
        }
        Server::getInstance().modFile(shared_from_this()->getCurrentFile(), false, usernameToInsert.size());

        json j = json{{"response",         "insert_res"},
                      {"usernameToInsert", usernameToInsert},
                      {"charToInsert",     charToInsert},
                      {"crdtToInsert",     crdtToInsert}};
        sendAllOtherClientsOnFile(j.dump());
        return j.dump();
    } else if (type_request == "remove") {
        //prendo il vettore di symbol
        std::vector<Symbol> symbolsToErase;
        std::vector<std::string> usernameToErase = js.at("usernameToErase").get<std::vector<std::string>>();
        std::vector<wchar_t> charToErase = js.at("charToErase").get<std::vector<wchar_t>>();
        std::vector<std::vector<int>> crdtToErase = js.at("crdtToErase").get<std::vector<std::vector<int>>>();
        for (int i = 0; i < usernameToErase.size(); i++)
            symbolsToErase.emplace_back(charToErase[i], usernameToErase[i], crdtToErase[i]);
        //cancello dal crdt symbols compresi tra i due e prendo indici

        Server::getInstance().eraseSymbolCRDT(symbolsToErase, shared_from_this()->getCurrentFile());
        Server::getInstance().modFile(shared_from_this()->getCurrentFile(), false, symbolsToErase.size());

        json j = json{{"response",        "remove_res"},
                      {"usernameToErase", usernameToErase},
                      {"charToErase",     charToErase},
                      {"crdtToErase",     crdtToErase}};
        sendAllOtherClientsOnFile(j.dump());
        return j.dump();
    } else if (type_request == "request_new_file") {

        if (js.at("name").empty()) {
            json j = json{{"response", "invalid_name"}};
            sendAtClient(j.dump());
            return j.dump();
        }

#ifdef __linux__
        boost::filesystem::path personalDir(pathFilesystem + "/" + js.at("username").get<std::string>());
#else //winzoz
        boost::filesystem::path personalDir(pathFilesystem + "\\" + js.at("username").get<std::string>());
#endif
        if (!boost::filesystem::exists(personalDir)) { //anche se gia' fatto in signup
            //creazione cartella personale
            boost::filesystem::create_directory(personalDir);
        }
#ifdef __linux__
        std::string filename =
                pathFilesystem + "/" + js.at("username").get<std::string>() + "/" + js.at("name").get<std::string>() +
                ".txt";
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
            Server::getInstance().openFile(filename, shared_from_this()->getUsername());

            json j = json{{"response",   "new_file_created"},
                          {"filename",   js.at("name").get<std::string>()},
                          {"invitation", ManagementDB::getInstance().getInvitation(shared_from_this()->getUsername(),
                                                                                   js.at("name").get<std::string>())}};
            sendAtClient(j.dump());
            //mando lista participant sul file (solo lui per ora)
            std::vector<std::string> usernameOnFile;
            usernameOnFile.push_back(shared_from_this()->getUsername());
            std::vector<std::string> colors;
            colors.push_back(shared_from_this()->getColor());
            j = json{{"response",   "update_participants"},
                     {"usernames",  usernameOnFile},
                     {"colorsList", colors}};
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
#ifdef __linux__
            std::string filename = pathFilesystem + "/" + js.at("username").get<std::string>() + "/" +
                                   js.at("name").get<std::string>() + ".txt";
#else //winzoz
            std::string filename = pathFilesystem + "\\" + js.at("username").get<std::string>() + "\\" + js.at("name").get<std::string>() + ".txt";
#endif
            this->setCurrentFile(filename);

            if (Server::getInstance().isFileInFileSymbols(filename)) { //il file era gia' stato aperto (e' nella mappa)

                //aggiungo participant a lista participants del file
                Server::getInstance().insertUsernameInFile(filename, shared_from_this()->getUsername());

                std::vector<Symbol> symbols = Server::getInstance().getSymbolsPerFile(filename);
                int dim = symbols.size();
                std::cout << SocketManager::output() << "CLIENT " << this->getId() << " ("
                          << this->getUsername() << ") OPEN (NOT NEW) FILE (" << dim << " char): " << filename
                          << std::endl;
                std::vector<int> usernameToInsert;
                std::vector<wchar_t> charToInsert;
                std::vector<std::vector<int>> crdtToInsert;
                std::map<std::string, int> usernameToId;
                std::vector<int> styleBold;
                std::vector<int> styleItalic;
                std::vector<int> styleUnderlined;
                std::vector<std::string> styleColor;
                std::vector<std::string> styleFontFamily;
                std::vector<int> styleSize;
                bool allBoldStandard = true;
                bool allItalicStandard = true;
                bool allUnderlinedStandard = true;
                bool allFontFamilyStandard = true;
                bool allColorStandard = true;
                bool allSizeStandard = true;
                int countId = 0;
                int count = 0;
                for (int iter = 0; iter < dim; iter++) {
                    if (usernameToId.find(symbols[iter].getUsername()) == usernameToId.end())
                        usernameToId.insert(
                                {symbols[iter].getUsername(), countId++});

                    usernameToInsert.push_back(
                            usernameToId.at(symbols[iter].getUsername()));

                    charToInsert.push_back(symbols[iter].getCharacter());
                    crdtToInsert.push_back(symbols[iter].getPosizione());

                    styleBold.push_back(symbols[iter].getSymbolStyle().isBold() ? 1 : 0);
                    if (symbols[iter].getSymbolStyle().isBold())
                        allBoldStandard = false;
                    styleItalic.push_back(symbols[iter].getSymbolStyle().isItalic() ? 1 : 0);
                    if (symbols[iter].getSymbolStyle().isItalic())
                        allItalicStandard = false;
                    styleUnderlined.push_back(
                            symbols[iter].getSymbolStyle().isUnderlined() ? 1 : 0);
                    if (symbols[iter].getSymbolStyle().isUnderlined())
                        allUnderlinedStandard = false;
                    styleColor.push_back(
                            symbols[iter].getSymbolStyle().getColor() != DEFAULT_COLOR
                            ? symbols[iter].getSymbolStyle().getColor() : "0");
                    if (symbols[iter].getSymbolStyle().getColor() != DEFAULT_COLOR)
                        allColorStandard = false;
                    styleFontFamily.push_back(symbols[iter].getSymbolStyle().getFontFamily() !=
                                              DEFAULT_FONT_FAMILY ? symbols[iter].getSymbolStyle().getFontFamily()
                                                                  : "0");
                    if (symbols[iter].getSymbolStyle().getFontFamily() != DEFAULT_FONT_FAMILY)
                        allFontFamilyStandard = false;
                    styleSize.push_back(
                            symbols[iter].getSymbolStyle().getFontSize() != DEFAULT_FONT_SIZE
                            ? symbols[iter].getSymbolStyle().getFontSize() : 0);
                    if (symbols[iter].getSymbolStyle().getFontSize() != DEFAULT_FONT_SIZE)
                        allSizeStandard = false;
                    if (++count == maxBufferSymbol) {
                        std::vector<std::string> idToUsername;
                        while (idToUsername.size() != usernameToId.size())
                            for (auto &pair : usernameToId)
                                if (pair.second == idToUsername.size())
                                    idToUsername.push_back(pair.first);

                        json j = json{{"response",         "open_file"},
                                      {"usernameToInsert", usernameToInsert},
                                      {"charToInsert",     charToInsert},
                                      {"crdtToInsert",     crdtToInsert},
                                      {"usernameToId",     idToUsername},};
                        allBoldStandard ? j["boldDefault"] = "" : j["bold"] = styleBold;
                        allItalicStandard ? j["italicDefault"] = "" : j["italic"] = styleItalic;
                        allUnderlinedStandard ? j["underlinedDefault"] = "" : j["underlined"] = styleUnderlined;
                        allFontFamilyStandard ? j["fontFamilyDefault"] = "" : j["fontFamily"] = styleFontFamily;
                        allColorStandard ? j["colorDefault"] = "" : j["color"] = styleColor;
                        allSizeStandard ? j["sizeDefault"] = "" : j["size"] = styleSize;
                        sendAtClient(j.dump());

                        usernameToInsert.clear();
                        charToInsert.clear();
                        crdtToInsert.clear();
                        styleBold.clear();
                        styleItalic.clear();
                        styleUnderlined.clear();
                        styleSize.clear();
                        styleFontFamily.clear();
                        styleColor.clear();
                        allBoldStandard = true;
                        allItalicStandard = true;
                        allUnderlinedStandard = true;
                        allFontFamilyStandard = true;
                        allColorStandard = true;
                        allSizeStandard = true;
                        count = 0;
                    }
                }
                std::vector<std::string> idToUsername;
                while (idToUsername.size() != usernameToId.size())
                    for (auto &pair : usernameToId)
                        if (pair.second == idToUsername.size())
                            idToUsername.push_back(pair.first);
                json j = json{{"response",         "open_file"},
                              {"usernameToInsert", usernameToInsert},
                              {"charToInsert",     charToInsert},
                              {"crdtToInsert",     crdtToInsert},
                              {"usernameToId",     idToUsername},
                              {"endOpenFile",      ""}};
                allBoldStandard ? j["boldDefault"] = "" : j["bold"] = styleBold;
                allItalicStandard ? j["italicDefault"] = "" : j["italic"] = styleItalic;
                allUnderlinedStandard ? j["underlinedDefault"] = "" : j["underlined"] = styleUnderlined;
                allFontFamilyStandard ? j["fontFamilyDefault"] = "" : j["fontFamily"] = styleFontFamily;
                allColorStandard ? j["colorDefault"] = "" : j["color"] = styleColor;
                allSizeStandard ? j["sizeDefault"] = "" : j["size"] = styleSize;

                sendAtClient(j.dump());

            } else { //prima volta che il file viene aperto (lettura da file)
                Server::getInstance().openFile(filename, shared_from_this()->getUsername());
                std::ifstream file;
                file.open(filename);
                std::vector<int> usernameToInsert;
                std::vector<wchar_t> charToInsert;
                //std::vector<std::vector<int>> crdtToInsert;
                std::vector<int> styleBold;
                std::vector<int> styleItalic;
                std::vector<int> styleUnderlined;
                std::vector<std::string> styleColor;
                std::vector<std::string> styleFontFamily;
                std::vector<int> styleSize;
                bool allBoldStandard = true;
                bool allItalicStandard = true;
                bool allUnderlinedStandard = true;
                bool allFontFamilyStandard = true;
                bool allColorStandard = true;
                bool allSizeStandard = true;
                json jline, jline2;
                std::string line;
                int dim = 0;
                int symbolCount = 0;
                std::map<std::string, int> usernameToId;
                int countId = 0;
                while (std::getline(file, line)) {
                    jline = json::parse(line);
                    if (usernameToId.find(jline.at("username").get<std::string>()) == usernameToId.end())
                        usernameToId.insert({jline.at("username").get<std::string>(), countId++});
                    usernameToInsert.push_back(usernameToId.at(jline.at("username").get<std::string>()));

                    charToInsert.push_back(jline.at("character").get<wchar_t>());
                    //crdtToInsert.push_back(jline.at("posizione").get<std::vector<int>>());

                    std::getline(file, line);
                    jline2 = json::parse(line);

                    jline2.at("bold").get<bool>() ? styleBold.push_back(1) : styleBold.push_back(0);
                    if (jline2.at("bold").get<bool>())
                        allBoldStandard = false;
                    jline2.at("italic").get<bool>() ? styleItalic.push_back(1) : styleItalic.push_back(0);
                    if (jline2.at("italic").get<bool>())
                        allItalicStandard = false;
                    jline2.at("underlined").get<bool>() ? styleUnderlined.push_back(1) : styleUnderlined.push_back(0);
                    if (jline2.at("underlined").get<bool>())
                        allUnderlinedStandard = false;
                    jline2.at("size").get<int>() != DEFAULT_FONT_SIZE ? styleSize.push_back(
                            jline2.at("size").get<int>()) : styleSize.push_back(0);
                    if (jline2.at("size").get<int>() != DEFAULT_FONT_SIZE)
                        allSizeStandard = false;
                    jline2.at("fontFamily").get<std::string>() != DEFAULT_FONT_FAMILY ? styleFontFamily.push_back(
                            jline2.at("fontFamily").get<std::string>()) : styleFontFamily.push_back("0");
                    if (jline2.at("fontFamily").get<std::string>() != DEFAULT_FONT_FAMILY)
                        allFontFamilyStandard = false;
                    jline2.at("color").get<std::string>() != DEFAULT_COLOR ? styleColor.push_back(
                            jline2.at("color").get<std::string>()) : styleColor.push_back("0");
                    if (jline2.at("color").get<std::string>() != DEFAULT_COLOR)
                        allColorStandard = false;

                    Style style(jline2.at("bold").get<int>() == 1,
                                jline2.at("underlined").get<int>() == 1,
                                jline2.at("italic").get<int>() == 1,
                                jline2.at("fontFamily").get<std::string>(),
                                jline2.at("size").get<int>(),
                                jline2.at("color").get<std::string>());

                    Server::getInstance().insertSymbolIndex(
                            Symbol(jline.at("character").get<wchar_t>(), jline.at("username").get<std::string>(),
                                   jline.at("posizione").get<std::vector<int>>(), style), dim++, filename);
                    if (++symbolCount == maxBufferSymbol) {
                        std::vector<std::string> idToUsername;
                        while (idToUsername.size() != usernameToId.size())
                            for (auto &pair : usernameToId)
                                if (pair.second == idToUsername.size())
                                    idToUsername.push_back(pair.first);

                        json j = json{{"response",         "open_file"},
                                      {"usernameToInsert", usernameToInsert},
                                      {"charToInsert",     charToInsert},
                                /*{"crdtToInsert",     crdtToInsert},*/
                                      {"usernameToId",     idToUsername},
                                      {"first_open",       ""}};
                        allBoldStandard ? j["boldDefault"] = "" : j["bold"] = styleBold;
                        allItalicStandard ? j["italicDefault"] = "" : j["italic"] = styleItalic;
                        allUnderlinedStandard ? j["underlinedDefault"] = "" : j["underlined"] = styleUnderlined;
                        allFontFamilyStandard ? j["fontFamilyDefault"] = "" : j["fontFamily"] = styleFontFamily;
                        allColorStandard ? j["colorDefault"] = "" : j["color"] = styleColor;
                        allSizeStandard ? j["sizeDefault"] = "" : j["size"] = styleSize;
                        sendAtClient(j.dump());

                        symbolCount = 0;
                        usernameToInsert.clear();
                        charToInsert.clear();
                        //crdtToInsert.clear();
                        styleBold.clear();
                        styleItalic.clear();
                        styleUnderlined.clear();
                        styleSize.clear();
                        styleFontFamily.clear();
                        styleColor.clear();
                        allBoldStandard = true;
                        allItalicStandard = true;
                        allUnderlinedStandard = true;
                        allFontFamilyStandard = true;
                        allColorStandard = true;
                        allSizeStandard = true;
                    }
                }
                std::vector<std::string> idToUsername;
                while (idToUsername.size() != usernameToId.size())
                    for (auto &pair : usernameToId)
                        if (pair.second == idToUsername.size())
                            idToUsername.push_back(pair.first);
                json j = json{{"response",         "open_file"},
                              {"usernameToInsert", usernameToInsert},
                              {"charToInsert",     charToInsert},
                        /*{"crdtToInsert",     crdtToInsert},*/
                              {"usernameToId",     idToUsername},
                              {"first_open",       ""},
                              {"endOpenFile",      ""}};
                allBoldStandard ? j["boldDefault"] = "" : j["bold"] = styleBold;
                allItalicStandard ? j["italicDefault"] = "" : j["italic"] = styleItalic;
                allUnderlinedStandard ? j["underlinedDefault"] = "" : j["underlined"] = styleUnderlined;
                allFontFamilyStandard ? j["fontFamilyDefault"] = "" : j["fontFamily"] = styleFontFamily;
                allColorStandard ? j["colorDefault"] = "" : j["color"] = styleColor;
                allSizeStandard ? j["sizeDefault"] = "" : j["size"] = styleSize;

                sendAtClient(j.dump());

                std::cout << SocketManager::output() << "CLIENT " << this->getId() << " ("
                          << this->getUsername() << ") OPEN (NEW) FILE (" << dim << " char): " << filename << std::endl;

            }
            //Server::getInstance().printCRDT(filename); //debug

            //aggiorna su ogni client lista di participant su questo file
            std::vector<std::string> usernamesInFile = Server::getInstance().getUsernamesInFile(filename);
            std::vector<std::string> colors = Server::getInstance().getColors(usernamesInFile);
            json j = json{{"response",   "update_participants"},
                          {"usernames",  usernamesInFile},
                          {"colorsList", colors}};
            sendAllClientsOnFile(j.dump());
            return j.dump();
        } else {
            json j = json{{"response", "errore_apertura_file"}};
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

            std::cout << SocketManager::output() << "CLIENT " << this->getId();
            if (shared_from_this()->getUsername() != "")
                std::cout << " ("
                          << shared_from_this()->getUsername() << ")";
            std::cout << " ACCEPTED INVITATION, OWNER: " << resp.first << " FILE: " << resp.second << std::endl;
            return j.dump();
        } else {
            json j = json{{"response", "invitation_error"}};
            sendAtClient(j.dump());
            return j.dump();
        }
    } else if (type_request == "update_cursorPosition") {
        json j = json{{"response", "update_cursorPosition"},
                      {"username", js.at("username").get<std::string>()},
                      {"pos",      js.at("pos").get<int>()}
        };
        sendAllOtherClientsOnFile(j.dump());
        return js.dump();
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

#ifdef __linux__
        std::string new_path = pathFilesystem + "/" + js.at("username").get<std::string>() + "/" +
                               js.at("new_name").get<std::string>() + ".txt";
        std::string old_path = pathFilesystem + "/" + js.at("username").get<std::string>() + "/" +
                               js.at("old_name").get<std::string>() + ".txt";
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

        std::vector<std::string> invited;
        std::string resDB = ManagementDB::getInstance().handleRenameFile(js.at("username").get<std::string>(),
                                                                         js.at("old_name").get<std::string>(),
                                                                         js.at("new_name").get<std::string>(),
                                                                         invited);
        if (resDB == "FILE_RENAME_SUCCESS") {


            boost::filesystem::rename(old_path, new_path);
            std::string filenameRedableOld = old_path;
            filenameRedableOld.erase(filenameRedableOld.size() - 4);
            filenameRedableOld += "_readable.txt";
            std::string filenameRedableNew = new_path;
            filenameRedableNew.erase(filenameRedableNew.size() - 4);
            filenameRedableNew += "_readable.txt";
            boost::filesystem::rename(filenameRedableOld, filenameRedableNew);

            json j = json{{"response", "file_renamed"},
                          {"newName",  js.at("new_name").get<std::string>()},
                          {"oldName",  js.at("old_name").get<std::string>()},
                          {"owner",    shared_from_this()->getUsername()}};
            for (auto &usernameInvited : invited)
                sendAtClientUsername(j.dump(), usernameInvited);

            std::cout << SocketManager::output() << "CLIENT " << this->getId();
            if (shared_from_this()->getUsername() != "")
                std::cout << " ("
                          << shared_from_this()->getUsername() << ")";
            std::cout << " RENAMED FILE, FROM " << js.at("old_name").get<std::string>() << " TO:"
                      << js.at("new_name").get<std::string>() << std::endl;
            return j.dump();
        } else {
            json j = json{{"response", "ERRORE_RINOMINA_FILE"}};
            sendAtClient(j.dump());
            return j.dump();
        }


    } else if (type_request == "delete_file") {
#ifdef __linux__
        std::string filename =
                pathFilesystem + "/" + js.at("username").get<std::string>() + "/" +
                js.at("name").get<std::string>() +
                ".txt";
#else //winzoz
        std::string filename = pathFilesystem + "\\" + js.at("username").get<std::string>() + "\\" + js.at("name").get<std::string>() + ".txt";
#endif

        if (Server::getInstance().isFileInFileSymbols(filename)) {
            //file aperto da qualche utente, impossibile rinominare
            json j = json{{"response", "FILE_IN_USE_D"}};
            sendAtClient(j.dump());
            return j.dump();
        }

        std::vector<std::string> invited;
        std::string resDB = ManagementDB::getInstance().handleDeleteFile(js.at("username").get<std::string>(),
                                                                         js.at("name").get<std::string>(),
                                                                         js.at("owner").get<std::string>(),
                                                                         invited);
        if (resDB == "FILE_DELETE_SUCCESS") {
            boost::filesystem::remove(filename);
            std::string filenameRedable = filename;
            filenameRedable.erase(filenameRedable.size() - 4);
            filenameRedable += "_readable.txt";
            boost::filesystem::remove(filenameRedable);
            json j = json{{"response", "file_deleted"},
                          {"name",     js.at("name").get<std::string>()},
                          {"username", js.at("username").get<std::string>()},
                          {"owner",    js.at("owner").get<std::string>()}};
            if (invited.empty()) {
                sendAtClient(j.dump()); //cancellazione invito
            } else
                for (auto &invitedUsername : invited)
                    sendAtClientUsername(j.dump(), invitedUsername);

            std::cout << SocketManager::output() << "CLIENT " << this->getId();
            if (shared_from_this()->getUsername() != "")
                std::cout << " ("
                          << shared_from_this()->getUsername() << ")";
            std::cout << " DELETED FILE, OWNER: " << js.at("owner").get<std::string>() << " FILE: "
                      << js.at("name").get<std::string>() << std::endl;
            return j.dump();
        } else {
            json j = json{{"response", "ERRORE_ELIMINAZIONE_FILE"}};
            sendAtClient(j.dump());
            return j.dump();
        }

    } else if (type_request == "request_update_profile") {
        std::string username, email, oldPassword, newPassword, color;

        username = js.at("username").get<std::string>();
        email = js.at("email").get<std::string>();
        newPassword = js.at("newPassword").get<std::string>();
        oldPassword = js.at("oldPassword").get<std::string>();
        color = js.at("newColor").get<std::string>();

        //devo creare in Managementdb una funzione handleEditProfile e fare le operazioni nel DB
        std::string resDB = ManagementDB::getInstance().handleEditProfile(username, email, newPassword,
                                                                          oldPassword, color);
        if (resDB == "EDIT_SUCCESS") {
            if(!color.empty())
                shared_from_this()->setColor(color);
            json j = json{{"response", resDB},
                          {"username", username},
                          {"email",    email},
                          {"color",    color}};
            sendAtClient(j.dump());
            return j.dump();
        } else {
            json j = json{{"response", resDB},
                          {"username", username}};
            sendAtClient(j.dump());
            return j.dump();
        }

    } else if (type_request == "styleChanged") {
        //prendo il vettore di symbol
        std::vector<std::string> usernameToChange = js.at("usernameToChange").get<std::vector<std::string>>();
        std::vector<wchar_t> charToChange = js.at("charToChange").get<std::vector<wchar_t>>();
        std::vector<std::vector<int>> crdtToChange = js.at("crdtToChange").get<std::vector<std::vector<int>>>();

        json j = json{{"response",         "styleChanged_res"},
                      {"usernameToChange", usernameToChange},
                      {"charToChange",     charToChange},
                      {"crdtToChange",     crdtToChange}
        };
        if (js.contains("bold"))
            j["bold"] = js.at("bold").get<bool>();
        if (js.contains("italic"))
            j["italic"] = js.at("italic").get<bool>();
        if (js.contains("underlined"))
            j["underlined"] = js.at("underlined").get<bool>();
        if (js.contains("color"))
            j["color"] = js.at("color").get<std::string>();
        if (js.contains("fontFamily"))
            j["fontFamily"] = js.at("fontFamily").get<std::string>();
        if (js.contains("size"))
            j["size"] = js.at("size").get<int>();

        for (int i = 0; i < usernameToChange.size(); i++)
            Server::getInstance().changeStyle(Symbol(charToChange[i], usernameToChange[i], crdtToChange[i]), j,
                                              this->getCurrentFile());

        sendAllOtherClientsOnFile(j.dump());
        return j.dump();
    } else if (type_request == "insertAndStyle") {
        //prendo il vettore di symbol
        std::vector<std::string> usernameToInsert = js.at("usernameToInsert").get<std::vector<std::string>>();
        std::vector<wchar_t> charToInsert = js.at("charToInsert").get<std::vector<wchar_t>>();
        std::vector<std::vector<int>> crdtToInsert = js.at("crdtToInsert").get<std::vector<std::vector<int>>>();
        std::string fontFamily = js.at("fontFamily").get<std::string>();
        int fontSize = js.at("size").get<int>();
        bool bold = js.at("bold").get<bool>();
        bool italic = js.at("italic").get<bool>();
        bool underlined = js.at("underlined").get<bool>();
        std::string color = js.at("color").get<std::string>();
        //TO DO: non dovrebbe esserci solo un elmento?
        for (int i = 0; i < usernameToInsert.size(); i++) {
            //ricreo il simbolo
            Style style = {bold, underlined, italic, fontFamily, fontSize, color};
            //Symbol symbolToInsert(charToInsert[i], usernameToInsert[i], crdtToInsert[i]);
            Symbol symbolToInsert(charToInsert[i], usernameToInsert[i], crdtToInsert[i], style);
            int index = Server::getInstance().generateIndexCRDT(symbolToInsert, shared_from_this()->getCurrentFile(), 0,
                                                                -1, -1);
            //aggiungo al crdt
            Server::getInstance().insertSymbolIndex(symbolToInsert, index, shared_from_this()->getCurrentFile());
        }
        Server::getInstance().modFile(shared_from_this()->getCurrentFile(), false, usernameToInsert.size());

        json j = json{{"response",         "insertAndStyle_res"},
                      {"usernameToInsert", usernameToInsert},
                      {"charToInsert",     charToInsert},
                      {"crdtToInsert",     crdtToInsert},
                      {"bold",             bold},
                      {"italic",           italic},
                      {"underlined",       underlined},
                      {"color",            color},
                      {"fontFamily",       fontFamily},
                      {"size",             fontSize}};
        sendAllOtherClientsOnFile(j.dump());
        return j.dump();
    } else {
        std::cout << "nessun match col tipo di richiesta" <<
                  std::endl;
        json j = json{{"response", "general_error"}};
        return j.dump();
    }
    return "";
}

void HandleRequest::deliver(const Message &msg) {
    bool write_in_progress = !write_msgs_.empty();
    //aggiungo un nuovo elemento alla fine della coda
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
        do_write();
    }
}

void HandleRequest::sendAtClientUsername(const std::string &response, const std::string &username) {
    participant_ptr participant = Server::getInstance().getParticipant(username);
    if (participant == nullptr)
        return;
    std::size_t len = response.size();
    Message msg;
    msg.body_length(len);
    std::memcpy(msg.body(), response.data(), msg.body_length());
    msg.body()[msg.body_length()] = '\0';
    msg.encode_header();
    if (debugOutput)
        std::cout << SocketManager::output() << "RESPONSE TO CLIENT " << participant->getId() << " ("
                  << participant->getUsername() << "): " << msg.body() << std::endl;
    participant->deliver(msg);
}
