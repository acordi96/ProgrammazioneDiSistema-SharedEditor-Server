//
// Created by Sam on 01/apr/2020.
//

#include "ManagementDB.h"

QSqlDatabase ManagementDB::connect() { //TODO: database and multithreading
    /*if (this->database.databaseName() == "login") {
        return this->database;
    }*/
    this->database = QSqlDatabase::addDatabase("QMYSQL");
    this->database.setHostName("localhost");
    this->database.setUserName("root");
    this->database.setPassword("");
    this->database.setDatabaseName("login");
    return this->database;
}

QString ManagementDB::generateRandomColor() {
    std::string hex = "abcdef0123456789";
    int n = hex.length();
    std::string color = "#88"; //alpha will be fixed to 88
    for (int i = 1; i <= 6; i++) {
        std::srand(std::time(nullptr));
        color.push_back(hex[std::rand() % n]);
    }
    return (QString::fromUtf8(color.data(), color.size()));
}

std::string ManagementDB::handleLogin(const std::string &user, const std::string &password, QString &color) {
    QSqlDatabase db = connect();
    if (db.open()) {
        QSqlQuery query;
        QString username = QString::fromUtf8(user.data(), user.size());
        query.prepare("SELECT sale FROM user_login WHERE username = '"+username+"'");
        query.exec();
        query.next();
        QString sale = query.value(0).toString();
        std::string saltedPassword = md5(password+sale.toUtf8().constData());
        QString psw = QString::fromUtf8(saltedPassword.data(), saltedPassword.size());
        query.prepare(
                "SELECT username, password, color FROM user_login WHERE username='" + username + "' and password='" +
                psw + "'");
        if (query.exec()) {
            if (query.next()) {
                color = query.value(2).toString();
                response = "LOGIN_SUCCESS";
            } else
                response = "LOGIN_ERROR";
        } else {
            response = "QUERY_ERROR";
        }
        db.close();
        return response;
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
                QString qsale = QString::fromStdString(sale.erase(6));
                std::string saltedPassword = psw + sale;
                saltedPassword = md5(saltedPassword);
                QString password = QString::fromStdString(saltedPassword);
                QSqlQuery query2;
                query2.prepare(
                        "INSERT INTO user_login(email, username, password, color, sale) VALUES ('" + email + "', '" +
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

std::string ManagementDB::handleOpenFile(const std::string &user, const std::string &file) {
    QSqlDatabase db = connect();

    if (db.open()) {
        QSqlQuery query;
        QString id = QString::fromUtf8(user.data(), user.size());
        QString title = QString::fromUtf8(file.data(), file.size());
        query.prepare("SELECT COUNT(1) FROM files WHERE username = '" + id + "' AND titolo = '" + title +
                      "'");
        if (query.exec()) {
            query.next();
            if (query.value(0) == 1) {
                db.close();
                return "FILE_OPEN_SUCCESS";
            }
        }
        db.close();
        std::cout << "apertura fallita" << std::endl;
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
        invitation.erase(12);
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

/*std::list<std::string>*/
std::multimap<std::string, std::string> ManagementDB::takeFiles(const std::string &user) {
    QSqlDatabase db = connect();
    if (db.open()) {
        QSqlQuery query;
        QString quser = QString::fromUtf8(user.data(), user.size());
        //query.prepare("SELECT username, titolo FROM files"); //SENZA GERARCHIA
        query.prepare("SELECT owner, titolo FROM files where username = '" + quser + "'"); //CON GERARCHIA
        if (query.exec()) {
            std::multimap<std::string, std::string> files;
            while (query.next()) {
                QString username = query.value(0).toString();
                QString title = query.value(1).toString();
                std::string utente = username.toStdString();
                std::string titolo = title.toStdString();
                files.insert({utente, titolo});
            }
            db.close();
            return files;
        }
    }
    return std::multimap<std::string, std::string>();
}

std::string
ManagementDB::handleRenameFile(const std::string &user, const std::string &oldName, const std::string &newName) {
    QSqlDatabase db = connect();

    if (db.open()) {
        QSqlQuery query;
        QString id = QString::fromUtf8(user.data(), user.size());
        QString vecchio = QString::fromUtf8(oldName.data(), oldName.size());
        QString nuovo = QString::fromUtf8(newName.data(), newName.size());
        query.prepare(
                "UPDATE files SET titolo = '" + nuovo + "' WHERE username ='" + id + "' and titolo = '" + vecchio +
                "'");
        if (query.exec()) {
            //la query dovrebbe ritornare il file

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

std::string ManagementDB::getInvitation(const std::string &user, const std::string &owner, const std::string &file) {
    QSqlDatabase db = connect();

    if (db.open()) {
        QSqlQuery query;
        QString quser = QString::fromUtf8(user.data(), user.size());
        QString qowner = QString::fromUtf8(owner.data(), file.size());
        QString qfile = QString::fromUtf8(file.data(), file.size());
        query.prepare(
                "SELECT invitation FROM files WHERE username = '" + quser + "' AND titolo = '" + qfile +
                "' AND owner = '" + qowner + "'");
        if (query.exec()) {
            db.close();
            return query.value(0).toString().toStdString();
        } else {
            db.close();
            return "QUERY_REFUSED";
        }

    } else
        return "CONNESSION_ERROR_";
}

std::string ManagementDB::validateInvitation(const std::string &user, const std::string &owner, const std::string &file,
                                             const std::string &code) {
    QSqlDatabase db = connect();
    if (db.open()) {
        QSqlQuery query;
        QString quser = QString::fromUtf8(user.data(), user.size());
        QString qowner = QString::fromUtf8(owner.data(), owner.size());
        QString qfile = QString::fromUtf8(file.data(), file.size());
        QString qcode = QString::fromUtf8(code.data(), code.size());

        query.prepare(
                "SELECT COUNT(1) FROM files WHERE owner ='" + qowner + "' and titolo ='" +
                qfile + "' AND invitation = '" + qcode + "'");
        if (query.exec()) {
            query.next();
            if (query.value(0) == 1) {
                query.prepare(
                        "INSERT INTO files (username, titolo, invitation, owner) VALUES ('" + quser + "', '" + qfile +
                        "', '/', " + qowner + "')");
                if (!query.exec())
                    response = "INVITATION_ERROR";
                response = "INVITATION_SUCCESS";
            } else
                response = "INVITATION_ERROR";
        } else {
            response = "QUERY_ERROR";
        }
        db.close();
        return response;
    } else {
        return "CONNESSION_ERROR";
    }
}
