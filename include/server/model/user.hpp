#ifndef USER_HPP
#define USER_HPP
#include<string>
using namespace std;


class User{
    public:
        User(int id = -1,string name = "",string password = "",string state = "offline")
            :id(id),name(name),password(password),state(state){};
        int getId(){ return id;}
        string getName(){ return name;}
        string getPassword(){ return password;}
        string getState(){ return state;}

        void setId(int id){ this->id = id;}
        void setName(string name){ this->name = name;}
        void setPassword(string password){ this->password = password;}
        void setState(string state){ this->state = state;}
        
    private:
        int id;
        string name;
        string password;
        string state;

};

#endif