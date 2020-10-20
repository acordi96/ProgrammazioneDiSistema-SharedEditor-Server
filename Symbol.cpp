//
// Created by Sam on 14/apr/2020.
//

#include "Headers/Symbol.h"

#include <utility>

Symbol::Symbol(wchar_t car, std::string usr, std::vector<int> pos) : character(car), username(std::move(usr)),
                                                                  posizione(std::move(pos)) {
    this->symbolStyle = Style();
}

Symbol::Symbol(wchar_t car, std::string usr, std::vector<int> pos, Style style) : character(car), username(std::move(usr)),
                                                                               posizione(std::move(pos)), symbolStyle(std::move(style)) {

}

Symbol::Symbol() {
    Symbol::character = '\0';
    Symbol::username = "";
    Symbol::posizione = std::vector<int>();
}

bool Symbol::operator==(const Symbol &s2) {
    if (this->posizione == s2.posizione && this->username == s2.username && this->character == s2.character)
        return true;
    return false;
}

std::string Symbol::toStdString() {
    std::string out;
    out += "[" + Symbol::username + ", " += Symbol::character + "' ";
    for(auto &i : Symbol::posizione)
        out += std::to_string(i);
    out += "]";
    return out;
}

wchar_t Symbol::getCharacter() {
    return character;
}

void Symbol::setCharacter(wchar_t ch) {
    this->character = ch;
}

std::string Symbol::getUsername() {
    return username;
}

void Symbol::setUsername(std::string usr) {
    this->username = usr;
}

std::vector<int> Symbol::getPosizione() {
    return posizione;
}

void Symbol::setPosizione(const std::vector<int> &pos) {
    this->posizione = pos;
}

const Style &Symbol::getSymbolStyle() const {
    return symbolStyle;
}

void Symbol::setSymbolStyle(const Style &symbolStyle) {
    Symbol::symbolStyle = symbolStyle;
}
