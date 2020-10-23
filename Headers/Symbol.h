//
// Created by Sam on 14/apr/2020.
//

#ifndef SERVERPDS_SYMBOL_H
#define SERVERPDS_SYMBOL_H

#include <string>
#include <iostream>
#include <array>
#include <vector>
#include "Style.h"

class Symbol {
private:
    wchar_t character;
    std::string username;
    std::vector<int> posizione;
public:
    Symbol(wchar_t car, std::string usr, std::vector<int> pos);
    Symbol(wchar_t car, std::string usr, std::vector<int> pos, Style style);

    Symbol();

    Style symbolStyle;

    const Style &getSymbolStyle() const;

    void setSymbolStyle(const Style &symbolStyle);

    bool operator==(const Symbol& s2);

    std::string toStdString();

    std::vector<int> getPosizione();

    void setPosizione(const std::vector<int> &pos);

    std::string getUsername();

    void setUsername(std::string usr);

    wchar_t getCharacter();

    void setCharacter(wchar_t ch);

};


#endif //SERVERPDS_SYMBOL_H
