#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
#include <vector>

class User;
class Group;
class GroupUser;

class MySQL {
public:
  MySQL();
  ~MySQL();
  bool connect();
  MYSQL_RES *query(std::string sql);
  bool update(std::string sql);
  MYSQL *getConn();

  // ---------- prepared statements (防注入，字符串可含单引号等) ----------
  bool insertUser(const std::string &name, const std::string &password,
                  const std::string &state, my_ulonglong *new_id);
  bool selectUserById(int id, User &user);
  bool updateUserState(int id, const std::string &state);
  bool resetOnlineUsersOffline();

  bool insertFriend(int userid, int friendid);
  bool selectFriendsOfUser(int userid, std::vector<User> &out);

  bool insertGroup(const std::string &groupname, const std::string &groupdesc,
                   my_ulonglong *new_id);
  bool insertGroupUser(int groupid, int userid, const std::string &role);
  bool selectGroupsBasicForUser(int userid, std::vector<Group> &out);
  bool selectGroupMembers(int groupid, std::vector<GroupUser> &out);
  bool selectGroupOtherUserIds(int groupid, int exclude_userid,
                               std::vector<int> &out);

  bool insertOfflineMessage(int userid, const std::string &msg);
  bool deleteOfflineMessages(int userid);
  bool selectOfflineMessages(int userid, std::vector<std::string> &out);

private:
  MYSQL *_conn;
};

#endif
