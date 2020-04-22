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

class ManagementDB {
private:
    QSqlDatabase connect();
    std::string response;
    std::string checkMail(const QString mail);
    std::string encPsw(const std::string password);
public:
    std::string handleLogin(const std::string user, const std::string password);
    std::string handleSignup(const std::string email,const std::string user,const std::string password);

};


#endif //SERVERPDS_MANAGEMENTDB_H
