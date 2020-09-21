//
// Created by Sam on 19/apr/2020.
//

#include "Participant.h"


int Participant::getId() const {
    return _siteId;
}

void Participant::setSiteId(int edId) {
    this->_siteId = edId;
}

void Participant::setUsername(std::string userName) {
    this->username = std::move(userName);
}

std::string Participant::getUsername() {
    return this->username;
}

const std::string &Participant::getCurrentFile() const {
    return currentFile;
}

void Participant::setCurrentFile(const std::string &currentFile) {
    Participant::currentFile = currentFile;
}

const std::string &Participant::getColor() const {
    return color;
}

void Participant::setColor(const std::string &color) {
    Participant::color = color;
}
