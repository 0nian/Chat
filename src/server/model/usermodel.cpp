#include "db.h"
#include "usermodel.hpp"
#include <iostream>
using namespace std;

bool UserModel::insert(User &user) {
  MySQL mysql;
  if (!mysql.connect())
    return false;
  my_ulonglong nid = 0;
  if (!mysql.insertUser(user.getName(), user.getPassword(), user.getState(),
                        &nid))
    return false;
  user.setId(static_cast<int>(nid));
  return true;
}

User UserModel::query(int id) {
  MySQL mysql;
  if (!mysql.connect())
    return User();
  User user;
  if (!mysql.selectUserById(id, user))
    return User();
  return user;
}

bool UserModel::updateState(User user) {
  MySQL mysql;
  if (!mysql.connect())
    return false;
  return mysql.updateUserState(user.getId(), user.getState());
}

void UserModel::resetState() {
  MySQL mysql;
  if (mysql.connect())
    mysql.resetOnlineUsersOffline();
}
