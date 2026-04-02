#ifndef CHATSERVICE_HPP
#define CHATSERVICE_HPP

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "chat_connection.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "json.hpp"
#include "offlinemsgmodel.hpp"
#include "redis.hpp"
#include "usermodel.hpp"

using json = nlohmann::json;

using MsgHander =
    std::function<void(const std::shared_ptr<ChatConnection> &conn, json js)>;

class ChatService {
public:
  void login(const std::shared_ptr<ChatConnection> &conn, json js);
  void reg(const std::shared_ptr<ChatConnection> &conn, json js);
  void oneChat(const std::shared_ptr<ChatConnection> &conn, json js);
  static ChatService *getInstance();
  MsgHander getHander(int msgid);
  void clientExceptionExit(const std::shared_ptr<ChatConnection> &conn);
  void addFriend(const std::shared_ptr<ChatConnection> &conn, json js);

  void createGroup(const std::shared_ptr<ChatConnection> &conn, json js);
  void addGroup(const std::shared_ptr<ChatConnection> &conn, json js);
  void groupChat(const std::shared_ptr<ChatConnection> &conn, json js);
  void logout(const std::shared_ptr<ChatConnection> &conn, json js);
  void reset();

  void handlerRedisSubscribeMsg(int channel, std::string msg);

private:
  ChatService();

  std::unordered_map<int, MsgHander> _msgHanderMap;
  std::unordered_map<int, std::shared_ptr<ChatConnection>> _userConnMap;
  std::mutex _connMutex;
  UserModel _userModel;
  OfflineMsgModel _offlineMsgModel;
  FriendModel _friendModel;
  GroupModel _groupModel;
  Redis _redis;
};

#endif
