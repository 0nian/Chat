#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include<muduo/base/Logging.h>
#include<functional>

using json = nlohmann::json;
using namespace std;

ChatServer::ChatServer(EventLoop* loop,    //事件循环
            const InetAddress& listenAddr,  //IP + 端口
            const string& nameArg)
    :_server(loop,listenAddr,nameArg),_loop(loop)
{
    //给服务器注册事件回调
    _server.setConnectionCallback(std::bind(&ChatServer::OnConnection,this,placeholders::_1));
                
    _server.setMessageCallback(std::bind(&ChatServer::OnMessage,this,placeholders::_1,placeholders::_2,placeholders::_3));

    //设置合适的服务端线程数量
    _server.setThreadNum(4);
}

void ChatServer::start(){
    _server.start();
}

void ChatServer::OnConnection(const TcpConnectionPtr &conn){
    LOG_INFO<<"ON connect";  
    if(!conn->connected()){
        LOG_INFO<<"client exit...";
        ChatService::getInstance()->clientExceptionExit(conn);
        conn->shutdown();
    }
}

void ChatServer::OnMessage(const TcpConnectionPtr &conn,
                            Buffer *buf,
                            Timestamp time)
{
        string str = buf->retrieveAllAsString();
        json js = json::parse(str);
        ChatService::getInstance()->getHander(js["msgid"].get<int>())(conn,js,time);
}