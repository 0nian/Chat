#ifndef OFFLINEMSGMODEL_HPP
#define OFFLINEMSGMODEL_HPP
#include<string>
#include<vector>
using namespace std;

class OfflineMsgModel{
    public:
        void insert(int id,string msg);
        void remove(int id);
        vector<string> query(int id);
};

#endif