#ifndef _TIMER_HPP_
#define _TIMER_HPP_ 1

#include <ctime>
#include <cstdint>

// 定时器堆节点（用于惰性删除：同一个 fd 会不断产生新版本的节点）
struct TimerEntry {
  std::time_t expire_at; // 绝对过期时间（秒）
  int fd;                // 关联的 socket fd
  uint64_t ver;          // 版本号（越大越新）
};

/**
 * @brief 定时器比较仿函数
 * @note 用于构建最小堆（最早过期时间在堆顶）
 */
struct time_cmp {
  bool operator()(const TimerEntry &a, const TimerEntry &b) const {
    return a.expire_at > b.expire_at; // 小顶堆
  }
};

#endif