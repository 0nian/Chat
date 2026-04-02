#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "chat_connection.hpp"
#include "event_loop.hpp"
#include "listener.hpp"

class ChatServer {
public:
  ChatServer(uint16_t port, int worker_threads = 4);
  ~ChatServer();

  void start();

  static ChatServer *instance();

  std::shared_ptr<ChatConnection> getOrCreateChatConn(
      std::shared_ptr<Connection> conn, std::shared_ptr<EventLoop> el);
  void onDisconnect(std::weak_ptr<Connection> w);
  void messageHandler(std::weak_ptr<Connection> wconn);
  void taskPush(std::weak_ptr<RingQueue<ClientInf>> wrq,
                std::weak_ptr<EventLoop> wel);

  void listenLoop();

private:
  void workerLoop();

  static ChatServer *inst_;
  uint16_t port_;
  int workers_;
  std::shared_ptr<RingQueue<ClientInf>> rq_;
  std::vector<std::thread> threads_;
  std::mutex fd_mtx_;
  std::unordered_map<int, std::shared_ptr<ChatConnection>> fd_to_chat_;
};

#endif
