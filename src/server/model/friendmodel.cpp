#include "db.h"
#include "friendmodel.hpp"

void FriendModel::insert(int userid, int friendid) {
  MySQL mysql;
  if (mysql.connect())
    mysql.insertFriend(userid, friendid);
}

vector<User> FriendModel::query(int userid) {
  MySQL mysql;
  vector<User> v;
  if (mysql.connect())
    mysql.selectFriendsOfUser(userid, v);
  return v;
}
