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
    //contient l'identificativo del client e il contatore con il numero di inerimenti fatti
    std::pair<int, int> idUser;
    std::vector<int> posizione;
public:
    Symbol(char car, std::pair<int, int> id, std::vector<int> pos);

    const std::vector<int> &getPosizione() const;
    void setPosizione(const std::vector<int> &posizione);

    const std::pair<int, int> &getIdUser() const;
    void setIdUser(const std::pair<int, int> &idUser);

    char getCharacter() const;
    void setCharacter(char character);

};


#endif //SERVERPDS_SYMBOL_H
