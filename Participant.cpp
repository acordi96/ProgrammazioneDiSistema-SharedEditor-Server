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
MessageSymbol Participant::localInsert(int index, char c) {
    std::vector<int> vector;
    if(_symbol.empty()){
        vector = {0};
        index = 0;
    }else if(index > _symbol.size()-1){
        vector = {_symbol.back().getPosizione().at(0)+1};
        index = _symbol.size();
    }else if(index == 0){
        vector = {_symbol.front().getPosizione().at(0)-1};
    }else
        vector = generatePos(index, c);
    Symbol s(c, std::make_pair(_siteId, ++count), vector);

    _symbol.insert(_symbol.begin()+index, s);

    for (auto it = begin (_symbol); it != end (_symbol); ++it) {
        std::cout<< it->getCharacter();
    }

    MessageSymbol m(0, getId(), s, index);
    return m;

}
std::vector<int> Participant::generatePos(int index, char c) {
    std::vector<int> posBefore = _symbol[index-1].getPosizione();
    std::vector<int> posAfter = _symbol[index].getPosizione();
    std::vector<int> newPos;
    int idBefore = posBefore.at(0);
    int idAfter = posAfter.at(0);
    if(idBefore-idAfter==0){
        newPos.push_back(idBefore);
        posAfter.erase(posAfter.begin());
        posBefore.erase(posBefore.begin());
        if(posAfter.empty()){
            newPos.push_back(posBefore.front()+1);
            return  newPos;
        }
    }    else if(idAfter - idBefore > 1) {
        newPos.push_back(posBefore.front()+1);
        return newPos;
    }
    else if(idAfter - idBefore == 1) {
        newPos.push_back(idBefore);
        posBefore.erase(posBefore.begin());
        if(posBefore.empty()) {
            newPos.push_back(0);
            return newPos;
        } else {
            newPos.push_back(posBefore.front()+1);
            return newPos;
        }
    }
}
MessageSymbol Participant::localErase(int startIndex,int endIndex){
    Symbol s = _symbol.at(startIndex);
    _symbol.erase(_symbol.begin()+startIndex,_symbol.begin()+endIndex);
    MessageSymbol m(1, getId(), s);
    return m;
}
const std::vector<Symbol> &Participant::getSymbol() const {
    return _symbol;
}

void Participant::setSymbol(const std::vector<Symbol> &symbol) {
    _symbol = symbol;
}

int Participant::comparePos(std::vector<int> currVetPos, std::vector<int> newVetPos, int index) {
    if(currVetPos.at(index) > newVetPos.at(index))
        return 1;
    else if(currVetPos.at(index) == newVetPos.at(index))
        if(newVetPos.size()>index-1 && currVetPos.size()<=index+1)
            return -1;
        else if(newVetPos.size() <= index+1 && currVetPos.size()>index+1)
            return 1;
        else if(newVetPos.size()>index+1 && currVetPos.size()>index+1)
            return comparePos(currVetPos, newVetPos, index+1);
}

void Participant::process(const MessageSymbol &m) {
    auto type = m.getType();
    auto index = m.getNewIndex();
    if(type == 0){
        /* INSERT */
        //caso di inserimento
        //int myIndex = _symbol.size();
        _symbol.insert(_symbol.begin()+index, m.getSymbol());

    }else if(type == 1){
        /* REMOVE */
        //int myIndex = _symbol.size();
        //_symbol.erase(_symbol.begin()+myIndex);
        for(auto it=_symbol.begin();it != _symbol.end(); it++){
            if(it->getPosizione()==m.getSymbol().getPosizione()){
                _symbol.erase(it);
                break;
            }
        }
    }
}

const std::string &Participant::getColor() const {
    return color;
}

void Participant::setColor(const std::string &color) {
    Participant::color = color;
}
