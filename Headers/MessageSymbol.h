//
// Created by Sam on 20/apr/2020.
//

#ifndef SERVERPDS_MESSAGESYMBOL_H
#define SERVERPDS_MESSAGESYMBOL_H

#include "Symbol.h"

class MessageSymbol {
private:
    int type; //inserimento,ccancellazione
    int partecipantId; //id del tipo
    int newIndex;
    Symbol symbol;
public:
    MessageSymbol(int type, int partecipantId, Symbol s);
    MessageSymbol(int type, Symbol s);
    MessageSymbol(int type, int partecipantId, Symbol s, int newIndex);

    int getType() const;
    void setType(int type);

    int getId() const;
    void setId(int id);

    const Symbol &getSymbol() const;
    void setSymbol(const Symbol &symbol);

    int getNewIndex() const;
    void setNewIndex(int newIndex);
};


#endif //SERVERPDS_MESSAGESYMBOL_H
