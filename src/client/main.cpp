#include "json.hpp"
#include "net_frame.hpp"
#include <cerrno>
#include <chrono>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;
using json = nlohmann::json;

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static string g_recv_accum;

static bool sendFramed(int fd, const string &json_text) {
  string wire = chat_net::Encode(json_text);
  size_t off = 0;
  while (off < wire.size()) {
    ssize_t n = send(fd, wire.data() + off, wire.size() - off, 0);
    if (n <= 0)
      return false;
    off += static_cast<size_t>(n);
  }
  return true;
}

static bool recvOneJson(int fd, json &out) {
  while (true) {
    string one;
    if (chat_net::Decode(g_recv_accum, one)) {
      out = json::parse(one);
      return true;
    }
    char buf[8192];
    ssize_t len = recv(fd, buf, sizeof(buf), 0);
    if (len == 0)
      return false;
    if (len < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        continue;
      return false;
    }
    g_recv_accum.append(buf, static_cast<size_t>(len));
  }
}

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

bool isMain = false;
// ?????????????????
User g_User;

// ???????????
vector<User> g_UserFriendList;

// ??????????
vector<Group> g_UserGroupList;

// ?????????????????
void showUser();

// ???????
void readTaskHandler(int clientfd);

// ????????????
void mainMenu(int clientfd);

// ????????
string getCurrentTime();

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "command invalid!" << endl;
        exit(-1);
    }
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);
    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1)
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }
    g_recv_accum.clear();
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    setsockopt(clientfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

    while (1)
    {
        cout << "====================" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.exit" << endl;
        cout << "====================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); // ?????????????????

        switch (choice)
        {
        case 1:
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get();
            cout << "password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = MSG_LOG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            if (!sendFramed(clientfd, request))
            {
                cerr << "send login message error:" << request << endl;
            }
            else
            {
                json rjs;
                if (!recvOneJson(clientfd, rjs))
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {
                    if (rjs["errno"].get<int>() != 0)
                    {
                        cerr << rjs["errmsg"] << endl;
                    }
                    else
                    {
                        //  ??????
                        g_UserFriendList.clear();
                        g_UserGroupList.clear();
                        g_User.setId(rjs["id"].get<int>());
                        g_User.setName(rjs["name"]);
                        if (rjs.contains("friends"))
                        {
                            vector<string> vec = rjs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_UserFriendList.push_back(user);
                            }
                        }
                        if (rjs.contains("groups"))
                        {
                            vector<string> vec2 = rjs["groups"];
                            for (string &str : vec2)
                            {
                                json js = json::parse(str);
                                Group group;
                                group.setId(js["id"].get<int>());
                                group.setName(js["groupname"]);
                                group.setDesc(js["groupdesc"]);

                                vector<string> vec3 = js["users"];
                                for (string &str1 : vec3)
                                {
                                    GroupUser user;
                                    json j3 = json::parse(str1);
                                    user.setId(j3["id"].get<int>());
                                    user.setName(j3["name"]);
                                    user.setState(j3["state"]);
                                    user.setRole(j3["role"]);
                                    group.getUsers().push_back(user);
                                }
                                g_UserGroupList.push_back(group);
                            }
                        }
                        showUser();
                        if (rjs.contains("offlineMsg"))
                        {
                            vector<string> vv = rjs["offlineMsg"];
                            for (string &str : vv)
                            {
                                json js = json::parse(str);
                                int msgtype = js["msgid"].get<int>();
                                if (msgtype == MSG_ONE_CHAT)
                                {
                                    cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                                }
                                else if (msgtype == MSG_GROUP_CHAT)
                                {
                                    cout << "group message: [" << js["groupid"] << "]" << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                                }
                            }
                        }
                        //?????????????????????
                        if (!isMain)
                        {
                            isMain = true;
                            thread readTask(readTaskHandler, clientfd);
                            readTask.detach();
                        }
                    
                        // ?????????????
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = MSG_REG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            if (!sendFramed(clientfd, request))
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {
                json rjs;
                if (!recvOneJson(clientfd, rjs))
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    if (rjs["errno"].get<int>() != 0)
                    {
                        cerr << "user has been registered!!" << endl;
                    }
                    else
                    {
                        cout << name << " register success, id is " << rjs["id"] << " do not forget it !!!" << endl;
                    }
                }
            }
        }
        break;
        case 3:
            close(clientfd);
            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
}

void showUser()
{
    cout << "--------------------login user---------------------" << endl;
    cout << "id: " << g_User.getId() << "    name: " << g_User.getName() << endl;
    cout << "--------------------friend list---------------------" << endl;
    if (!g_UserFriendList.empty())
    {
        for (User &user : g_UserFriendList)
        {
            cout << "id: " << user.getId() << "    name: " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "--------------------group list---------------------" << endl;
    if (!g_UserGroupList.empty())
    {
        for (Group &group : g_UserGroupList)
        {
            cout << "id: " << group.getId() << "    name: " << group.getName() << " " << group.getDesc() << endl;
            cout << "*************users******" << endl;

            for (GroupUser &groupuser : group.getUsers())
            {
                cout << "id: " << groupuser.getId() << "    name: " << groupuser.getName() << " " << groupuser.getState() << " " << groupuser.getRole() << endl;
            }
            cout << "*************users******" << endl;
        }
    }
}

void readTaskHandler(int clientfd)
{
    while (isMain)
    {
        json js;
        if (!recvOneJson(clientfd, js))
        {
            close(clientfd);
            exit(-1);
        }
        int msgtype = js["msgid"].get<int>();
        if (msgtype == MSG_ONE_CHAT)
        {
            cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        else if (msgtype == MSG_GROUP_CHAT)
        {
            cout << "group message:  [" << js["groupid"] << "]" << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
    }
}

void help(int fd = 0, string s = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void quit(int, string);
// ????????
unordered_map<string, string> commandMap{
    {"help: ", "help"},
    {"chat: ", "chat:friendid:msg"},
    {"addfriend: ", "addfriend:friendid"},
    {"creategroup: ", "creategroup:name:desc"},
    {"addgroup: ", "addgroup:groupid"},
    {"groupchat: ", "groupchat:groupid:msg"},
    {"quit: ", "quit"}};

// ?????????????????
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"quit", quit}};

void mainMenu(int clientfd)
{
    help();
    char buf[1024] = {0};
    while (isMain)
    {
        cin.getline(buf, 1024);
        string commandbuf(buf);
        string command;
        int idx = commandbuf.find(":");
        if (idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid command!!!" << endl;
            continue;
        }

        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

void help(int fd, string s)
{
    for (auto &p : commandMap)
    {
        cout << p.first << " " << p.second << endl;
    }
    cout << endl;
}

//{"chat","chat:friendid:msg"}
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string msg = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = MSG_ONE_CHAT;
    js["id"] = g_User.getId();
    js["name"] = g_User.getName();
    js["toid"] = friendid;
    js["msg"] = msg;
    js["time"] = getCurrentTime();
    string buf = js.dump();

    if (!sendFramed(clientfd, buf))
    {
        cerr << "send chat msg error :" << buf << endl;
    }
}

//{"addfriend","addfriend:friendid"}
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = MSG_ADD_FRIEND;
    js["id"] = g_User.getId();
    js["friendid"] = friendid;
    string buf = js.dump();

    if (!sendFramed(clientfd, buf))
    {
        cerr << "send add friend msg error :" << buf << endl;
    }
}

// {"creategroup","creategroup:name:desc"}
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "create group command error!!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = MSG_CREATE_GROUP;
    js["id"] = g_User.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buf = js.dump();

    if (!sendFramed(clientfd, buf))
    {
        cerr << "send create group msg error :" << buf << endl;
    }
}

//{"addgroup","addgroup:groupid"}
void addgroup(int clientfd, string str)
{

    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = MSG_ADD_GROUP;
    js["id"] = g_User.getId();
    js["groupid"] = groupid;

    string buf = js.dump();

    if (!sendFramed(clientfd, buf))
    {
        cerr << "send add group msg error :" << buf << endl;
    }
}

// {"groupchat","groupchat:groupid:msg"}
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "group chat command error!!" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string msg = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = MSG_GROUP_CHAT;
    js["id"] = g_User.getId();
    js["name"] = g_User.getName();
    js["groupid"] = groupid;
    js["msg"] = msg;
    js["time"] = getCurrentTime();
    string buf = js.dump();

    if (!sendFramed(clientfd, buf))
    {
        cerr << "group chat msg error :" << buf << endl;
    }
}
void quit(int clientfd, string str)
{
    json js;
    js["msgid"] = MSG_LOGOUT;
    js["id"] = g_User.getId();
    string buf = js.dump();

    if (!sendFramed(clientfd, buf))
    {
        cerr << "send logout msg error :" << buf << endl;
    }
    else
    {
        isMain = false;
    }
}

string getCurrentTime()
{
    auto now = chrono::system_clock::now();
    time_t time = chrono::system_clock::to_time_t(now);
    struct tm *timeinfo = localtime(&time);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    return string(buffer);
}