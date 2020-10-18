//
// Created by Sam on 19/apr/2020.
//

#include <fstream>
#include <boost/filesystem.hpp>
#include "Headers/Server.h"
#include "Headers/SocketManager.h"

#define nModsBeforeWrite 150 //numero di modifiche prima di modificare il file (>0)

Server::~Server() {
    std::vector<std::string> openFiles;
    for (auto &pair : this->symbolsPerFile)
        openFiles.push_back(pair.first);
    for (auto &filename : openFiles)
        this->modFile(filename, true);
}

Server &Server::getInstance() {
    static Server instance;
    return instance;
}

void Server::join(const participant_ptr &participant) {
    participants_.insert(participant);
    std::cout << SocketManager::output() << "PARTICIPANTS LIST UPDATED ("
              << std::to_string(participants_.size())
              << "): ";
    for (auto &p : participants_) {
        std::cout << "[ID: " << std::to_string(p->getId()) << ", USERNAME: ";
        if (p->getUsername().empty())
            std::cout << "''";
        else
            std::cout << p->getUsername();
        std::cout << ", FILE: ";
        if (p->getCurrentFile().empty())
            std::cout << "(NO)], ";
        else
            std::cout << "(YES)], ";
    }
    std::cout << std::endl;
}

void Server::leave(const participant_ptr &participant) {
    for (auto &p : participants_) {
        if (p == participant) {
            participants_.erase(p);
            std::cout << SocketManager::output() << "PARTICIPANTS LIST UPDATED ("
                      << std::to_string(participants_.size())
                      << "): ";
            for (auto &p : participants_) {
                std::cout << "[ID: " << std::to_string(p->getId()) << ", USERNAME: ";
                if (p->getUsername().empty())
                    std::cout << "''";
                else
                    std::cout << p->getUsername();
                std::cout << ", FILE: ";
                if (p->getCurrentFile().empty())
                    std::cout << "(NO)], ";
                else
                    std::cout << "(YES)], ";
            }
            std::cout << std::endl;
            return;
        }
    }
}

void Server::deliver(const Message &msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    for (const auto &p: participants_)
        p->deliver(msg);
}

void Server::deliverToAllOtherOnFile(const Message &msg, const participant_ptr &participant) {
    recent_msgs_.push_back(msg);

    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    //non a tutti ma a tutti su quel file
    for (const auto &username: this->usernamePerFile.at(participant->getCurrentFile())) {
        if (username != participant->getUsername()) {
            this->getParticipant(username)->deliver(msg);
        }
    }
}

void Server::deliverToAllOnFile(const Message &msg, const participant_ptr &participant) {
    recent_msgs_.push_back(msg);

    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    //non a tutti ma a tutti su quel file
    for (const auto &username: this->usernamePerFile.at(participant->getCurrentFile())) {
        this->getParticipant(username)->deliver(msg);
    }
}

void Server::send(const MessageSymbol &m) {
    infoMsgs_.push(m);
}

void Server::openFile(const std::string &filename, const std::string &username) {
    //inserisco entry in mappa dei simboli
    this->symbolsPerFile.emplace(
            std::pair<std::string, std::vector<Symbol>>(filename, std::vector<Symbol>()));
    //inserisco entry in mappa delle modifiche
    this->modsPerFile.emplace(std::pair<std::string, int>(filename, 0));
    //aggiungo participant a lista participants del file
    Server::getInstance().insertUsernameInFile(filename, username);
}

bool Server::isFileInFileSymbols(const std::string &filename) {
    if (this->symbolsPerFile.find(filename) == this->symbolsPerFile.end())
        return false;
    return true;
}

void Server::insertUsernameInFile(const std::string &filename, const std::string &username) {
    if (this->usernamePerFile.find(filename) == this->usernamePerFile.end()) {
        std::vector<std::string> newVector;
        newVector.push_back(username);
        this->usernamePerFile.insert(std::make_pair(filename, newVector));
    } else {
        for (auto &usr : this->usernamePerFile.at(filename))
            if (usr == username)
                return;
        this->usernamePerFile.at(filename).push_back(username);
    }
}

std::vector<Symbol> Server::getSymbolsPerFile(const std::string &filename) {
    return this->symbolsPerFile.at(filename);
}

std::vector<std::string> Server::getUsernamesInFile(const std::string &filename) {
    return this->usernamePerFile.at(filename);
}

bool Server::removeUsernameFromFile(const std::string &filename, const std::string &username) {
    for (auto it = this->usernamePerFile.at(filename).begin();
         it != this->usernamePerFile.at(filename).end(); ++it) {
        if (*it == username) {
            this->usernamePerFile.at(filename).erase(it);
            break;
        }
    }
    if (this->usernamePerFile.at(filename).empty()) {    //se era l'unico/ultimo sul file
        //forza scrittura su file
        this->modFile(filename, true);
        //dealloca strutture
        this->usernamePerFile.erase(filename);
        this->symbolsPerFile.erase(filename);
        this->modsPerFile.erase(filename);
        return true;
    }
    return false;
}

void Server::modFile(const std::string &filename, bool force) {
    if (force && (this->modsPerFile.at(filename) == 0)) //scrittura forzata ma file gia' sincronizzato
        return;
    if (!force)
        this->modsPerFile.at(filename)++;
    if (this->modsPerFile.at(filename) >= nModsBeforeWrite || force) {
        std::ofstream file;
        file.open(filename);
        char crdtToWrite[this->symbolsPerFile.at(filename).size()];
        int i = 0;
        for (auto &symbol : this->symbolsPerFile.at(filename)) {
            crdtToWrite[i++] = symbol.getCharacter();
        }
        file.write(crdtToWrite, this->symbolsPerFile.at(filename).size());
        file.close();
        std::cout << SocketManager::output() << "UPDATED" << " (" << this->modsPerFile.at(filename) << " MODS)"
                  << " LOCAL FILE: " << filename << std::endl;
        this->modsPerFile.at(filename) = 0;
    }
}

std::vector<std::string> Server::closeFile(const std::string &filename, const std::string &username) {
    std::vector<std::string> othersOnFile;
    if (this->removeUsernameFromFile(filename, username)) { //era l'ultimo sul file
        return othersOnFile;
    } else {
//ci sono altri sul file, li informo della mia uscita
        othersOnFile = Server::getInstance().getUsernamesInFile(filename);
        return othersOnFile;
    }
}

std::vector<std::string> Server::getColors(const std::vector<std::string> &usernames) {
    std::vector<std::string> colors;
    for (auto &user : usernames) {
        for (auto &participant : participants_) {
            if (participant->getUsername() == user) {
                colors.emplace_back(participant->getColor());
            }
        }
    }
    return colors;
}

bool Server::isParticipantIn(int id) {
    for (auto &participant : participants_)
        if (participant->getId() == id)
            return true;
    return false;
}

bool Server::isParticipantIn(const std::string &username) {
    for (auto &participant : participants_)
        if (participant->getUsername() == username)
            return true;
    return false;
}

participant_ptr Server::getParticipant(const std::string &username) {
    for (auto &participant : participants_) {
        if (participant->getUsername() == username)
            return participant;
    }
    return nullptr;
}

int Server::getOutputcount() {
    return this->countOutput++;
}

/* ########################################### CRDT METHODS ########################################### */

std::vector<int>
Server::insertSymbolNewCRDT(int index, char character, const std::string &username, const std::string &filename) {
    std::vector<int> vector;
    if (this->symbolsPerFile.at(filename).empty()) {
        vector = {0};
        index = 0;
    } else if (index > this->symbolsPerFile.at(filename).size() - 1) {
        vector = {this->symbolsPerFile.at(filename).back().getPosizione().at(0) + 1};
        index = this->symbolsPerFile.at(filename).size();
    } else if (index == 0) {
        vector = {this->symbolsPerFile.at(filename).front().getPosizione().at(0) - 1};
    } else
        vector = generatePos(index, filename);
    Symbol s(character, username, vector);

    this->symbolsPerFile.at(filename).insert(this->symbolsPerFile.at(filename).begin() + index, s);

    return vector;
}

//generate for a new symbol the index position in a symbols vector
int Server::generateIndexCRDT(Symbol symbol, const std::string &filename, int iter, int start, int end) {
    if (start == -1 && end == -1) {
        if (this->symbolsPerFile.at(filename).empty())
            return 0;
        if (symbol.getPosizione()[0] < this->symbolsPerFile.at(filename)[0].getPosizione()[0])
            return 0;
        start = 0;
        end = this->symbolsPerFile.at(filename).size();
    }
    if (start == end) {
        return iter;
    }
    int newStart = -1;
    int newEnd = start;
    for (auto iterPositions = this->symbolsPerFile.at(filename).begin() + start;
         iterPositions != this->symbolsPerFile.at(filename).begin() + end; ++iterPositions) {
        if (iterPositions->getPosizione().size() > iter && symbol.getPosizione().size() > iter) {
            if (iterPositions->getPosizione()[iter] == symbol.getPosizione()[iter] && newStart == -1)
                newStart = newEnd;
            if (iterPositions->getPosizione()[iter] > symbol.getPosizione()[iter]) {
                if (newStart == -1)
                    return newEnd;
                else
                    return generateIndexCRDT(symbol, filename, ++iter, newStart, newEnd);
            }
        }
        newEnd++;
    }
    return newEnd;
}

void Server::insertSymbolIndex(const Symbol &symbol, int index, const std::string &filename) {
    int i = 0;
    for (auto iterPositions = this->symbolsPerFile.at(filename).begin();
         iterPositions != this->symbolsPerFile.at(filename).end(); ++iterPositions) {
        if (index == i) {
            this->symbolsPerFile.at(filename).insert(iterPositions, symbol);
            return;
        }
        i++;
    }
    this->symbolsPerFile.at(filename).insert(this->symbolsPerFile.at(filename).end(), symbol);
}

void Server::eraseSymbolCRDT(std::vector<Symbol> symbolsToErase, const std::string &filename) {
    int lastFound = 0;
    for (auto iterSymbolsToErase = symbolsToErase.begin();
         iterSymbolsToErase != symbolsToErase.end(); ++iterSymbolsToErase) {
        if ((lastFound - 2) >= 0)
            lastFound -= 2;
        else if ((lastFound - 1) >= 0)
            lastFound--;
        int count = lastFound;
        bool foundSecondPart = false;
        for (auto iterSymbols = this->symbolsPerFile.at(filename).begin() + lastFound; iterSymbols !=
                                                                                       this->symbolsPerFile.at(
                                                                                               filename).end(); ++iterSymbols) { //cerca prima da dove hai trovato prima in poi
            if (*iterSymbolsToErase == *iterSymbols) {
                this->symbolsPerFile.at(filename).erase(iterSymbols);
                lastFound = count;
                foundSecondPart = true;
                break;
            }
            count++;
        }
        if (!foundSecondPart) { //se non hai trovato cerca anche nella prima parte
            for (auto iterSymbols = this->symbolsPerFile.at(filename).begin();
                 iterSymbols != this->symbolsPerFile.at(filename).begin() + lastFound; ++iterSymbols) {
                if (*iterSymbolsToErase == *iterSymbols) {
                    this->symbolsPerFile.at(filename).erase(iterSymbols);
                    lastFound = count;
                    break;
                }
                count++;
            }
        }
    }
}

std::vector<int> Server::generatePos(int index, std::string filename) {
    const std::vector<int> posBefore = symbolsPerFile.at(filename)[index - 1].getPosizione();
    const std::vector<int> posAfter = symbolsPerFile.at(filename)[index].getPosizione();
    std::vector<int> newPos;
    return generatePosBetween(posBefore, posAfter, newPos);
}

std::vector<int> Server::generatePosBetween(std::vector<int> pos1, std::vector<int> pos2, std::vector<int> newPos) {
    int id1 = pos1.at(0);
    int id2 = pos2.at(0);

    if (id2 - id1 == 0) { // [1] [1 0] or [1 0] [1 1]
        newPos.push_back(id1);
        pos1.erase(pos1.begin());
        pos2.erase(pos2.begin());
        if (pos1.empty()) {
            newPos.push_back(pos2.front() - 1); // [1] [1 0] -> [1 -1]
            return newPos;
        } else
            return generatePosBetween(pos1, pos2, newPos); // [1 0] [1 1] -> recall and enter third if
    } else if (id2 - id1 > 1) { // [0] [3]
        newPos.push_back(pos1.front() + 1); // [0] [3] -> [1]
        return newPos;
    } else if (id2 - id1 == 1) { // [1] [2] or [1 1] [2]
        newPos.push_back(id1);
        pos1.erase(pos1.begin());
        if (pos1.empty()) {
            newPos.push_back(0); // [1] [2] -> [1 0]
            return newPos;
        } else {
            newPos.push_back(pos1.front() + 1); // [1 1] [2] -> [1 2]
            return newPos;
        }
    }
    return std::vector<int>();
}
