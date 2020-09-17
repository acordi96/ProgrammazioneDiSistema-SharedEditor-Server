//
// Created by Sam on 19/apr/2020.
//

#include "Room.h"

void Room::join(participant_ptr participant) {
    participants_.insert(participant);
    for (auto msg: recent_msgs_)
        participant->deliver(msg);
}

void Room::leave(participant_ptr participant) {
    participants_.erase(participant);
}

void Room::deliver(const message &msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    for (const auto& p: participants_)
        p->deliver(msg);
}

void Room::deliverToAll(const message &msg, const int &partecipantId) {
    recent_msgs_.push_back(msg);

    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    /*//TODO: ricerca nella room da ottimizzare splittando room
    std::string onFile;
    for(const auto& p: participants_) {
        if(p->getId() == partecipantId) {
            onFile = p->getCurrentFile();
            break;
        }
    }*/
    for(const auto& p: this->participants_){
        //if(p->getId()!=partecipantId && p->getCurrentFile() == onFile) {
            p->deliver(msg);
        //}
    }
}

const std::map<std::string, std::vector<Symbol>> &Room::getRoomMap() const {
    return room_map_;
}

void Room::setRoomMap(const std::map<std::string, std::vector<Symbol>> &roomMap) {
    room_map_ = roomMap;
}

void Room::send(const MessageSymbol &m) {
    infoMsgs_.push(m);
}
//questo serve a mandare la stessa coda di messaggi a tutti i partecipanti
void Room::dispatchMessages() {
    while(!infoMsgs_.empty()) {
        for (const auto& it: participants_) {
            if (it->getId() != infoMsgs_.front().getId())
                it->process(infoMsgs_.front());
        }
        infoMsgs_.pop();
    }
}

MessageSymbol Room::localInsert(int index, char c, int id) {
std::vector<int> vector;
if(this->_symbol.empty()){
    vector = {0};
    index = 0;
}else if(index > this->_symbol.size()-1){
    vector = {this->_symbol.back().getPosizione().at(0)+1};
    index = this->_symbol.size();
}else if(index == 0){
    vector = {this->_symbol.front().getPosizione().at(0)-1};
}else
    vector = generatePos(index, c);
Symbol s(c, std::make_pair(id, ++this->count), vector);

    this->_symbol.insert(this->_symbol.begin()+index, s);

    /*for (auto it = begin (_symbol); it != end (_symbol); ++it) {
        std::cout<< it->getCharacter();
    }*/

    MessageSymbol m(0, id, s, index);
    return m;

}

std::vector<int> Room::generatePos(int index, char c) {
    std::vector<int> posBefore = this->_symbol[index-1].getPosizione();
    std::vector<int> posAfter = this->_symbol[index].getPosizione();
    std::vector<int> newPos;
    int idBefore = posBefore.at(0);
    int idAfter = posAfter.at(0);
    if(idBefore-idAfter==0){
        newPos.push_back(idBefore);
        posAfter.erase(posAfter.begin());
        posBefore.erase(posBefore.begin());
        if(posAfter.empty()){
            newPos.push_back(posBefore.front()+1);
            return  newPos;
        }
    }    else if(idAfter - idBefore > 1) {
        newPos.push_back(posBefore.front()+1);
        return newPos;
    }
    else if(idAfter - idBefore == 1) {
        newPos.push_back(idBefore);
        posBefore.erase(posBefore.begin());
        if(posBefore.empty()) {
            newPos.push_back(0);
            return newPos;
        } else {
            newPos.push_back(posBefore.front()+1);
            return newPos;
        }
    }
}

MessageSymbol Room::localErase(int startIndex,int endIndex, int id){
    Symbol s = this->_symbol.at(startIndex);
    this->_symbol.erase(_symbol.begin()+startIndex,this->_symbol.begin()+endIndex);
    MessageSymbol m(1, id, s);
    return m;
}
