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
public:
    virtual ~Participant() = default;

    virtual void deliver(const message &msg) = 0;

    //metodi set
    void setUsername(std::string userName);

    void setSiteId(int edId);

    void setCurrentFile(const std::string &currentFile);

    const std::string &getColor() const;

    void setColor(const std::string &color);

    //metodi get
    std::string getUsername();

    int getId() const;

    const std::string &getCurrentFile() const;
};


#endif //SERVERPDS_PARTICIPANT_H
