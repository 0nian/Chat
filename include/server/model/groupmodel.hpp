#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include"group.hpp"

class GroupModel{
    public:
        bool create(Group &group);
        bool addGroup(int userid, int groupid, string role);
        vector<Group> queryGroupById(int userid);
        vector<int> queryGroupUsers(int userid,int groupid);
};

#endif