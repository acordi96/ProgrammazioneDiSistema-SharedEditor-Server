//
// Created by Sam on 19/apr/2020.
//

#include <fstream>
#include <boost/filesystem.hpp>
#include "Headers/Server.h"
#include "Headers/SocketManager.h"

#define nModsBeforeWrite 15 //numero di m/home/jaceschrist/Documents/progettoPDS/serverPDSodifiche per modificare il file (>0)

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
}

void Server::leave(const participant_ptr &participant) {
    participants_.erase(participant);
}

void Server::deliver(const message &msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    for (const auto &p: participants_)
        p->deliver(msg);
}

void Server::deliverToAllOnFile(const message &msg, const participant_ptr& participant) {
    recent_msgs_.push_back(msg);

    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    //non a tutti ma a tutti su quel file
    for (const auto &p: this->participantsPerFile.at(participant->getCurrentFile())) {
        if (p->getId() != participant->getId()) {
            p->deliver(msg);
        }
    }
}

void Server::send(const MessageSymbol &m) {
    infoMsgs_.push(m);
}

void Server::openFile(const participant_ptr &participant) {
    //inserisco entry in mappa dei simboli
    this->symbolsPerFile.emplace(std::pair<std::string, std::vector<Symbol>>(participant->getCurrentFile(), std::vector<Symbol>()));
    //inserisco entry in mappa delle modifiche
    this->modsPerFile.emplace(std::pair<std::string, int>(participant->getCurrentFile(), 0));
    //aggiungo participant a lista participants del file
    Server::getInstance().insertParticipantInFile(participant);
}

bool Server::isFileInFileSymbols(const std::string &filename) {
    if (this->symbolsPerFile.find(filename) == this->symbolsPerFile.end())
        return false;
    return true;
}

MessageSymbol Server::insertSymbol(const std::string &filename, int index, char character, int id) {
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
        vector = generateNewPosition(filename, index);
    Symbol s(character, std::make_pair(id, this->symbolsPerFile.at(filename).size() + 1), vector);

    this->symbolsPerFile.at(filename).insert(this->symbolsPerFile.at(filename).begin() + index, s);

    MessageSymbol m(0, id, s, index);

    return m;
}

MessageSymbol Server::eraseSymbol(const std::string &filename, int startIndex, int endIndex, int id) {
    Symbol s = this->symbolsPerFile.at(filename).at(startIndex);
    this->symbolsPerFile.at(filename).erase(this->symbolsPerFile.at(filename).begin() + startIndex,
                                            this->symbolsPerFile.at(filename).begin() + endIndex);
    MessageSymbol m(1, id, s);
    return m;
}

std::vector<int> Server::generateNewPosition(const std::string &filename, int index) {
    std::vector<int> posBefore = this->symbolsPerFile.at(filename)[index - 1].getPosizione();
    std::vector<int> posAfter = this->symbolsPerFile.at(filename)[index].getPosizione();
    std::vector<int> newPos;
    int idBefore = posBefore.at(0);
    int idAfter = posAfter.at(0);
    if (idBefore - idAfter == 0) {
        newPos.push_back(idBefore);
        posAfter.erase(posAfter.begin());
        posBefore.erase(posBefore.begin());
        if (posAfter.empty()) {
            newPos.push_back(posBefore.front() + 1);
            return newPos;
        }
    } else if (idAfter - idBefore > 1) {
        newPos.push_back(posBefore.front() + 1);
        return newPos;
    } else if (idAfter - idBefore == 1) {
        newPos.push_back(idBefore);
        posBefore.erase(posBefore.begin());
        if (posBefore.empty()) {
            newPos.push_back(0);
            return newPos;
        } else {
            newPos.push_back(posBefore.front() + 1);
            return newPos;
        }
    }
    return std::vector<int>();
}

void Server::insertParticipantInFile(const participant_ptr &participant) {
    if (this->participantsPerFile.find(participant->getCurrentFile()) == this->participantsPerFile.end()) {
        std::vector<participant_ptr> newVector;
        newVector.push_back(participant);
        this->participantsPerFile.insert(std::make_pair(participant->getCurrentFile(), newVector));
    } else
        this->participantsPerFile.at(participant->getCurrentFile()).push_back(participant);
}

std::vector<Symbol> Server::getSymbolsPerFile(const std::string &filename) {
    return this->symbolsPerFile.at(filename);
}

std::vector<participant_ptr> Server::getParticipantsInFile(const std::string &filename) {
    return this->participantsPerFile.at(filename);
}

bool Server::removeParticipantInFile(const std::string &filename, int id) {
    for (auto it = this->participantsPerFile.at(filename).begin();
         it != this->participantsPerFile.at(filename).end(); ++it) {
        if (it->get()->getId() == id) {
            this->participantsPerFile.at(filename).erase(it);
            break;
        }
    }
    if (this->participantsPerFile.at(filename).empty()) {    //se era l'unico/ultimo sul file
        //forza scrittura su file
        this->modFile(filename, true);
        //dealloca strutture
        this->participantsPerFile.erase(filename);
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

void Server::removeParticipant(const participant_ptr &participant) {
    for (auto it = participants_.begin(); it != participants_.end(); ++it) {
        if (it->get()->getId() == participant->getId()) {
            this->participants_.erase(it);
            break;
        }
    }
}

std::vector<int> Server::closeFile(const participant_ptr &participant) {
    std::vector<int> othersOnFile;
    if (this->removeParticipantInFile(participant->getCurrentFile(),
                                      participant->getId())) { //era l'ultimo sul file
        return othersOnFile;
    } else {
//ci sono altri sul file, li informo della mia uscita
        std::vector<participant_ptr> participantsOnFile = Server::getInstance().getParticipantsInFile(
                participant->getCurrentFile());
        for (const auto &participants : participantsOnFile) {
            othersOnFile.push_back(participants->getId());
        }
        return othersOnFile;
    }
}

std::vector<std::string> Server::getColors(const std::vector<int>& users) {
    std::vector<std::string> colors;
    for(auto &participant : participants_) {
        for(auto &user : users) {
            if(participant->getId() == user) {
                colors.push_back(participant->getColor());
                break;
            }
        }
    }
    return colors;
}
