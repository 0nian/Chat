#ifndef REDIS_H
#define REDIS_H

#include<string>
#include<functional>
#include<hiredis/hiredis.h>
using namespace std;

class Redis{
    public:
        Redis();
        ~Redis();

        bool connect();

        bool publish(int channel, string msg);

        bool subscribe(int channel);

        bool unsbuscribe(int channel);

        void observe_channel_message();

        // 初始化消息回调函数
        void init_notify_handler(function<void(int,string)> fn);



    private:
        // hiredis同步上下文对象，负责publish对象
        redisContext *_publish_context;

        // hiredis同步上下文对象，负责subscribe对象
        redisContext *_subscribe_context;

        // 回调操作
        function<void(int,string)> _notify_message_handler;

};

#endif