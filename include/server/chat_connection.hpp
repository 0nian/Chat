#ifndef CHAT_CONNECTION_HPP
#define CHAT_CONNECTION_HPP

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "connection.hpp"
#include "event_loop.hpp"
#include "net_frame.hpp"

class ChatConnection {
public:
  ChatConnection(std::shared_ptr<Connection> c, std::shared_ptr<EventLoop> el)
      : conn_(c), el_(el), id_(++s_next_id) {}

  void send(const std::string &json_body) {
    std::lock_guard<std::mutex> lk(mu_);
    auto c = conn_.lock();
    auto loop = el_.lock();
    if (!c || !loop)
      return;
    std::string wire = chat_net::Encode(json_body);
    c->AppendOutBuffer(wire);
    loop->Send(c);
  }

  uint64_t traceId() const { return id_; }

private:
  static std::atomic<uint64_t> s_next_id;
  std::weak_ptr<Connection> conn_;
  std::weak_ptr<EventLoop> el_;
  std::mutex mu_;
  uint64_t id_;
};

#endif
