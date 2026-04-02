#ifndef NET_FRAME_HPP
#define NET_FRAME_HPP

#include <string>

namespace chat_net {

inline const char *kFrameSep = "\n";

// 格式: "长度\n内容\n"（与 Reactor protocol 一致，便于 TCP 粘包切分）
inline bool Decode(std::string &package, std::string &content) {
  size_t pos = package.find('\n');
  if (pos == std::string::npos)
    return false;
  size_t body_len = 0;
  try {
    body_len = static_cast<size_t>(std::stoul(package.substr(0, pos)));
  } catch (...) {
    package.clear();
    return false;
  }
  const size_t total_len = pos + 1 + body_len + 1;
  if (package.size() < total_len)
    return false;
  if (package[pos + 1 + body_len] != '\n') {
    package.clear();
    return false;
  }
  content = package.substr(pos + 1, body_len);
  package.erase(0, total_len);
  return true;
}

inline std::string Encode(const std::string &content) {
  std::string ret = std::to_string(content.size());
  ret += kFrameSep;
  ret += content;
  ret += kFrameSep;
  return ret;
}

} // namespace chat_net

#endif
