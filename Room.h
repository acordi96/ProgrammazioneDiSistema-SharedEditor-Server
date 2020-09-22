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
    static Room &getInstance();

    Room(Room const &) = delete;

    void operator=(Room const &) = delete;

    void join(const participant_ptr &participant);

    void leave(const participant_ptr &participant);

    void deliver(const message &msg);

    void deliverToAllOnFile(const std::string &filename, const message &msg, const int &partecipantId);

    void send(const MessageSymbol &m);

    void insertNewFileSymbols(const std::string &);

    bool isFileInFileSymbols(const std::string &);

    void insertParticipantInFile(const std::string &filename, const participant_ptr &participant);

    bool removeParticipantInFile(const std::string& filename, int id);

    std::vector<participant_ptr> getParticipantsInFile(const std::string& filename);

    MessageSymbol insertSymbol(const std::string &, int, char, int);

    MessageSymbol eraseSymbol(const std::string &, int, int, int);

    std::vector<int> generateNewPosition(const std::string &, int);

    std::vector<Symbol> getSymbolsPerFile(const std::string &filename);

private:
    Room() = default;

    std::set<participant_ptr> participants_;
    enum {
        max_recent_msgs = 100
    };
    message_queue recent_msgs_;
    msgInfo_queue infoMsgs_;
    std::map<std::string, std::vector<Symbol>> room_map_;
    std::map<std::string, std::vector<participant_ptr>> participantsPerFile;
};


#endif //SERVERPDS_ROOM_H
