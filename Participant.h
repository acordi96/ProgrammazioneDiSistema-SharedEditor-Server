//
// Created by Sam on 19/apr/2020.
//

#ifndef SERVERPDS_PARTICIPANT_H
#define SERVERPDS_PARTICIPANT_H

#include "message.h"
#include "MessageSymbol.h"
class Participant {
private:
    int _siteId;
    int count = 0;
    std::string currentFile;
    std::string username;
    std::string color;
    std::vector<Symbol> _symbol;
    std::vector<int> generatePos(int index, char c);
    int comparePos(std::vector<int> currVetPos, std::vector<int> newVetPos, int index);
public:
    virtual ~Participant() {}
    virtual void deliver(const message& msg) = 0;
    //metodi set
    void setUsername(std::string userName);
    void setSiteId(int edId);
    void setCurrentFile(const std::string &currentFile);
    void setSymbol(const std::vector<Symbol> &symbol);

    const std::string &getColor() const;

    void setColor(const std::string &color);

    //metodi get
    const std::vector<Symbol> &getSymbol() const;
    std::string getUsername();
    int getId() const;
    const std::string &getCurrentFile() const;
    //metodi per la concorrenza
    MessageSymbol localInsert(int index, char c);
    MessageSymbol localErase(int startIndex,int endIndex);
    void process(const MessageSymbol &m);
};


#endif //SERVERPDS_PARTICIPANT_H
