#include "db.h"
#include "groupmodel.hpp"
#include <iostream>

bool GroupModel::create(Group &group) {
  MySQL mysql;
  if (!mysql.connect())
    return false;
  my_ulonglong gid = 0;
  if (!mysql.insertGroup(group.getName(), group.getDesc(), &gid))
    return false;
  group.setId(static_cast<int>(gid));
  return true;
}

bool GroupModel::addGroup(int userid, int groupid, string role) {
  MySQL mysql;
  if (!mysql.connect())
    return false;
  return mysql.insertGroupUser(groupid, userid, role);
}

vector<Group> GroupModel::queryGroupById(int userid) {
  vector<Group> vec;
  MySQL mysql;
  if (!mysql.connect())
    return vec;
  if (!mysql.selectGroupsBasicForUser(userid, vec))
    return vec;
  for (Group &group : vec) {
    MySQL mysql2;
    vector<GroupUser> members;
    if (mysql2.connect() &&
        mysql2.selectGroupMembers(group.getId(), members)) {
      for (GroupUser &gu : members)
        group.getUsers().push_back(gu);
    }
  }
  return vec;
}

vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
  vector<int> vec;
  MySQL mysql;
  if (mysql.connect())
    mysql.selectGroupOtherUserIds(groupid, userid, vec);
  return vec;
}
