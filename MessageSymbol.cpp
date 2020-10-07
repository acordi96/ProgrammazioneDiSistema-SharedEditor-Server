//
// Created by Sam on 20/apr/2020.
//

#include "Headers/MessageSymbol.h"

MessageSymbol::MessageSymbol(int t, std::string usr, Symbol s) : type(t), username(usr), symbol(s) {}

int MessageSymbol::getType() const {
    return type;
}

void MessageSymbol::setType(int type) {
    MessageSymbol::type = type;
}

Symbol MessageSymbol::getSymbol() {
    return symbol;
}

void MessageSymbol::setSymbol(const Symbol &symbol) {
    MessageSymbol::symbol = symbol;
}

std::string MessageSymbol::getUsername() const {
    return username;
}

void MessageSymbol::setUsername(std::string usr) {
    MessageSymbol::username = usr;
}

int MessageSymbol::getNewIndex() const {
    return newIndex;
}

void MessageSymbol::setNewIndex(int newIndex) {
    MessageSymbol::newIndex = newIndex;
}
