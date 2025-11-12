#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>

void ChatService::login(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int id = js["id"].get<int>();
    string password = js["password"];
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPassword() == password)
    {
        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = MSG_LOG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "repeat login!";
            conn->send(response.dump());
        }

        // 登录成功，更新用户状态信息
        user.setState("online");
        _userModel.updateState(user);

        // 保存用户上线状态
        {
            std::lock_guard<mutex> mylock(_connMutex);
            _userConnMap.insert({id, conn});
        }
        _redis.subscribe(id);

        json response;
        response["msgid"] = MSG_LOG_ACK;
        response["errno"] = 0;
        response["id"] = id;
        response["name"] = user.getName();
        vector<string> v = _offlineMsgModel.query(id);
        if(!v.empty()){
            response["offlineMsg"] = v;
            _offlineMsgModel.remove(id);
        }

        //查询好友信息
        vector<User> v0 = _friendModel.query(id);
        if(!v0.empty()){
            vector<string> vec;
            for(User &user : v0){
                json js;
                js["id"] = user.getId();
                js["name"] = user.getName();
                js["state"] = user.getState();
                vec.push_back(js.dump());
            }
            response["friends"] = vec;
        }

        //查询群组信息
        vector<Group> groupVec = _groupModel.queryGroupById(id);
        if(!groupVec.empty()){
            vector<string> groupV;
            for(Group &group : groupVec){
                json j1;
                j1["id"] = group.getId();
                j1["groupname"] = group.getName();
                j1["groupdesc"] = group.getDesc();
                vector<string> temp;
                for(GroupUser &gs : group.getUsers()){
                    json j4;
                    j4["id"] = gs.getId();
                    j4["name"] = gs.getName();
                    j4["state"] = gs.getState();
                    j4["role"] = gs.getRole();
                    temp.push_back(j4.dump());
                }
                j1["users"] = temp;
                groupV.push_back(j1.dump());
            }
            response["groups"] = groupV;
        }
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = MSG_LOG_ACK;
        response["errno"] = 2;
        response["errmsg"] = "user or password error!";
        conn->send(response.dump());
    }
}

void ChatService::reg(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];
    User user;
    user.setName(name);
    user.setPassword(password);
    bool res = _userModel.insert(user);
    if (res)
    {
        json response;
        response["msgid"] = MSG_REG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = MSG_REG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn,json js,Timestamp time){
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> mylock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end()){
            it->second->send(js.dump());
            return;
        }
    }
    User user = _userModel.query(toid);
    if(user.getState() == "online"){
        _redis.publish(toid,js.dump());
        return;
    }
    //不在线 存储离线消息
    _offlineMsgModel.insert(toid,js.dump());
}

ChatService *ChatService::getInstance()
{
    static ChatService chatService;
    return &chatService;
}

ChatService::ChatService()
{
    _msgHanderMap.insert({MSG_LOG, std::bind(&ChatService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHanderMap.insert({MSG_REG, std::bind(&ChatService::reg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHanderMap.insert({MSG_ONE_CHAT, std::bind(&ChatService::oneChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHanderMap.insert({MSG_ADD_FRIEND, std::bind(&ChatService::addFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

     _msgHanderMap.insert({MSG_CREATE_GROUP, std::bind(&ChatService::createGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHanderMap.insert({MSG_ADD_GROUP, std::bind(&ChatService::addGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHanderMap.insert({MSG_GROUP_CHAT, std::bind(&ChatService::groupChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    
    _msgHanderMap.insert({MSG_LOGOUT, std::bind(&ChatService::logout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    if(_redis.connect()){
        _redis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribeMsg,this,std::placeholders::_1, std::placeholders::_2));
    }
}

void ChatService::handlerRedisSubscribeMsg(int channel,string msg){
    lock_guard<mutex> mylock(_connMutex);
    auto it = _userConnMap.find(channel);
    if(it != _userConnMap.end()){
        it->second->send(msg);
        return;
    }

    _offlineMsgModel.insert(channel,msg);
}

MsgHander ChatService::getHander(int msgid)
{
    if (_msgHanderMap.find(msgid) != _msgHanderMap.end())
    {
        return _msgHanderMap[msgid];
    }
    else
    {
        return [](const TcpConnectionPtr &conn, json js, Timestamp time)
        {
            LOG_ERROR << "handler is not found!!!";
        };
    }
}

void ChatService::clientExceptionExit(const TcpConnectionPtr &conn)
{
    {
        User user;
        lock_guard<mutex> mylock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
        _redis.unsbuscribe(user.getId());
        if (user.getId() != -1)
        {
            user.setState("offline");
            _userModel.updateState(user);
        }
    }
}


void ChatService::logout(const TcpConnectionPtr &conn,json js,Timestamp time){
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> mylock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end()){
            _userConnMap.erase(it);
        }
    }
    _redis.unsbuscribe(userid);
    User user(userid);
    _userModel.updateState(user);    
}

void ChatService::reset(){
    _userModel.resetState();
}

void ChatService::addFriend(const TcpConnectionPtr &conn,json js,Timestamp time){
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid,friendid);

}

void ChatService::createGroup(const TcpConnectionPtr &conn,json js,Timestamp time){
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1,name,desc);
    if(_groupModel.create(group)){
        _groupModel.addGroup(userid,group.getId(),"creator");
    }
}

void ChatService::addGroup(const TcpConnectionPtr &conn,json js,Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn,json js,Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid,groupid);
    lock_guard<mutex> mylock(_connMutex);
    for(int id : useridVec){
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end()){
            it->second->send(js.dump());
        }else{
            User user = _userModel.query(id);
            if(user.getState() == "online"){
                _redis.publish(id,js.dump());
            }else{
                _offlineMsgModel.insert(id,js.dump());
            }
        }
    }
}
