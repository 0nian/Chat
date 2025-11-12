#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

//聊天服务器主类
class ChatServer{
    public:
        ChatServer(EventLoop* loop,    //事件循环
            const InetAddress& listenAddr,  //IP + 端口
            const string& nameArg); // 名字
            
        void start();
    private:
        void OnConnection(const TcpConnectionPtr &conn);

        void OnMessage(const TcpConnectionPtr &conn,
                            Buffer *buf,
                            Timestamp time);

        TcpServer _server;
        EventLoop *_loop;
};

#endif