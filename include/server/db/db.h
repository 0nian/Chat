#ifndef DB_H
#define DB_H

#include<mysql/mysql.h>
#include<string>
using namespace std;

class MySQL{
    public:
        MySQL();
        ~MySQL();
        bool connect();
        MYSQL_RES* query(string sql);
        bool update(string sql);
        MYSQL* getConn();


    private:
        MYSQL *_conn;
};

#endif