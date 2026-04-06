#include "db.h"
#include "offlinemsgmodel.hpp"

void OfflineMsgModel::insert(int id, string msg) {
  MySQL mysql;
  if (mysql.connect())
    mysql.insertOfflineMessage(id, msg);
}

void OfflineMsgModel::remove(int id) {
  MySQL mysql;
  if (mysql.connect())
    mysql.deleteOfflineMessages(id);
}

vector<string> OfflineMsgModel::query(int id) {
  vector<string> v;
  MySQL mysql;
  if (mysql.connect())
    mysql.selectOfflineMessages(id, v);
  return v;
}
