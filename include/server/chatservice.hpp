#ifndef CHATSERVICE_HPP
#define CHATSERVICE_HPP

#include<muduo/net/TcpClient.h>
#include<unordered_map>
#include "json.hpp"
#include"usermodel.hpp"
#include"friendmodel.hpp"
#include"offlinemsgmodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"
#include<mutex>
using namespace muduo::net;
using namespace muduo;
using json = nlohmann::json;

using MsgHander = std::function<void (const TcpConnectionPtr &conn,json js,Timestamp time)>;
class ChatService{
    public:
        void login(const TcpConnectionPtr &conn,json js,Timestamp time);
        void reg(const TcpConnectionPtr &conn,json js,Timestamp time);
        void oneChat(const TcpConnectionPtr &conn,json js,Timestamp time);
        static ChatService* getInstance();
        MsgHander getHander(int msgid);
        void clientExceptionExit(const TcpConnectionPtr &conn);
        void addFriend(const TcpConnectionPtr &conn,json js,Timestamp time);

        void createGroup(const TcpConnectionPtr &conn,json js,Timestamp time);
        void addGroup(const TcpConnectionPtr &conn,json js,Timestamp time);
        void groupChat(const TcpConnectionPtr &conn,json js,Timestamp time);
        void logout(const TcpConnectionPtr &conn,json js,Timestamp time);
        void reset();

        void handlerRedisSubscribeMsg(int channel,string msg);
    private:
        ChatService();

        std::unordered_map<int,MsgHander> _msgHanderMap;
        std::unordered_map<int,TcpConnectionPtr> _userConnMap;
        std::mutex _connMutex;
        UserModel _userModel;
        OfflineMsgModel _offlineMsgModel;
        FriendModel _friendModel;
        GroupModel _groupModel;
        Redis _redis;

};

#endif

