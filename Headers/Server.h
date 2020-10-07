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

typedef std::deque<Message> message_queue;
typedef std::queue<MessageSymbol> msgInfo_queue;
typedef std::shared_ptr<Participant> participant_ptr;


class Server {
public:
    static Server &getInstance();

    Server(Server const &) = delete;

    void operator=(Server const &) = delete;

    void join(const participant_ptr &participant);

    void leave(const participant_ptr &participant);

    void deliver(const Message &msg);

    void deliverToAllOnFile(const Message &msg, const participant_ptr &participant);

    void send(const MessageSymbol &m);

    void openFile(const participant_ptr &participant);

    std::vector<int> closeFile(const participant_ptr &participant);

    bool isFileInFileSymbols(const std::string &filename);

    void insertParticipantInFile(const participant_ptr &participant);

    bool removeParticipantInFile(const std::string &filename, int id);

    std::vector<participant_ptr> getParticipantsInFile(const std::string &filename);

    MessageSymbol insertSymbolNewCRDT(int index, char character, const std::string& username, const std::string& filename);

    int generateIndexCRDT(Symbol symbol, const std::string& filename, int iter, int start, int end);

    void eraseSymbolCRDT(Symbol symbolStart, Symbol symbolEnd, const std::string& filename);

    void insertSymbolIndex(const Symbol& symbol, int index, const std::string& filename);

    std::vector<int> generatePos(int index, std::string filename);

    std::vector<int> generatePosBetween(std::vector<int> pos1, std::vector<int> pos2, std::vector<int> newPos);

    std::vector<Symbol> getSymbolsPerFile(const std::string &filename);

    void modFile(const std::string &filename, bool force);

    std::vector<std::string> getColors(const std::vector<int> &users);

    std::vector<std::string> getUsernames(const std::vector<int> &users);

    bool isParticipantIn(int id);

    bool isParticipantIn(const std::string& username);

    participant_ptr  getParticipant(const std::string &username);

    int getOutputcount();

private:
    Server() = default;

    ~Server();

    std::set<participant_ptr> participants_;
    enum {
        max_recent_msgs = 100
    };
    message_queue recent_msgs_;
    msgInfo_queue infoMsgs_;
    unsigned int countOutput = 1;

    std::map<std::string, std::vector<Symbol>> symbolsPerFile;
    std::map<std::string, std::vector<participant_ptr>> participantsPerFile;
    std::map<std::string, int> modsPerFile;
};


#endif //SERVERPDS_SERVER_H
