#include "groupmodel.hpp"
#include "db.h"
#include <iostream>

bool GroupModel::create(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname,groupdesc) values('%s','%s')", group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConn()));
            return true;
        }
    }
    return false;
}
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values('%d','%d','%s')", groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}
vector<Group> GroupModel::queryGroupById(int userid)
{
    //  两次联合查询查到结果
    char sql[1024] = {0};
    vector<Group> vec;
    sprintf(sql, "select b.id,b.groupname,b.groupdesc from groupuser a inner join allgroup b on a.groupid = b.id where a.userid = %d", userid);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                vec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    for (Group &group : vec)
    {
        MySQL mysql;
        char sql[1024] = {0};
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on a.id = b.userid where b.groupid = %d", group.getId());
        if (mysql.connect())
        {
            MYSQL_RES *res = mysql.query(sql);
            GroupUser user;
            if (res != nullptr)
            {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    user.setId(atoi(row[0]));
                    user.setName(row[1]);
                    user.setState(row[2]);
                    user.setRole(row[3]);
                    group.getUsers().push_back(user);
                }
            }
            mysql_free_result(res);
        }
    }
    return vec;
}
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    vector<int> vec;
    sprintf(sql, "select userid from groupuser where groupid = %d and userid !=%d", groupid, userid);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
        return vec;
    }
    return vec;
}