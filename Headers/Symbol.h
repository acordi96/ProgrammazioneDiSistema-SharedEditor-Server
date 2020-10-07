//
// Created by Sam on 14/apr/2020.
//

#ifndef SERVERPDS_SYMBOL_H
#define SERVERPDS_SYMBOL_H

#include <string>
#include <iostream>
#include <array>
#include <vector>

class Symbol {
private:
    char character;
    std::string username;
    std::vector<int> posizione;
public:
    Symbol(char car, std::string usr, std::vector<int> pos);
    Symbol();

    bool operator==(const Symbol& s2);

    void operator=(const Symbol& s2);

    std::string toStdString();

    std::vector<int> getPosizione();

    void setPosizione(const std::vector<int> &pos);

    std::string getUsername();

    void setUsername(std::string usr);

    char getCharacter();

    void setCharacter(char ch);

};


#endif //SERVERPDS_SYMBOL_H
