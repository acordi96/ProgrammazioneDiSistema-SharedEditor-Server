//
// Created by Sam on 19/apr/2020.
//

#ifndef SERVERPDS_SERVER_H
#define SERVERPDS_SERVER_H

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


class Server {
public:
    static Server &getInstance();

    Server(Server const &) = delete;

    void operator=(Server const &) = delete;

    void join(const participant_ptr &participant);

    void leave(const participant_ptr &participant);

    void deliver(const message &msg);

    void deliverToAllOnFile(const message &msg, const participant_ptr& participant);

    void send(const MessageSymbol &m);

    void openFile(const participant_ptr& participant);

    std::vector<int> closeFile(const participant_ptr& participant);

    bool isFileInFileSymbols(const std::string &filename);

    void insertParticipantInFile(const participant_ptr &participant);

    bool removeParticipantInFile(const std::string& filename, int id);

    void removeParticipant(const participant_ptr& participant);

    std::vector<participant_ptr> getParticipantsInFile(const std::string& filename);

    MessageSymbol insertSymbol(const std::string &filename, int index, char character, int id);

    MessageSymbol eraseSymbol(const std::string &filename, int startIndex, int endIndex, int id);

    std::vector<int> generateNewPosition(const std::string &, int);

    std::vector<Symbol> getSymbolsPerFile(const std::string &filename);

    void modFile(const std::string& filename, bool force);

    std::vector<std::string> getColors(const std::vector<int>& users);

private:
    Server() = default;

    ~Server();

    std::set<participant_ptr> participants_;
    enum {
        max_recent_msgs = 100
    };
    message_queue recent_msgs_;
    msgInfo_queue infoMsgs_;
    std::map<std::string, std::vector<Symbol>> symbolsPerFile;
    std::map<std::string, std::vector<participant_ptr>> participantsPerFile;
    std::map<std::string, int> modsPerFile;
};


#endif //SERVERPDS_SERVER_H
