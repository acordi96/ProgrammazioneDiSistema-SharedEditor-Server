//
// Created by Sam on 01/apr/2020.
//

#include "Headers/ManagementDB.h"

ManagementDB &ManagementDB::getInstance() {
    static ManagementDB instance;
    return instance;
}

QSqlDatabase ManagementDB::connect() {
    if (this->database.databaseName() == "login" && this->database.isValid()) {
        return this->database;
    }
    this->database = QSqlDatabase::addDatabase("QMYSQL");
    this->database.setHostName("localhost");
    this->database.setUserName("root");
    this->database.setPassword("");
    this->database.setDatabaseName("login");
    return this->database;
}

QString ManagementDB::generateRandomColor() {
    std::string color = "#";
    static const char alphanum[] =
            "0123456789ABCDEF";

    std::srand((unsigned) time(NULL)/* * getpid()*/);
    for (int i = 0; i < 6; ++i)
        color += alphanum[rand() % (sizeof(alphanum) - 1)];
    return (QString::fromUtf8(color.data(), color.size()));
}

std::string
ManagementDB::handleLogin(const std::string &user, const std::string &password, QString &color, QString &email) {
    QSqlDatabase db = connect();
    if (db.open()) {
        QSqlQuery query;
        QString username = QString::fromUtf8(user.data(), user.size());
        query.prepare("SELECT sale FROM user_login WHERE username = '" + username + "'");
        if (query.exec()) {
            if (query.next()) {
                QString sale = query.value(0).toString();
                std::string saltedPassword = md5(password + sale.toUtf8().constData());
                QString psw = QString::fromUtf8(saltedPassword.data(), saltedPassword.size());
                query.prepare(
                        "SELECT color, email FROM user_login WHERE username='" + username +
                        "' and password='" +
                        psw + "'");
                if (query.exec()) {
                    if (query.next()) {
                        color = query.value(0).toString();
                        email = query.value(1).toString();
                        db.close();
                        return "LOGIN_SUCCESS";
                    }
                }
            }
        }
        db.close();
        return "LOGIN_ERROR";
    } else {
        return "CONNESSION_ERROR";
    }
}

std::string ManagementDB::handleSignup(const std::string &e, const std::string &username, const std::string &psw) {
    QSqlDatabase db = connect();

    if (db.open()) {
        QSqlQuery query;
        QString user = QString::fromUtf8(username.data(), username.size());
        QString email = QString::fromUtf8(e.data(), e.size());

        if (checkMail(email) == "\"EMAIL_ERROR") {
            db.close();
            return "EMAIL_ERROR";
        }
        query.prepare("SELECT username FROM user_login WHERE username='" + user + "'");
        if (query.exec()) {
            if (query.next())
                return "SIGNUP_ERROR_DUPLICATE_USERNAME";
            else {
                QString color = generateRandomColor();
                std::srand(std::time(nullptr));
                std::string sale = std::to_string(std::rand());
                QString qsale = QString::fromStdString(sale);
                std::string saltedPassword = psw + sale;
                saltedPassword = md5(saltedPassword);
                QString password = QString::fromStdString(saltedPassword);
                QSqlQuery query2;
                query2.prepare(
                        "INSERT INTO user_login(email, username, password, color, sale) VALUES ('" + email +
                        "', '" +
                        user +
                        "', '" + password + "','" + color + "', '" + qsale + "')");
                if (query2.exec()) {
                    db.close();
                    return color.toStdString();
                } else {
                    db.close();
                    return "SIGNUP_ERROR_INSERT_FAILED";
                }
            }


        } else
            return "CONNESSION_ERROR";


    } else {
        return "CONNESSION_ERROR_";
    }

}

std::string ManagementDB::checkMail(const QString &mail) {
    QRegExp mailREX(R"(\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,4}\b)");
    mailREX.setCaseSensitivity(Qt::CaseInsensitive);
    mailREX.setPatternSyntax(QRegExp::RegExp);
    if (!mailREX.exactMatch(mail)) {
        return "EMAIL_ERROR";
    }
    return "EMAIL_OK";
}

std::string
ManagementDB::handleOpenFile(const std::string &owner, const std::string &user, const std::string &file) {
    QSqlDatabase db = connect();
    if (db.open()) {
        QSqlQuery query;
        QString id = QString::fromUtf8(user.data(), user.size());
        QString title = QString::fromUtf8(file.data(), file.size());
        QString qowner = QString::fromUtf8(owner.data(), owner.size());
        query.prepare("SELECT COUNT(1) FROM files WHERE username = '" + id + "' AND titolo = '" + title +
                      "' AND owner = '" + qowner + "'");
        if (query.exec()) {
            if (query.next()) {
                if (query.value(0) == 1) {
                    db.close();
                    return "FILE_OPEN_SUCCESS";
                }
            }
        }
        db.close();
        return "FILE_OPEN_FAILED";
    } else
        return "CONNESSION_ERROR_";
}

std::string ManagementDB::handleNewFile(const std::string &user, const std::string &file) {
    QSqlDatabase db = connect();

    if (db.open()) {
        QSqlQuery query;
        QString id = QString::fromUtf8(user.data(), user.size());
        QString title = QString::fromUtf8(file.data(), file.size());
        std::srand(std::time(nullptr));
        std::string invitation = user + file + std::to_string(std::rand());
        invitation = md5(invitation);
        invitation.erase(15);
        QString qinvitation = QString::fromUtf8(invitation.data(), invitation.size());
        query.prepare("INSERT INTO files(username,titolo,invitation,owner) VALUES ('" + id + "','" + title + "','" +
                      qinvitation + "','" + id + "')");

        if (query.exec()) {
            db.close();
            return "FILE_INSERT_SUCCESS";
        } else {
            db.close();
            return "FILE_INSERT_FAILED";
        }

    } else
        return "CONNESSION_ERROR_";
}

std::multimap<std::pair<std::string, std::string>, std::string> ManagementDB::takeFiles(const std::string &user) {
    QSqlDatabase db = connect();
    if (db.open()) {
        QSqlQuery query;
        QString quser = QString::fromUtf8(user.data(), user.size());
        query.prepare(
                "SELECT owner, titolo, invitation FROM files where username = '" + quser +
                "'");
        if (query.exec()) {
            std::multimap<std::pair<std::string, std::string>, std::string> files;
            while (query.next()) {
                QString owner = query.value(0).toString();
                QString filename = query.value(1).toString();
                QString invitation = query.value(2).toString();
                files.insert({std::pair<std::string, std::string>(owner.toStdString(), filename.toStdString()),
                              invitation.toStdString()});
            }
            db.close();
            return files;
        }
    }
    return std::multimap<std::pair<std::string, std::string>, std::string>();
}

std::string
ManagementDB::handleRenameFile(const std::string &user, const std::string &oldName, const std::string &newName,
                               std::vector<std::string> &invited) {
    QSqlDatabase db = connect();

    if (db.open()) {
        QSqlQuery query;
        QString id = QString::fromUtf8(user.data(), user.size());
        QString vecchio = QString::fromUtf8(oldName.data(), oldName.size());
        QString nuovo = QString::fromUtf8(newName.data(), newName.size());
        QSqlQuery query2;
        query2.prepare("SELECT username FROM files WHERE owner ='" + id + "' and titolo = '" + vecchio + "'");
        if (query2.exec()) {
            while (query2.next()) {
                invited.push_back(query2.value(0).toString().toStdString());
            }
        } else {
            db.close();
            std::cout << "\n rinomina fallita\n";
            return "FILE_DELETE_FAILED";
        }
        query.prepare(
                "UPDATE files SET titolo = '" + nuovo + "' WHERE owner ='" + id + "' and titolo = '" + vecchio +
                "'");
        if (query.exec()) {

            db.close();
            return "FILE_RENAME_SUCCESS";
        } else {
            db.close();
            std::cout << "\n rinomina fallita\n";
            return "FILE_OPEN_FAILED";
        }

    } else
        return "CONNESSION_ERROR_";
}

std::pair<std::string, std::string>
ManagementDB::validateInvitation(const std::string &user, const std::string &code) {
    QSqlDatabase db = connect();
    if (db.open()) {
        QSqlQuery query;
        QString quser = QString::fromUtf8(user.data(), user.size());
        QString qcode = QString::fromUtf8(code.data(), code.size());

        query.prepare(
                "SELECT titolo, owner FROM files WHERE invitation = '" + qcode + "'");
        if (query.exec()) {
            if (query.next()) {
                QString qfile = query.value(0).toString();
                QString qowner = query.value(1).toString();
                if (qowner == quser) {
                    db.close();
                    return std::pair<std::string, std::string>("REFUSED", "INVITATION_ERROR"); //non puoi autoinvitarti
                }
                QSqlQuery query3;
                query3.prepare(
                        "SELECT COUNT(*) FROM files WHERE username = '" + quser + "' AND titolo = '" +
                        qfile + "' AND owner = '" + qowner + "'");
                if (query3.exec()) {
                    if (query3.next()) {
                        if (query3.value(0) > 0) { //gia' invitato
                            db.close();
                            return std::pair<std::string, std::string>("REFUSED", "INVITATION_ERROR");
                        }
                    }
                }
                QSqlQuery query2;
                query2.prepare(
                        "INSERT INTO files (username, titolo, invitation, owner) VALUES ('" + quser + "', '" +
                        qfile +
                        "', '///////////////', '" + qowner + "')");
                if (query2.exec()) {
                    db.close();
                    return std::pair<std::string, std::string>(qowner.toStdString(), qfile.toStdString());
                }
            }
        }
        db.close();
        return std::pair<std::string, std::string>("REFUSED", "INVITATION_ERROR");
    } else
        return std::pair<std::string, std::string>("REFUSED", "CONNESSION_ERROR");
}

std::string ManagementDB::handleDeleteFile(const std::string &user, const std::string &name, const std::string &owner,
                                           std::vector<std::string> &invited) {
    QSqlDatabase db = connect();

    if (db.open()) {
        QSqlQuery query;
        QString qowner = QString::fromUtf8(owner.data(), owner.size());
        QString qfilename = QString::fromUtf8(name.data(), name.size());
        QString quser = QString::fromUtf8(user.data(), user.size());
        if (owner == user) { //cancella per il creatore cancella per tutti
            QSqlQuery query2;
            query2.prepare("SELECT username FROM files WHERE owner ='" + qowner + "' and titolo = '" + qfilename + "'");
            if (query2.exec()) {
                while (query2.next()) {
                    invited.push_back(query2.value(0).toString().toStdString());
                }
            } else {
                db.close();
                std::cout << "\n rinomina fallita\n";
                return "FILE_DELETE_FAILED";
            }
            query.prepare(
                    "DELETE FROM files WHERE owner ='" + qowner + "' and titolo = '" + qfilename +
                    "'");
            if (query.exec()) {
                db.close();
                return "FILE_DELETE_SUCCESS";
            } else {
                db.close();
                std::cout << "\n rinomina fallita\n";
                return "FILE_DELETE_FAILED";
            }
        } else { //cancella invito
            query.prepare(
                    "DELETE FROM files WHERE owner ='" + qowner + "' and titolo = '" + qfilename +
                    "' and username = '" + quser + "'");
            if (query.exec()) {
                db.close();
                return "FILE_DELETE_SUCCESS";
            } else {
                db.close();
                std::cout << "\n rinomina fallita\n";
                return "FILE_DELETE_FAILED";
            }
        }


    } else
        return "CONNESSION_ERROR_";


}

std::string ManagementDB::getInvitation(const std::string &owner, const std::string &filename) {
    QSqlDatabase db = connect();
    if (db.open()) {
        QSqlQuery query;
        QString qfilename = QString::fromUtf8(filename.data(), filename.size());
        QString qowner = QString::fromUtf8(owner.data(), owner.size());
        query.prepare("SELECT invitation FROM files WHERE owner = '" + qowner + "' AND titolo = '" + qfilename + "'");
        if (query.exec()) {
            if (query.next()) {
                db.close();
                return query.value(0).toString().toStdString();
            }
        }
        db.close();
        return "QUERY_FAILED";
    } else
        return "CONNESSION_ERROR_";
}

std::string
ManagementDB::handleEditProfile(const std::string &user, const std::string &email, const std::string &newPassword,
                                const std::string &oldPassword, const std::string &color) {
    QSqlDatabase db = connect();

    if (db.open()) {
        QSqlQuery query;

        QString qUser = QString::fromUtf8(user.data(), user.size());
        QString qEmail = QString::fromUtf8(email.data(), email.size());
        QString qColor = QString::fromUtf8(color.data(), color.size());
        QString qSaltedOldPassword, qSaltedNewPassword;
        std::string sale;

        //controllo corrispondenza vecchia password
        query.prepare("SELECT sale, password FROM user_login WHERE username = '" + qUser + "'");
        if (query.exec()) {
            if (query.next()) {
                sale = query.value(0).toString().toStdString();
                std::string saltedPassword = md5(oldPassword + sale);
                qSaltedOldPassword = QString::fromUtf8(saltedPassword.data(), saltedPassword.size());
                if (query.value(1).toString() != qSaltedOldPassword) {
                    return "WRONG_OLD_PASSWORD";
                }
            }
        } else {
            db.close();
            return "EDIT_FAILED";
        }
        //update email
        if (!email.empty()) {
            query.clear();
            query.prepare("UPDATE user_login SET email = '" + qEmail + "' WHERE username = '" + qUser + "'");
            if (!query.exec()) {
                db.close();
                return "EDIT_FAILED";
            }
        }
        //update password
        if (!newPassword.empty()) {
            std::string saltedNewPassword = md5(newPassword + sale);
            qSaltedNewPassword = QString::fromUtf8(saltedNewPassword.data(), saltedNewPassword.size());
            query.clear();
            query.prepare(
                    "UPDATE user_login SET password = '" + qSaltedNewPassword + "' WHERE username = '" + qUser + "'");
            if (!query.exec()) {
                db.close();
                return "EDIT_FAILED";
            }
        }
        //update color
        if (!color.empty()) {
            query.clear();
            query.prepare("UPDATE user_login SET color = '" + qColor + "' WHERE username = '" + qUser + "'");
            if (!query.exec()) {
                db.close();
                return "EDIT_FAILED";
            }
        }

        return "EDIT_SUCCESS";

    } else
        return "CONNESSION_ERROR_";
}
