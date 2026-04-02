#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include "log.hpp"
#include "net_frame.hpp"

#include <functional>

using json = nlohmann::json;

ChatServer *ChatServer::inst_ = nullptr;

ChatServer::ChatServer(uint16_t port, int worker_threads)
    : port_(port), workers_(worker_threads) {}

ChatServer::~ChatServer() {
  if (inst_ == this)
    inst_ = nullptr;
}

ChatServer *ChatServer::instance() { return inst_; }

void ChatServer::start() {
  inst_ = this;
  rq_ = std::make_shared<RingQueue<ClientInf>>(2048);
  threads_.emplace_back([this] { listenLoop(); });
  for (int i = 0; i < workers_; ++i)
    threads_.emplace_back([this] { workerLoop(); });
  for (auto &t : threads_)
    if (t.joinable())
      t.join();
}

void ChatServer::listenLoop() {
  auto lt = std::make_shared<Listener>(port_);
  lt->Init();
  auto baser = std::make_shared<EventLoop>(rq_);
  baser->AddConnection(lt->Fd(), EPOLLIN | EPOLLET,
                       std::bind(&Listener::Accepter, lt, std::placeholders::_1),
                       nullptr, nullptr, "0.0.0.0", 0, true);
  baser->Loop();
}

void ChatServer::workerLoop() {
  auto el = std::make_shared<EventLoop>(
      rq_,
      std::bind(&ChatServer::messageHandler, this, std::placeholders::_1),
      std::bind(&ChatServer::taskPush, this, std::placeholders::_1,
                std::placeholders::_2));
  el->Loop();
}

void ChatServer::taskPush(std::weak_ptr<RingQueue<ClientInf>> wrq,
                          std::weak_ptr<EventLoop> wel) {
  auto rq = wrq.lock();
  auto el = wel.lock();
  if (!rq || !el)
    return;
  if (auto client_inf = rq->Pop()) {
    el->AddConnection(
        client_inf->sockfd, EPOLLIN | EPOLLET,
        std::bind(&EventLoop::Recv, el, std::placeholders::_1),
        std::bind(&EventLoop::Send, el, std::placeholders::_1),
        [this, el](std::weak_ptr<Connection> w) {
          onDisconnect(w);
          el->Except(w);
        },
        client_inf->client_ip, client_inf->client_port);
  }
}

std::shared_ptr<ChatConnection>
ChatServer::getOrCreateChatConn(std::shared_ptr<Connection> conn,
                                std::shared_ptr<EventLoop> el) {
  int fd = conn->Sockfd();
  std::lock_guard<std::mutex> lk(fd_mtx_);
  auto it = fd_to_chat_.find(fd);
  if (it != fd_to_chat_.end())
    return it->second;
  auto cc = std::make_shared<ChatConnection>(conn, el);
  fd_to_chat_[fd] = cc;
  return cc;
}

void ChatServer::onDisconnect(std::weak_ptr<Connection> w) {
  auto c = w.lock();
  if (!c)
    return;
  int fd = c->Sockfd();
  std::shared_ptr<ChatConnection> cc;
  {
    std::lock_guard<std::mutex> lk(fd_mtx_);
    auto it = fd_to_chat_.find(fd);
    if (it != fd_to_chat_.end()) {
      cc = it->second;
      fd_to_chat_.erase(it);
    }
  }
  if (cc)
    ChatService::getInstance()->clientExceptionExit(cc);
}

void ChatServer::messageHandler(std::weak_ptr<Connection> wconn) {
  auto conn = wconn.lock();
  if (!conn)
    return;
  auto el = conn->el.lock();
  if (!el)
    return;
  auto chatConn = getOrCreateChatConn(conn, el);
  std::string &buf = conn->Inbuffer();
  std::string one;
  while (chat_net::Decode(buf, one)) {
    try {
      json js = json::parse(one);
      int msgid = js.at("msgid").get<int>();
      ChatService::getInstance()->getHander(msgid)(chatConn, js);
    } catch (const std::exception &e) {
      lg(Error, "message parse/handler error: %s", e.what());
    } catch (...) {
      lg(Error, "message parse/handler unknown error");
    }
  }
}
