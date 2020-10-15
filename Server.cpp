//
// Created by Sam on 19/apr/2020.
//

#include <fstream>
#include <boost/filesystem.hpp>
#include "Headers/Server.h"
#include "Headers/SocketManager.h"

#define nModsBeforeWrite 150 //numero di modifiche prima di modificare il file (>0)

Server::~Server() {
    std::vector<std::string> openFiles;
    for (auto &pair : this->symbolsPerFile)
        openFiles.push_back(pair.first);
    for (auto &filename : openFiles)
        this->modFile(filename, true);
}

Server &Server::getInstance() {
    static Server instance;
    return instance;
}

/* ########################################### PARTICIPANT MANAGEMENT ########################################## */

void Server::join(const participant_ptr &participant) {
    participants_.insert(participant);
    std::cout << SocketManager::output() << "PARTICIPANTS LIST UPDATED ("
              << std::to_string(participants_.size())
              << "): ";
    for (auto &p : participants_) {
        std::cout << "[ID: " << std::to_string(p->getId()) << ", USERNAME: ";
        if (p->getUsername().empty())
            std::cout << "''";
        else
            std::cout << p->getUsername();
        std::cout << ", FILE: ";
        if (p->getCurrentFile().empty())
            std::cout << "(NO)], ";
        else
            std::cout << "(YES)], ";
    }
    std::cout << std::endl;
}

void Server::leave(const participant_ptr &participant) {
    for (auto &p : participants_) {
        if (p == participant) {
            participants_.erase(p);
            std::cout << SocketManager::output() << "PARTICIPANTS LIST UPDATED ("
                      << std::to_string(participants_.size())
                      << "): ";
            for (auto &pa : participants_) {
                std::cout << "[ID: " << std::to_string(p->getId()) << ", USERNAME: ";
                if (pa->getUsername().empty())
                    std::cout << "''";
                else
                    std::cout << pa->getUsername();
                std::cout << ", FILE: ";
                if (pa->getCurrentFile().empty())
                    std::cout << "(NO)], ";
                else
                    std::cout << "(YES)], ";
            }
            std::cout << std::endl;
            return;
        }
    }
}

std::vector<std::string> Server::getColors(const std::vector<std::string> &usernames) {
    std::vector<std::string> colors;
    for (auto &user : usernames) {
        for (auto &participant : participants_) {
            if (participant->getUsername() == user) {
                colors.emplace_back(participant->getColor());
            }
        }
    }
    return colors;
}

bool Server::isParticipantIn(int id) {
    for (auto &participant : participants_)
        if (participant->getId() == id)
            return true;
    return false;
}

bool Server::isParticipantIn(const std::string &username) {
    for (auto &participant : participants_)
        if (participant->getUsername() == username)
            return true;
    return false;
}

participant_ptr Server::getParticipant(const std::string &username) {
    for (auto &participant : participants_) {
        if (participant->getUsername() == username)
            return participant;
    }
    return nullptr;
}

unsigned int Server::getOutputcount() {
    return this->countOutput++;
}

void Server::printCRDT(const std::string &filename) {
    std::cout << SocketManager::output() << "FILE CRDT: " << std::flush; //print crdt
    for (auto iterPositions = this->symbolsPerFile.at(filename).begin();
         iterPositions != this->symbolsPerFile.at(filename).end(); ++iterPositions) {
        if (iterPositions->getCharacter() != 10 && iterPositions->getCharacter() != 13)
            std::cout << "[" << (int) iterPositions->getCharacter() << "(" << iterPositions->getCharacter()
                      << ") - " << std::flush;
        else
            std::cout << "[" << (int) iterPositions->getCharacter() << "(\\n) - " << std::flush;
        for (int i = 0; i < iterPositions->getPosizione().size(); i++)
            std::cout << std::to_string(iterPositions->getPosizione()[i]) << std::flush;
        std::cout << "]" << std::flush;
    }
    std::cout << std::endl;

}

/* ########################################### COMMUNICATION ########################################## */

void Server::deliver(const Message &msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    for (const auto &p: participants_)
        p->deliver(msg);
}

void Server::deliverToAllOtherOnFile(const Message &msg, const participant_ptr &participant) {
    recent_msgs_.push_back(msg);

    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    //non a tutti ma a tutti su quel file
    for (const auto &username: this->usernamePerFile.at(participant->getCurrentFile())) {
        if (username != participant->getUsername()) {
            this->getParticipant(username)->deliver(msg);
        }
    }
}

void Server::deliverToAllOnFile(const Message &msg, const participant_ptr &participant) {
    recent_msgs_.push_back(msg);

    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();

    //non a tutti ma a tutti su quel file
    for (const auto &username: this->usernamePerFile.at(participant->getCurrentFile())) {
        this->getParticipant(username)->deliver(msg);
    }
}

void Server::send(const MessageSymbol &m) {
    infoMsgs_.push(m);
}

/* ########################################### FILE MANAGEMENT ######################################## */

void Server::openFile(const std::string &filename, const std::string &username) {
    //inserisco entry in mappa dei simboli
    this->symbolsPerFile.emplace(
            std::pair<std::string, std::vector<Symbol>>(filename, std::vector<Symbol>()));
    //inserisco entry in mappa delle modifiche
    this->modsPerFile.emplace(std::pair<std::string, int>(filename, 0));
    //aggiungo participant a lista participants del file
    Server::getInstance().insertUsernameInFile(filename, username);
}

bool Server::isFileInFileSymbols(const std::string &filename) {
    if (this->symbolsPerFile.find(filename) == this->symbolsPerFile.end())
        return false;
    return true;
}

void Server::insertUsernameInFile(const std::string &filename, const std::string &username) {
    if (this->usernamePerFile.find(filename) == this->usernamePerFile.end()) {
        std::vector<std::string> newVector;
        newVector.push_back(username);
        this->usernamePerFile.insert(std::make_pair(filename, newVector));
    } else {
        for (auto &usr : this->usernamePerFile.at(filename))
            if (usr == username)
                return;
        this->usernamePerFile.at(filename).push_back(username);
    }
}

std::vector<Symbol> Server::getSymbolsPerFile(const std::string &filename) {
    return this->symbolsPerFile.at(filename);
}

std::vector<std::string> Server::getUsernamesInFile(const std::string &filename) {
    return this->usernamePerFile.at(filename);
}

bool Server::removeUsernameFromFile(const std::string &filename, const std::string &username) {
    for (auto it = this->usernamePerFile.at(filename).begin();
         it != this->usernamePerFile.at(filename).end(); ++it) {
        if (*it == username) {
            this->usernamePerFile.at(filename).erase(it);
            break;
        }
    }
    if (this->usernamePerFile.at(filename).empty()) {    //se era l'unico/ultimo sul file
        //forza scrittura su file
        this->modFile(filename, true);
        //dealloca strutture
        this->usernamePerFile.erase(filename);
        this->symbolsPerFile.erase(filename);
        this->modsPerFile.erase(filename);
        return true;
    }
    return false;
}

void Server::modFile(const std::string &filename, bool force) {
    //std::locale::global(std::locale(""));
    if (!force)
        this->modsPerFile.at(filename)++;
    if (this->modsPerFile.at(filename) >= nModsBeforeWrite || force) {
        char toWriteReadable[this->symbolsPerFile.at(filename).size()];
        std::ofstream file;
        std::ofstream fileReadable;
        file.open(filename);
        std::vector<int> forcedCRDT;
        json j;
        for (int iter = 0; iter != this->symbolsPerFile.at(filename).size(); iter++) {
            if (force) {
                forcedCRDT.clear();
                forcedCRDT.push_back(iter);
                j = {
                        {"character", this->symbolsPerFile.at(filename)[iter].getCharacter()},
                        {"username",  this->symbolsPerFile.at(filename)[iter].getUsername()},
                        {"posizione", forcedCRDT}
                };
            } else {
                j = {
                        {"character", this->symbolsPerFile.at(filename)[iter].getCharacter()},
                        {"username",  this->symbolsPerFile.at(filename)[iter].getUsername()},
                        {"posizione", this->symbolsPerFile.at(filename)[iter].getPosizione()}
                };
            }
            std::string linej = j.dump();
            if (iter != this->symbolsPerFile.at(filename).size() - 1)
                linej += "\n";
            const char *line = linej.c_str();
            file.write(line, linej.size());
            toWriteReadable[iter] = this->symbolsPerFile.at(filename)[iter].getCharacter();
        }
        file.close();
        std::string filenameReadable = filename;
        filenameReadable.erase(filename.size() - 4);
        filenameReadable += "_readable.txt";
        fileReadable.open(filenameReadable);
        fileReadable.write(toWriteReadable, this->symbolsPerFile.at(filename).size());
        //fileReadable<<toWriteReadable;
        fileReadable.close();
        std::cout << SocketManager::output() << "UPDATED" << " (" << this->modsPerFile.at(filename) << " MODS)"
                  << " LOCAL FILE: " << filename << std::endl;
        this->modsPerFile.at(filename) = 0;
    }
}

std::vector<std::string> Server::closeFile(const std::string &filename, const std::string &username) {
    std::vector<std::string> othersOnFile;
    if (this->removeUsernameFromFile(filename, username)) { //era l'ultimo sul file
        return othersOnFile;
    } else {
//ci sono altri sul file, li informo della mia uscita
        othersOnFile = Server::getInstance().getUsernamesInFile(filename);
        return othersOnFile;
    }
}

/* ########################################### CRDT METHODS ########################################### */

//genera per un simbolo (con crdt) la posizione all'interno di un vettore di simboli
int Server::generateIndexCRDT(Symbol symbol, const std::string &filename, int iter, int start, int end) {
    if (start == -1 && end == -1) {
        if (this->symbolsPerFile.at(filename).empty())
            return 0;
        if (symbol.getPosizione()[0] < this->symbolsPerFile.at(filename)[0].getPosizione()[0])
            return 0;
        start = 0;
        end = this->symbolsPerFile.at(filename).size();
    }
    if (start == end) {
        return iter;
    }
    int newStart = -1;
    int newEnd = start;
    for (auto iterPositions = this->symbolsPerFile.at(filename).begin() + start;
         iterPositions != this->symbolsPerFile.at(filename).begin() + end; ++iterPositions) {
        if (iterPositions->getPosizione().size() > iter && symbol.getPosizione().size() > iter) {
            if (iterPositions->getPosizione()[iter] == symbol.getPosizione()[iter] && newStart == -1)
                newStart = newEnd;
            if (iterPositions->getPosizione()[iter] > symbol.getPosizione()[iter]) {
                if (newStart == -1)
                    return newEnd;
                else
                    return generateIndexCRDT(symbol, filename, ++iter, newStart, newEnd);
            }
        }
        newEnd++;
    }
    return newEnd;
}

//inserisce in un vettore di simoli un simbolo ad una certa posizione posizione
void Server::insertSymbolIndex(const Symbol &symbol, int index, const std::string &filename) {
    int i = 0;
    for (auto iterPositions = this->symbolsPerFile.at(filename).begin();
         iterPositions != this->symbolsPerFile.at(filename).end(); ++iterPositions) {
        if (index == i) {
            this->symbolsPerFile.at(filename).insert(iterPositions, symbol);
            return;
        }
        i++;
    }
    this->symbolsPerFile.at(filename).insert(this->symbolsPerFile.at(filename).end(), symbol);
}

//cerca e cancella un simobolo da vettore di simboli
void Server::eraseSymbolCRDT(std::vector<Symbol> symbolsToErase, const std::string &filename) {
    int lastFound = 0;
    int missed = 0;
    for (auto iterSymbolsToErase = symbolsToErase.begin();
         iterSymbolsToErase != symbolsToErase.end(); ++iterSymbolsToErase) {
        int count = lastFound;
        if ((lastFound - 2) >= 0)
            lastFound -= 2;
        bool foundSecondPart = false;
        for (auto iterSymbols = this->symbolsPerFile.at(filename).begin() + lastFound; iterSymbols !=
                                                                                       this->symbolsPerFile.at(
                                                                                               filename).end(); ++iterSymbols) { //cerca prima da dove hai trovato prima in poi
            if (*iterSymbolsToErase == *iterSymbols) {
                this->symbolsPerFile.at(filename).erase(iterSymbols);
                lastFound = count;
                foundSecondPart = true;
                break;
            }
            count++;
        }
        if (!foundSecondPart) { //se non hai trovato cerca anche nella prima parte
            for (auto iterSymbols = this->symbolsPerFile.at(filename).begin();
                 iterSymbols != this->symbolsPerFile.at(filename).begin() + lastFound; ++iterSymbols) {
                if (*iterSymbolsToErase == *iterSymbols) {
                    this->symbolsPerFile.at(filename).erase(iterSymbols);
                    lastFound = count;
                    break;
                }
                count++;
            }
        }
    }
}

//genera un vettore crdt ad una data posizione in un vettore di simboli
std::vector<int> Server::generatePos(int index, const std::string &filename) {
    const std::vector<int> posBefore = symbolsPerFile.at(filename)[index - 1].getPosizione();
    const std::vector<int> posAfter = symbolsPerFile.at(filename)[index].getPosizione();
    std::vector<int> newPos;
    return generatePosBetween(posBefore, posAfter, newPos);
}

//richiamata da generatePos
std::vector<int> Server::generatePosBetween(std::vector<int> pos1, std::vector<int> pos2, std::vector<int> newPos) {
    int id1 = pos1.at(0);
    int id2 = pos2.at(0);

    if (id2 - id1 == 0) {
        newPos.push_back(id1);
        pos1.erase(pos1.begin());
        pos2.erase(pos2.begin());
        if (pos1.empty()) {
            newPos.push_back(pos2.front() - 1);
            return newPos;
        } else
            return generatePosBetween(pos1, pos2, newPos);
    } else if (id2 - id1 > 1) {
        newPos.push_back(pos1.front() + 1);
        return newPos;
    } else if (id2 - id1 == 1) {
        newPos.push_back(id1);
        pos1.erase(pos1.begin());
        if (pos1.empty()) {
            newPos.push_back(0);
            return newPos;
        } else {
            newPos.push_back(pos1.front() + 1);
            return newPos;
        }
    }
    return std::vector<int>();
}
