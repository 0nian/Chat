#include "chatserver.hpp"
#include"chatservice.hpp"
#include<iostream>
#include<signal.h>
using namespace std;
void handleExit(int){
    ChatService::getInstance()->reset();
    exit(0);
}

int main(int argc, char* argv[]){

    if (argc < 3)
    {
        cout << "command invalid!" << endl;
        exit(-1);
    }
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT,handleExit);
    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"ChatServer");

    server.start();
    loop.loop();
}