//
// Created by Sam on 01/apr/2020.
//

#ifndef SERVERPDS_MANAGEMENTDB_H
#define SERVERPDS_MANAGEMENTDB_H

#include <QtSql>
#include <QtCore/QRegExp>
#include <QRegExp>
#include <string>
#include <iostream>
#include <QString>
#include "lib/md5.h"

class ManagementDB {
private:
    QSqlDatabase connect();
    std::string response;
    std::string checkMail(const QString mail);
    QString generateRandomColor();
public:
    std::string handleLogin(const std::string user, const std::string password, QString& color);
    std::string handleSignup(const std::string email,const std::string user,const std::string password);
    std::string handleNewFile(const std::string user, const std::string file );
    std::list<std::string> takeFiles(const std::string user);
    std::string handleOpenFile(const std::string user, const std::string file);

};


#endif //SERVERPDS_MANAGEMENTDB_H
