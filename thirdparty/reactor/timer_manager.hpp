#ifndef _TIMER_MANAGER_HPP_
#define _TIMER_MANAGER_HPP_ 1

#include "timer.hpp"
#include <queue>
#include <unordered_map>

// 默认配置常量
inline const int default_alive_gap = 10;  // 默认存活周期（秒）

/**
 * @brief 定时器管理核心类
 * @note 最小堆 + 版本号惰性删除（只实现“固定 idle 超时”）
 */
class TimerManager
{
public:
    // 初始化存活周期（秒）
    explicit TimerManager(int gap = default_alive_gap) : alive_gap_(gap) {}

    /**
     * @brief 添加新连接定时器
     * @param sockfd 要管理的 socket fd
     */
    void Push(int sockfd) {
        const uint64_t ver = ++ver_[sockfd];
        const std::time_t exp = std::time(nullptr) + alive_gap_;
        latest_[sockfd] = exp;
        heap_.push(TimerEntry{exp, sockfd, ver});
    }

    /**
     * @brief 检查堆顶是否过期
     * @return true表示存在过期连接需要处理
     */
    bool IsTopExpired() {
        const std::time_t now = std::time(nullptr);
        while (!heap_.empty()) {
            const auto top = heap_.top();
            const auto it = latest_.find(top.fd);
            if (it == latest_.end()) { // 已 LazyDelete
                heap_.pop();
                continue;
            }
            // 旧版本节点：丢弃
            if (ver_[top.fd] != top.ver || it->second != top.expire_at) {
                heap_.pop();
                continue;
            }
            // 最新节点
            return top.expire_at <= now;
        }
        return false;
    }

    /// 惰性删除元素
    void LazyDelete(int sockfd) { latest_.erase(sockfd); }

    /// 获取堆顶过期 fd（调用前需确保 IsTopExpired() == true）
    int GetTopFd() const { return heap_.top().fd; }

    /**
     * @brief 更新连接活跃时间
     * @param sockfd 要更新的socket描述符
     */
    void UpdateTime(int sockfd) {
        if (latest_.find(sockfd) == latest_.end())
            return;
        const uint64_t ver = ++ver_[sockfd];
        const std::time_t exp = std::time(nullptr) + alive_gap_;
        latest_[sockfd] = exp;
        heap_.push(TimerEntry{exp, sockfd, ver});
    }

private:
    std::priority_queue<TimerEntry, std::vector<TimerEntry>, time_cmp> heap_;
    std::unordered_map<int, std::time_t> latest_; // fd -> 最新 expire
    std::unordered_map<int, uint64_t> ver_;       // fd -> 最新版本号
    int alive_gap_;
};

#endif