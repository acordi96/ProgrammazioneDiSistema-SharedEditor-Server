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
QString ManagementDB::generateRandomColor() {
    std::string hex = "abcdef0123456789";
    int n = hex.length();
    std::string color = "#88"; //alpha will be fixed to 88
    for (int i=1; i<=6; i++)
        color.push_back(hex[rand() % n]);
    return(QString::fromUtf8(color.data(), color.size()));
}

std::string ManagementDB::handleLogin(const std::string user,const std::string password,QString& color) {
    QSqlDatabase db  = connect();
    if (db.open()) {
        QSqlQuery query;
        QString username = QString::fromUtf8(user.data(), user.size());
        QString psw = QString::fromUtf8(password.data(), password.size());
        query.prepare("SELECT username, password, color FROM user_login WHERE username='"+username+"' and password='"+psw+"'");
        if(query.exec()) {
            if(query.next()) {
                color = query.value(2).toString();
                std::cout << "L'utente ha colore " << color.toStdString() << std::endl;
                response = "LOGIN_SUCCESS";
            }else
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
        QString color = generateRandomColor();

        if(checkMail(email)=="\"EMAIL_ERROR") {
            db.close();
            return "EMAIL_ERROR";
        }
        query.prepare("SELECT username FROM user_login WHERE username='"+user+"'");
        if(query.exec()){
            if(query.next())
                return "SIGNUP_ERROR_DUPLICATE_USERNAME";
            else{
                QSqlQuery query2;
                query2.prepare("INSERT INTO user_login(email, username, password, color) VALUES ('"+email+"', '"+user+"', '"+password+"','"+color+"')");
                if(query2.exec()) {
                    db.close();
                    return "SIGNUP_SUCCESS";
                }else {
                    db.close();
                    return "SIGNUP_ERROR_INSERT_FAILED";
                }
            }


        }else
            return "CONNESSION_ERROR";


    } else {
        return "CONNESSION_ERROR_";
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

std::string ManagementDB::handleNewFile(const std::string user, const std::string file) {
    QSqlDatabase db  = connect();

    if (db.open()) {
        QSqlQuery query;
        QString id = QString::fromUtf8(user.data(),user.size());
        QString title = QString::fromUtf8(file.data(),file.size());
        query.prepare("INSERT INTO files(username,titolo) VALUES ('"+id+"','"+title+"')");

        if(query.exec()){
            db.close();
            return "FILE_INSERT_SUCCESS";
        }else {
            db.close();
            return "FILE_INSERT_FAILED";
        }

    } else
        return "CONNESSION_ERROR_";
}

std::list<std::string> ManagementDB::takeFiles(const std::string user) {
    QSqlDatabase db  = connect();
    if(db.open()){
        std::cout << "\n entrato nel db \n";
        QSqlQuery query;
        QString id = QString::fromUtf8(user.data(),user.size());
        query.prepare("SELECT titolo FROM files WHERE username = '"+id+"'");
        if(query.exec()){
            std::list<std::string> files;
            while(query.next()){
                QString title = query.value(0).toString();
                std::string titolo = title.toStdString();
                std::cout <<"\n " << titolo << "\n";
                files.push_back(titolo);

            }
            return files;
        }

    }
    return std::list<std::string>();
}
