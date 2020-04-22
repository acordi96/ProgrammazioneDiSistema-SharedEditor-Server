//
// Created by Sam on 01/apr/2020.
//

#include "ManagementDB.h"


QSqlDatabase ManagementDB::connect(){
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setUserName("root");
    db.setPassword("");
    db.setDatabaseName("login");
    return db;
}

std::string ManagementDB::handleLogin(const std::string user,const std::string password) {
    QSqlDatabase db  = connect();
    if (db.open()) {
        QSqlQuery query;
        QString username = QString::fromUtf8(user.data(), user.size());
        QString psw = QString::fromUtf8(password.data(), password.size());
        query.prepare("SELECT username, password FROM user_login WHERE username='"+username+"' and password='"+psw+"'");
        if(query.exec()) {
            if(query.next())
                response = "LOGIN_SUCCESS";
            else
                response = "LOGIN_ERROR";
        }else {
            response = "QUERY_ERROR";
        }
        db.close();
        return response;
    } else {
        return "CONNESSION_ERROR";
    }
}
std::string ManagementDB::handleSignup(const std::string e,const std::string username,const std::string psw){
    QSqlDatabase db  = connect();
    //TO DO, check per il controllo della mail
    //TO DO, non salvare la password in chiaro


    if (db.open()) {
        std::cout << "Connessione con il DB andata a buon fine";
        QSqlQuery query;
        QString email = QString::fromUtf8(e.data(), e.size());
        QString user  = QString::fromUtf8(username.data(), username.size());
        QString password = QString::fromUtf8(psw.data(), psw.size());

        if(checkMail(email)=="\"EMAIL_ERROR") {
            db.close();
            return "EMAIL_ERROR";
        }
        query.prepare("INSERT INTO user_login(email, username, password) VALUES ('"+email+"', '"+user+"', '"+password+"')");
        if(query.exec()) {
            db.close();
            return "SIGNUP_SUCCESS";
        }else {
            db.close();
            return "SIGNUP_ERROR";
        }
    } else {
        return "CONNESSION_ERROR";
    }

}
std::string ManagementDB::checkMail(const QString mail){
    QRegExp mailREX(R"(\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,4}\b)");
    mailREX.setCaseSensitivity(Qt::CaseInsensitive);
    mailREX.setPatternSyntax(QRegExp::RegExp);
    if (!mailREX.exactMatch(mail)) {
        return "EMAIL_ERROR";
    }
    return "EMAIL_OK";
}
std::string ManagementDB::encPsw(const std::string password){
    return "";
}