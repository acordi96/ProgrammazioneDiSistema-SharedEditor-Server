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
#include "../Libs/md5.h"

class ManagementDB {
private:
    ManagementDB() = default;

    ~ManagementDB() = default;

    QSqlDatabase connect();

    std::string response;
    QSqlDatabase database;

    static std::string checkMail(const QString &mail);

    static QString generateRandomColor();

public:
    static ManagementDB &getInstance();

    ManagementDB(ManagementDB const &) = delete;

    void operator=(ManagementDB const &) = delete;

    std::string handleLogin(const std::string &user, const std::string &password, QString &color, QString &email);

    std::string handleSignup(const std::string &email, const std::string &user, const std::string &password);

    std::string handleNewFile(const std::string &user, const std::string &file);

    std::multimap<std::pair<std::string, std::string>, std::string> takeFiles(const std::string &user);

    std::string handleOpenFile(const std::string &owner, const std::string &user, const std::string &file);

    std::string handleRenameFile(const std::string &user, const std::string &oldName, const std::string &newName, std::vector<std::string> &invited);

    std::string handleDeleteFile(const std::string &user, const std::string &name, const std::string &owner, std::vector<std::string> &invited);

    std::pair<std::string, std::string> validateInvitation(const std::string &user, const std::string &code);

    std::string getInvitation(const std::string& owner, const std::string& filename);

    std::string handleEditProfile(const std::string &user,const std::string &oldUser,const std::string &email,const std::string &newPassword,const std::string &oldPassword);
};


#endif //SERVERPDS_MANAGEMENTDB_H
