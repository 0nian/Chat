#include "db.h"
#include "group.hpp"
#include "groupuser.hpp"
#include "log.hpp"
#include "user.hpp"

#include <cstring>

using std::string;
using std::vector;

static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

static void bind_param_int(MYSQL_BIND *b, int *v) {
  std::memset(b, 0, sizeof(MYSQL_BIND));
  b->buffer_type = MYSQL_TYPE_LONG;
  b->buffer = v;
}

static void bind_param_string(MYSQL_BIND *b, const string &s,
                              unsigned long *plen) {
  std::memset(b, 0, sizeof(MYSQL_BIND));
  b->buffer_type = MYSQL_TYPE_STRING;
  b->buffer = const_cast<char *>(s.c_str());
  b->buffer_length = static_cast<unsigned long>(s.size());
  *plen = static_cast<unsigned long>(s.size());
  b->length = plen;
}

MySQL::MySQL() { _conn = mysql_init(nullptr); }
MySQL::~MySQL() {
  if (_conn != nullptr) {
        mysql_close(_conn);
    }
}
bool MySQL::connect() {
  MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                password.c_str(), dbname.c_str(), 3306,
                                nullptr, 0);
  if (p != nullptr) {
    mysql_query(_conn, "set names utf8mb4");
    lg(Info, "mysql connect success !!");
    }
    return p;
}
MYSQL_RES *MySQL::query(std::string sql) {
  if (mysql_query(_conn, sql.c_str())) {
    lg(Info, "%s:%d: query failed: %s", __FILE__, __LINE__, sql.c_str());
        return nullptr;
    }
    return mysql_use_result(_conn);
}
bool MySQL::update(std::string sql) {
  if (mysql_query(_conn, sql.c_str())) {
    lg(Info, "%s:%d: update failed: %s", __FILE__, __LINE__, sql.c_str());
        return false;
    }
    return true;
}

MYSQL *MySQL::getConn() { return _conn; }

bool MySQL::insertUser(const string &name, const string &password,
                       const string &state, my_ulonglong *new_id) {
  const char *sql =
      "INSERT INTO user (name, password, state) VALUES (?, ?, ?)";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  unsigned long l0, l1, l2;
  MYSQL_BIND bind[3];
  bind_param_string(&bind[0], name, &l0);
  bind_param_string(&bind[1], password, &l1);
  bind_param_string(&bind[2], state, &l2);
  bool ok = false;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, bind))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;
  if (new_id)
    *new_id = mysql_insert_id(_conn);
  ok = true;
end:
  if (!ok)
    lg(Info, "insertUser stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::selectUserById(int id, User &user) {
  user = User();
  const char *sql = "SELECT id, name, password, state FROM user WHERE id = ?";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  MYSQL_BIND pb;
  bind_param_int(&pb, &id);
  bool ok = false;
  int oid = 0;
  char nb[512] = {}, pwdb[512] = {}, stb[128] = {};
  unsigned long nlen = 0, pwlen = 0, slen = 0;
  MYSQL_BIND rb[4];
  int fr = 0;
  std::memset(rb, 0, sizeof(rb));
  rb[0].buffer_type = MYSQL_TYPE_LONG;
  rb[0].buffer = &oid;
  rb[1].buffer_type = MYSQL_TYPE_STRING;
  rb[1].buffer = nb;
  rb[1].buffer_length = sizeof(nb);
  rb[1].length = &nlen;
  rb[2].buffer_type = MYSQL_TYPE_STRING;
  rb[2].buffer = pwdb;
  rb[2].buffer_length = sizeof(pwdb);
  rb[2].length = &pwlen;
  rb[3].buffer_type = MYSQL_TYPE_STRING;
  rb[3].buffer = stb;
  rb[3].buffer_length = sizeof(stb);
  rb[3].length = &slen;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, &pb))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;

  if (mysql_stmt_bind_result(st, rb))
    goto end;
  fr = mysql_stmt_fetch(st);
  if (fr == 1)
    goto end;
  if (fr == MYSQL_NO_DATA) {
    ok = true;
    goto end;
  }
  user.setId(oid);
  user.setName(string(nb, nlen));
  user.setPassword(string(pwdb, pwlen));
  user.setState(string(stb, slen));
  ok = true;
end:
  if (!ok && st)
    lg(Info, "selectUserById stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::updateUserState(int id, const string &state) {
  const char *sql = "UPDATE user SET state = ? WHERE id = ?";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  unsigned long slen;
  MYSQL_BIND bind[2];
  bind_param_string(&bind[0], state, &slen);
  bind_param_int(&bind[1], &id);
  bool ok = false;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, bind))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;
  ok = true;
end:
  if (!ok)
    lg(Info, "updateUserState stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::resetOnlineUsersOffline() {
  return update("UPDATE user SET state = 'offline' WHERE state = 'online'");
}

bool MySQL::insertFriend(int userid, int friendid) {
  const char *sql = "INSERT INTO friend (userid, friendid) VALUES (?, ?)";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  MYSQL_BIND bind[2];
  bind_param_int(&bind[0], &userid);
  bind_param_int(&bind[1], &friendid);
  bool ok = false;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, bind))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;
  ok = true;
end:
  if (!ok)
    lg(Info, "insertFriend stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::selectFriendsOfUser(int userid, vector<User> &out) {
  out.clear();
  const char *sql =
      "SELECT a.id, a.name, a.state FROM user a "
      "INNER JOIN friend b ON a.id = b.friendid WHERE b.userid = ?";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  MYSQL_BIND pb;
  bind_param_int(&pb, &userid);
  bool ok = false;
  int uid = 0;
  char nb[512] = {}, stb[128] = {};
  unsigned long nlen = 0, slen = 0;
  MYSQL_BIND rb[3];
  std::memset(rb, 0, sizeof(rb));
  rb[0].buffer_type = MYSQL_TYPE_LONG;
  rb[0].buffer = &uid;
  rb[1].buffer_type = MYSQL_TYPE_STRING;
  rb[1].buffer = nb;
  rb[1].buffer_length = sizeof(nb);
  rb[1].length = &nlen;
  rb[2].buffer_type = MYSQL_TYPE_STRING;
  rb[2].buffer = stb;
  rb[2].buffer_length = sizeof(stb);
  rb[2].length = &slen;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, &pb))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;

  if (mysql_stmt_bind_result(st, rb))
    goto end;
  for (;;) {
    int fr = mysql_stmt_fetch(st);
    if (fr == MYSQL_NO_DATA)
      break;
    if (fr != 0)
      goto end;
    User u;
    u.setId(uid);
    u.setName(string(nb, nlen));
    u.setState(string(stb, slen));
    out.push_back(u);
  }
  ok = true;
end:
  if (!ok)
    lg(Info, "selectFriendsOfUser stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::insertGroup(const string &groupname, const string &groupdesc,
                        my_ulonglong *new_id) {
  const char *sql =
      "INSERT INTO allgroup (groupname, groupdesc) VALUES (?, ?)";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  unsigned long l0, l1;
  MYSQL_BIND bind[2];
  bind_param_string(&bind[0], groupname, &l0);
  bind_param_string(&bind[1], groupdesc, &l1);
  bool ok = false;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, bind))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;
  if (new_id)
    *new_id = mysql_insert_id(_conn);
  ok = true;
end:
  if (!ok)
    lg(Info, "insertGroup stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::insertGroupUser(int groupid, int userid, const string &role) {
  const char *sql =
      "INSERT INTO groupuser (groupid, userid, grouprole) VALUES (?, ?, ?)";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  unsigned long rlen;
  MYSQL_BIND bind[3];
  bind_param_int(&bind[0], &groupid);
  bind_param_int(&bind[1], &userid);
  bind_param_string(&bind[2], role, &rlen);
  bool ok = false;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, bind))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;
  ok = true;
end:
  if (!ok)
    lg(Info, "insertGroupUser stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::selectGroupsBasicForUser(int userid, vector<Group> &out) {
  out.clear();
  const char *sql =
      "SELECT b.id, b.groupname, b.groupdesc FROM groupuser a "
      "INNER JOIN allgroup b ON a.groupid = b.id WHERE a.userid = ?";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  MYSQL_BIND pb;
  bind_param_int(&pb, &userid);
  bool ok = false;
  int gid = 0;
  char nameb[256] = {}, descb[512] = {};
  unsigned long nlen = 0, dlen = 0;
  MYSQL_BIND rb[3];
  std::memset(rb, 0, sizeof(rb));
  rb[0].buffer_type = MYSQL_TYPE_LONG;
  rb[0].buffer = &gid;
  rb[1].buffer_type = MYSQL_TYPE_STRING;
  rb[1].buffer = nameb;
  rb[1].buffer_length = sizeof(nameb);
  rb[1].length = &nlen;
  rb[2].buffer_type = MYSQL_TYPE_STRING;
  rb[2].buffer = descb;
  rb[2].buffer_length = sizeof(descb);
  rb[2].length = &dlen;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, &pb))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;

  if (mysql_stmt_bind_result(st, rb))
    goto end;
  for (;;) {
    int fr = mysql_stmt_fetch(st);
    if (fr == MYSQL_NO_DATA)
      break;
    if (fr != 0)
      goto end;
    Group g;
    g.setId(gid);
    g.setName(string(nameb, nlen));
    g.setDesc(string(descb, dlen));
    out.push_back(g);
  }
  ok = true;
end:
  if (!ok)
    lg(Info, "selectGroupsBasicForUser stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::selectGroupMembers(int groupid, vector<GroupUser> &out) {
  out.clear();
  const char *sql =
      "SELECT a.id, a.name, a.state, b.grouprole FROM user a "
      "INNER JOIN groupuser b ON a.id = b.userid WHERE b.groupid = ?";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  MYSQL_BIND pb;
  bind_param_int(&pb, &groupid);
  bool ok = false;
  int uid = 0;
  char nb[512] = {}, stb[128] = {}, rb[64] = {};
  unsigned long nlen = 0, slen = 0, rlen = 0;
  MYSQL_BIND resb[4];
  std::memset(resb, 0, sizeof(resb));
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, &pb))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;

  resb[0].buffer_type = MYSQL_TYPE_LONG;
  resb[0].buffer = &uid;
  resb[1].buffer_type = MYSQL_TYPE_STRING;
  resb[1].buffer = nb;
  resb[1].buffer_length = sizeof(nb);
  resb[1].length = &nlen;
  resb[2].buffer_type = MYSQL_TYPE_STRING;
  resb[2].buffer = stb;
  resb[2].buffer_length = sizeof(stb);
  resb[2].length = &slen;
  resb[3].buffer_type = MYSQL_TYPE_STRING;
  resb[3].buffer = rb;
  resb[3].buffer_length = sizeof(rb);
  resb[3].length = &rlen;
  if (mysql_stmt_bind_result(st, resb))
    goto end;
  for (;;) {
    int fr = mysql_stmt_fetch(st);
    if (fr == MYSQL_NO_DATA)
      break;
    if (fr != 0)
      goto end;
    GroupUser gu;
    gu.setId(uid);
    gu.setName(string(nb, nlen));
    gu.setState(string(stb, slen));
    gu.setRole(string(rb, rlen));
    out.push_back(gu);
  }
  ok = true;
end:
  if (!ok)
    lg(Info, "selectGroupMembers stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::selectGroupOtherUserIds(int groupid, int exclude_userid,
                                    vector<int> &out) {
  out.clear();
  const char *sql = "SELECT userid FROM groupuser WHERE groupid = ? AND "
                     "userid != ?";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  MYSQL_BIND bind[2];
  bind_param_int(&bind[0], &groupid);
  bind_param_int(&bind[1], &exclude_userid);
  bool ok = false;
  int uid = 0;
  MYSQL_BIND rb;
  std::memset(&rb, 0, sizeof(rb));
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, bind))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;

  rb.buffer_type = MYSQL_TYPE_LONG;
  rb.buffer = &uid;
  if (mysql_stmt_bind_result(st, &rb))
    goto end;
  for (;;) {
    int fr = mysql_stmt_fetch(st);
    if (fr == MYSQL_NO_DATA)
      break;
    if (fr != 0)
      goto end;
    out.push_back(uid);
  }
  ok = true;
end:
  if (!ok)
    lg(Info, "selectGroupOtherUserIds stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::insertOfflineMessage(int userid, const string &msg) {
  const char *sql =
      "INSERT INTO offlinemessage (userid, message) VALUES (?, ?)";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  unsigned long mlen;
  MYSQL_BIND bind[2];
  bind_param_int(&bind[0], &userid);
  bind_param_string(&bind[1], msg, &mlen);
  bool ok = false;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, bind))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;
  ok = true;
end:
  if (!ok)
    lg(Info, "insertOfflineMessage stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::deleteOfflineMessages(int userid) {
  const char *sql = "DELETE FROM offlinemessage WHERE userid = ?";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  MYSQL_BIND pb;
  bind_param_int(&pb, &userid);
  bool ok = false;
  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, &pb))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;
  ok = true;
end:
  if (!ok)
    lg(Info, "deleteOfflineMessages stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}

bool MySQL::selectOfflineMessages(int userid, vector<string> &out) {
  out.clear();
  const char *sql =
      "SELECT message FROM offlinemessage WHERE userid = ?";
  MYSQL_STMT *st = mysql_stmt_init(_conn);
  if (!st)
    return false;
  MYSQL_BIND pb;
  bind_param_int(&pb, &userid);
  bool ok = false;
  vector<char> mbuf(65536);
  unsigned long mlen = 0;
  MYSQL_BIND rb;
  std::memset(&rb, 0, sizeof(rb));
  rb.buffer_type = MYSQL_TYPE_STRING;
  rb.buffer = mbuf.data();
  rb.buffer_length = static_cast<unsigned long>(mbuf.size());
  rb.length = &mlen;

  if (mysql_stmt_prepare(st, sql, static_cast<unsigned long>(strlen(sql))))
    goto end;
  if (mysql_stmt_bind_param(st, &pb))
    goto end;
  if (mysql_stmt_execute(st))
    goto end;
  if (mysql_stmt_bind_result(st, &rb))
    goto end;
  for (;;) {
    mlen = 0;
    int fr = mysql_stmt_fetch(st);
    if (fr == MYSQL_NO_DATA)
      break;
    if (fr != 0)
      goto end;
    out.emplace_back(mbuf.data(), mlen);
  }
  ok = true;
end:
  if (!ok)
    lg(Info, "selectOfflineMessages stmt: %s", mysql_stmt_error(st));
  mysql_stmt_close(st);
  return ok;
}
