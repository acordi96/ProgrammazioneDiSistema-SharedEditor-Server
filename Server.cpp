//
// Created by Sam on 19/apr/2020.
//

#include "Server.h"
#include "Connection.h"

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

void Server::deliverToAllOnFile(const std::string &filename, const message &msg, const int &partecipantId) {
    recent_msgs_.push_back(msg);

    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    //non a tutti ma a tutti su quel file
    for (const auto &p: this->participantsPerFile.at(filename)) {
        if (p->getId() != partecipantId) {
            p->deliver(msg);
        }
    }
}

void Server::send(const MessageSymbol &m) {
    infoMsgs_.push(m);
}

void Server::insertNewFileSymbols(const std::string &filename) {
    this->room_map_.emplace(std::pair<std::string, std::vector<Symbol>>(filename, std::vector<Symbol>()));
}

bool Server::isFileInFileSymbols(const std::string &filename) {
    if (this->room_map_.find(filename) == this->room_map_.end())
        return false;
    return true;
}

MessageSymbol Server::insertSymbol(const std::string &filename, int index, char character, int id) {
    std::vector<int> vector;
    if (this->room_map_.at(filename).empty()) {
        vector = {0};
        index = 0;
    } else if (index > this->room_map_.at(filename).size() - 1) {
        vector = {this->room_map_.at(filename).back().getPosizione().at(0) + 1};
        index = this->room_map_.at(filename).size();
    } else if (index == 0) {
        vector = {this->room_map_.at(filename).front().getPosizione().at(0) - 1};
    } else
        vector = generateNewPosition(filename, index);
    Symbol s(character, std::make_pair(id, this->room_map_.at(filename).size() + 1), vector);

    this->room_map_.at(filename).insert(this->room_map_.at(filename).begin() + index, s);

    MessageSymbol m(0, id, s, index);

    return m;
}

MessageSymbol Server::eraseSymbol(const std::string &filename, int startIndex, int endIndex, int id) {
    Symbol s = this->room_map_.at(filename).at(startIndex);
    this->room_map_.at(filename).erase(this->room_map_.at(filename).begin() + startIndex,
                                       this->room_map_.at(filename).begin() + endIndex);
    MessageSymbol m(1, id, s);
    return m;
}

std::vector<int> Server::generateNewPosition(const std::string &filename, int index) {
    std::vector<int> posBefore = this->room_map_.at(filename)[index - 1].getPosizione();
    std::vector<int> posAfter = this->room_map_.at(filename)[index].getPosizione();
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
}

void Server::insertParticipantInFile(const std::string &filename, const participant_ptr &participant) {
    if (this->participantsPerFile.find(filename) == this->participantsPerFile.end()) {
        std::vector<participant_ptr> newVector;
        newVector.push_back(participant);
        this->participantsPerFile.insert(std::make_pair(filename, newVector));
    } else
        this->participantsPerFile.at(filename).push_back(participant);
}

std::vector<Symbol> Server::getSymbolsPerFile(const std::string &filename) {
    return this->room_map_.at(filename);
}

std::vector<participant_ptr> Server::getParticipantsInFile(const std::string &filename) {
    return this->participantsPerFile.at(filename);
}

bool Server::removeParticipantInFile(const std::string& filename, int id) {
    for (auto it = this->participantsPerFile.at(filename).begin() ; it != this->participantsPerFile.at(filename).end(); ++it) {
        if(it->get()->getId() == id) {
            this->participantsPerFile.at(filename).erase(it);
        }
    }
    //se era l'unico sul file dealloca cose
    if(this->participantsPerFile.at(filename).empty()) {
        this->participantsPerFile.erase(filename);
        this->room_map_.erase(filename);
        return true;
    }
    return false;
}
