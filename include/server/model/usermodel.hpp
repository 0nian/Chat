#ifndef USERMODEL_HPP
#define USERMODEL_HPP
#include"user.hpp"

class UserModel{
    public:
        bool insert(User &user);
        User query(int id);
        bool updateState(User user);
        void resetState();
    private:
};

#endif