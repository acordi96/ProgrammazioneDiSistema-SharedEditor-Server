//
// Created by Sam on 20/apr/2020.
//

#include "Headers/MessageSymbol.h"
MessageSymbol::MessageSymbol(int type, Symbol s) : type(type), symbol(s){

}
MessageSymbol::MessageSymbol(int type, int partecipantId, Symbol s) : type(type), partecipantId(partecipantId), symbol(s) {}

MessageSymbol::MessageSymbol(int type, int partecipantId, Symbol s, int newIndex) : type(type), partecipantId(partecipantId), symbol(s), newIndex(newIndex){}
int MessageSymbol::getType() const {
    return type;
}

void MessageSymbol::setType(int type) {
    MessageSymbol::type = type;
}

const Symbol &MessageSymbol::getSymbol() const {
    return symbol;
}

void MessageSymbol::setSymbol(const Symbol &symbol) {
    MessageSymbol::symbol = symbol;
}

int MessageSymbol::getId() const {
    return partecipantId;
}

void MessageSymbol::setId(int id) {
    MessageSymbol::partecipantId = id;
}

int MessageSymbol::getNewIndex() const {
    return newIndex;
}

void MessageSymbol::setNewIndex(int newIndex) {
    MessageSymbol::newIndex = newIndex;
}
