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

    for(const auto& p: participants_){
        if(p->getId()!=partecipantId)
            p->deliver(msg);
    }
}

const std::map<std::string, std::vector<Symbol>> &Room::getRoomMap() const {
    return room_map_;
}

void Room::setRoomMap(const std::map<std::string, std::vector<Symbol>> &roomMap) {
    room_map_ = roomMap;
}

void Room::send(const MessageSymbol &m) {
    infoMsgs_.push_back(m);
}
//questo serve a mandare la stessa coda di messaggi a tutti i partecipanti
void Room::dispatchMessages() {
    while(!infoMsgs_.empty()) {
        for (const auto& it: participants_) {
            if (it->getId() != infoMsgs_.front().getId())
                it->process(infoMsgs_.front());
        }
        infoMsgs_.pop_front();
    }
}