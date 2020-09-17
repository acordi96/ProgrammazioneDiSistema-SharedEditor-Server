//
// Created by Sam on 19/apr/2020.
//

#ifndef SERVERPDS_ROOM_H
#define SERVERPDS_ROOM_H

#include <memory>
#include <set>
#include <list>
#include <deque>
#include <queue>
#include "Participant.h"
#include <map>
typedef std::deque<message> message_queue;
typedef std::queue<MessageSymbol> msgInfo_queue;
typedef std::shared_ptr<Participant> participant_ptr;


class Room {
public:
    void join(participant_ptr participant);
    void leave(participant_ptr participant);
    void deliver(const message& msg);
    void deliverToAll(const message& msg, const int& partecipantId);
    void send(const MessageSymbol& m);
    void dispatchMessages();

    //TODO: struttura per tenere i file aperti (non funge)
    //std::map<std::string, std::ofstream *> files;
    const std::map<std::string, std::vector<Symbol>> &getRoomMap() const;

    void setRoomMap(const std::map<std::string, std::vector<Symbol>> &roomMap);
    std::set<participant_ptr> participants_;

    std::map<std::string, Room*> roomPerFile;

    //presi da participant
    int count = 0;
    std::vector<Symbol> _symbol; //TODO: uno per user????
    std::vector<int> generatePos(int index, char c);
    int comparePos(std::vector<int> currVetPos, std::vector<int> newVetPos, int index);
    MessageSymbol localInsert(int index, char c, int id);
    MessageSymbol localErase(int startIndex,int endIndex, int id);
private:
    enum { max_recent_msgs = 100 };
    message_queue recent_msgs_;
    msgInfo_queue infoMsgs_;
    std::map<std::string, std::vector<Symbol>> room_map_;
};


#endif //SERVERPDS_ROOM_H
