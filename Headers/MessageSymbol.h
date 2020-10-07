//
// Created by Sam on 20/apr/2020.
//

#ifndef SERVERPDS_MESSAGESYMBOL_H
#define SERVERPDS_MESSAGESYMBOL_H

#include "Symbol.h"

class MessageSymbol {
private:
    int type; //inserimento 0, cancellazione 1
    std::string username;
    int newIndex;
    Symbol symbol;
public:
    MessageSymbol(int type, std::string usr, Symbol s);

    int getType() const;
    void setType(int type);

    std::string getUsername() const;
    void setUsername(std::string);

    Symbol getSymbol();
    void setSymbol(const Symbol &symbol);

    int getNewIndex() const;
    void setNewIndex(int newIndex);
};


#endif //SERVERPDS_MESSAGESYMBOL_H
