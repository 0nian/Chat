#include"redis.hpp"
#include<iostream>
#include <thread>
using namespace std;


Redis::Redis():_publish_context(nullptr),_subscribe_context(nullptr){

}
Redis::~Redis(){
    if(_publish_context != nullptr){
        redisFree(_publish_context);
    }      
    if(_subscribe_context != nullptr){
        redisFree(_subscribe_context);
    }
}

bool Redis::connect(){
    _publish_context = redisConnect("127.0.0.1",6379);
    if(_publish_context == nullptr){
        cerr<<"connect redis failed !!!" <<endl;
        return false;
    }

    _subscribe_context = redisConnect("127.0.0.1",6379);
    if(_subscribe_context == nullptr){
        cerr<<"connect redis failed !!!"<<endl;
        return false;
    }
    thread t([&](){
        observe_channel_message();
    });
    t.detach();

    cout<<"connect redis success !!!"<<endl;

    return true;
}

bool Redis::publish(int channel, string msg){
    redisReply *reply = (redisReply *)redisCommand(_publish_context,"PUBLISH %d %s",channel,msg.c_str());
    if(reply == nullptr){
        cerr<<"publish command failed !!!"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel){
    if(redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d",channel) == REDIS_ERR){
        cerr<<"subscribe command failed!!"<<endl;
        return false;
    }
    int done = 0;
    while(!done){
        if(redisBufferWrite(this->_subscribe_context,&done) == REDIS_ERR){
            cerr<<"subscribe command failed!!"<<endl;
            return false;
        }
    }

    return true;
}

bool Redis::unsbuscribe(int channel){
    if(redisAppendCommand(this->_subscribe_context,"UNSUBSCRIBE %d",channel) == REDIS_ERR){
        cerr<<"unsubscribe command failed!!"<<endl;
        return false;
    }
    int done = 0;
    while(!done){
        if(redisBufferWrite(this->_subscribe_context,&done) == REDIS_ERR){
            cerr<<"unsubscribe command failed!!"<<endl;
            return false;
        }
    }

    return true;
}

void Redis::observe_channel_message(){
    redisReply *reply = nullptr;
    while(redisGetReply(this->_subscribe_context,(void **)&reply) == REDIS_OK){
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr){
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr <<"observe_channel_message quit <<<<<< "<<endl;
}

// 初始化消息回调函数
void Redis::init_notify_handler(function<void(int,string)> fn){
    _notify_message_handler = fn;
}
