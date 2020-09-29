//
// Created by Sam on 14/apr/2020.
//

#include "Headers/Symbol.h"

#include <utility>

Symbol::Symbol(char car, std::pair<int, int> id, std::vector<int> pos) : character(car), idUser(std::move(id)),
                                                                         posizione(std::move(pos)) {

}

char Symbol::getCharacter() const {
    return character;
}

void Symbol::setCharacter(char character) {
    Symbol::character = character;
}

const std::pair<int, int> &Symbol::getIdUser() const {
    return idUser;
}

void Symbol::setIdUser(const std::pair<int, int> &idUser) {
    Symbol::idUser = idUser;
}

const std::vector<int> &Symbol::getPosizione() const {
    return posizione;
}

void Symbol::setPosizione(const std::vector<int> &posizione) {
    Symbol::posizione = posizione;
}
