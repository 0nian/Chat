#ifndef NET_FRAME_HPP
#define NET_FRAME_HPP

#include <cstddef>
#include <string>

namespace chat_net
{

  inline const char *kFrameSep = "\n";
  inline constexpr size_t kMaxFrameBodyLen = 64 * 1024; // 64KiB，防止恶意 length 导致内存膨胀

  enum class DecodeStatus
  {
    Ok,
    NeedMore,
    ProtocolError,
    TooLarge,
  };

  // 格式: "长度\n内容\n"（与 Reactor protocol 一致，便于 TCP 粘包切分）
  inline DecodeStatus DecodeOne(std::string &package, std::string &content)
  {
    size_t pos = package.find('\n');
    if (pos == std::string::npos)
    return DecodeStatus::NeedMore;
    size_t body_len = 0;
    try
    {
      body_len = static_cast<size_t>(std::stoul(package.substr(0, pos)));
    }
    catch (...)
    {
      return DecodeStatus::ProtocolError;
    }
    if (body_len > kMaxFrameBodyLen)
      return DecodeStatus::TooLarge;

    const size_t total_len = pos + 1 + body_len + 1;
    if (package.size() < total_len)
      return DecodeStatus::NeedMore;
    if (package[pos + 1 + body_len] != '\n')
    {
      return DecodeStatus::ProtocolError;
    }
    content = package.substr(pos + 1, body_len);
    package.erase(0, total_len);
    return DecodeStatus::Ok;
  }

  // 兼容旧接口：仅返回是否成功解出一帧；错误会清空缓冲区（调用方无法区分错误类型）。
  inline bool Decode(std::string &package, std::string &content)
  {
    auto st = DecodeOne(package, content);
    if (st == DecodeStatus::Ok)
      return true;
    if (st == DecodeStatus::ProtocolError || st == DecodeStatus::TooLarge)
      package.clear();
    return false;
  }

  inline std::string Encode(const std::string &content)
  {
    std::string ret = std::to_string(content.size());
    ret += kFrameSep;
    ret += content;
    ret += kFrameSep;
    return ret;
  }

} // namespace chat_net

#endif
