# Chat 即时通讯服务端 — 部署与运行说明

基于 **自研多 Reactor（epoll 边缘触发 + 主从线程 + 环形队列）** 的 TCP 聊天服务：用户注册登录、好友与群组、单聊/群聊、离线消息入库，以及通过 **Redis 发布订阅** 在同一用户连到不同节点时转发消息。客户端与服务端之间使用 **JSON 业务报文**，外层采用 **`长度\n正文\n`** 的定界帧（见 `include/server/net_frame.hpp`），与裸 TCP 粘包兼容。

---

## 目录

- [环境要求](#环境要求)
- [依赖服务：MySQL](#依赖服务mysql)
- [依赖服务：Redis](#依赖服务redis)
- [编译](#编译)
- [单机运行](#单机运行)
- [配置说明（数据库与 Redis 地址）](#配置说明数据库与-redis-地址)
- [可选：Nginx TCP 负载均衡（多实例）](#可选nginx-tcp-负载均衡多实例)
- [进程与信号](#进程与信号)
- [常见问题](#常见问题)

---

## 环境要求

| 项目 | 说明 |
|------|------|
| 操作系统 | **Linux**（或 WSL2）。服务端使用 `epoll`，需在类 Unix 环境编译运行。 |
| 编译器 | 支持 **C++17**（如 `g++ 7+`）。 |
| 构建 | **CMake 3.x**。 |
| 系统库 | **pthread**（一般随编译器）。 |
| 开发包 | **libmysqlclient**（MySQL C API）、**hiredis**（Redis C 客户端）。 |

**Debian / Ubuntu 示例：**

```bash
sudo apt update
sudo apt install -y build-essential cmake \
  libmysqlclient-dev libhiredis-dev
```

**Fedora 示例：**

```bash
sudo dnf install -y gcc-c++ cmake mysql-devel hiredis-devel
```

> 网络栈在 `thirdparty/reactor/`。

---

## 依赖服务：MySQL

### 连接参数（与源码一致）

当前写死在 `src/server/db/db.cpp`：

- 主机：`127.0.0.1`
- 端口：`3306`
- 用户：`root`
- 密码：`123456`
- 数据库名：`chat`

生产环境请修改该文件后重新编译，或自行改为配置文件读取。

### 创建数据库与表

下列 DDL 与 `src/server/model/*.cpp` 中的 SQL 字段顺序一致，可直接初始化。

```sql
CREATE DATABASE IF NOT EXISTS chat
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE chat;

-- 用户：id 自增；注册时写入 name, password, state
CREATE TABLE IF NOT EXISTS user (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(50) NOT NULL,
  password VARCHAR(50) NOT NULL,
  state VARCHAR(20) NOT NULL DEFAULT 'offline'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 好友：userid -> friendid
CREATE TABLE IF NOT EXISTS friend (
  userid INT NOT NULL,
  friendid INT NOT NULL,
  PRIMARY KEY (userid, friendid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 群组
CREATE TABLE IF NOT EXISTS allgroup (
  id INT AUTO_INCREMENT PRIMARY KEY,
  groupname VARCHAR(50) NOT NULL,
  groupdesc VARCHAR(200) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 群成员：groupid, userid, grouprole（如 creator / normal）
CREATE TABLE IF NOT EXISTS groupuser (
  groupid INT NOT NULL,
  userid INT NOT NULL,
  grouprole VARCHAR(20) NOT NULL,
  PRIMARY KEY (groupid, userid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 离线消息：可同一 userid 多行
CREATE TABLE IF NOT EXISTS offlinemessage (
  userid INT NOT NULL,
  message TEXT NOT NULL,
  KEY idx_userid (userid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

导入示例：

```bash
mysql -u root -p < init_chat.sql
```

连接成功后，服务端日志中会打印 MySQL 连接成功（使用 Reactor 自带 `log.hpp` 输出到终端）。

业务访问数据库时，在 `MySQL` 封装中统一使用 **`mysql_stmt_*` 预处理语句**（`db.cpp`），避免字符串拼接导致 **SQL 注入** 与内容中的单引号等问题；连接建立后会执行 `set names utf8mb4`，与上面建表的 `utf8mb4` 一致。

---

## 依赖服务：Redis

当前写死在 `src/server/redis/redis.cpp`：

- 地址：`127.0.0.1`
- 端口：`6379`

服务端会建立 **两个** Redis 连接（一个用于 `PUBLISH`，一个用于 `SUBSCRIBE`），并在独立线程中处理订阅消息。单机也必须启动 Redis，否则订阅初始化失败，跨连接实例转发不可用。

**安装并启动（Ubuntu）：**

```bash
sudo apt install -y redis-server
sudo systemctl enable --now redis-server
redis-cli ping   # 应返回 PONG
```

---

## 编译

在 **`Chat` 目录**（本 README 所在目录）执行：

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

成功后可执行文件位于：

- `Chat/bin/ChatServer`
- `Chat/bin/ChatClient`

若 CMake 找不到 MySQL / hiredis，请确认已安装对应的 `-dev` 包，必要时在 `CMakeLists.txt` 中为 `target_link_libraries` 增加 `link_directories` 指向库路径。

---

## 单机运行

### 1. 启动依赖

```bash
# MySQL、Redis 已运行，且已建好 chat 库与表
sudo systemctl status mysql redis-server   # 或你的发行版等价命令
```

### 2. 启动服务端

```bash
cd /path/to/Chat/bin
./ChatServer 0.0.0.0 6000
```

- 第一个参数：历史兼容保留；当前监听实现绑定 **全接口**（`INADDR_ANY`），与端口有关。
- 第二个参数：**监听端口**（示例 `6000`）。

### 3. 启动客户端

必须使用 **当前工程编译的客户端**（已实现与服务器一致的帧协议）：

```bash
./ChatClient 127.0.0.1 6000
```

按菜单进行注册、登录后使用 `chat`、`addfriend`、`groupchat` 等命令（输入 `help` 查看说明）。

---

## 配置说明（数据库与 Redis 地址）

| 配置项 | 文件 | 修改后 |
|--------|------|--------|
| MySQL 主机/用户/密码/库名 | `src/server/db/db.cpp` | 需 **重新编译** |
| Redis 地址与端口 | `src/server/redis/redis.cpp` | 需 **重新编译** |
| 工作线程数 | `src/server/main.cpp` 中 `ChatServer server(port, 4)` 的 `4` | 需 **重新编译** |
| 环形队列容量 | `src/server/chatserver.cpp` 中 `RingQueue<ClientInf>(2048)` | 需 **重新编译** |
| 连接空闲超时 | `thirdparty/reactor/event_loop.hpp` 中 `TimerManager(600)`（秒） | 需 **重新编译** |

---

## 可选：Nginx TCP 负载均衡（多实例）

多机或多进程部署时，可在前层使用 **Nginx `stream` 模块** 做 TCP 转发，后端为多个 `ChatServer` 监听端口。业务要求 **同一用户会话粘在同一节点** 时，需使用 **一致性哈希或 ip_hash** 等策略（视 Nginx 版本与模块而定）；否则跨节点用户依赖 **Redis PUB/SUB** 转发（本仓库已实现按用户 ID 频道订阅）。

示例（仅供参考，路径与端口按实际修改）：

```nginx
stream {
    upstream chat_backend {
        server 127.0.0.1:6001;
        server 127.0.0.1:6002;
        # 可选：hash $remote_addr consistent;
    }
    server {
        listen 6000;
        proxy_pass chat_backend;
        proxy_timeout 1d;
    }
}
```

每个后端节点均需连接 **同一 MySQL、同一 Redis**，否则会话与离线数据会不一致。

---

## 进程与信号

- `ChatServer` 主线程会 **join** 监听线程与多个 Reactor 工作线程，进程为长驻。
- 收到 **`SIGINT`（Ctrl+C）** 时，`main.cpp` 中注册的处理函数会调用 `ChatService::reset()`，将数据库中在线用户状态置为离线，然后 `exit(0)`。

---

## 常见问题

**1. 编译报错找不到 `mysql.h` / `hiredis/hiredis.h`**  
安装 `libmysqlclient-dev`、`libhiredis-dev`（或发行版对应包名）。

**2. 服务端启动后无法登录**  
检查 MySQL 是否监听、`chat` 库表是否已创建、账号密码是否与 `db.cpp` 一致。

**3. 跨机收不到消息**  
确认各节点 Redis 可达且 `redis.cpp` 中地址一致；防火墙放行 Redis 与业务端口。

**4. 旧客户端连不上**  
协议已改为 **带帧的 TCP**，必须使用本仓库当前版本的 `ChatClient`。

**5. 高并发 accept 丢连接**  
环形队列默认为 **非阻塞** 满则丢弃，可适当增大 `chatserver.cpp` 中队列容量或改为阻塞策略（需改 `thirdparty/reactor/ring_queue.hpp` 实现）。

---

## 项目结构（与部署相关）

```
Chat/
├── CMakeLists.txt
├── bin/                    # 可执行文件输出目录（cmake 生成）
├── include/server/         # 业务头文件、net_frame、chat_connection
├── src/server/             # ChatServer、ChatService、db、redis、model
├── src/client/             # 命令行客户端
└── thirdparty/
    ├── json.hpp
    └── reactor/            # epoll / EventLoop / Listener / RingQueue 等
```

---

## 许可证与致谢

数据结构及业务逻辑若来自课程或开源示例，请遵循原许可；网络栈为自研 Reactor 裁剪版，与 `Reactor/` 示例目录可分别维护。

如有问题，请结合 **MySQL 错误日志、Redis `redis-cli MONITOR`、服务端终端 `lg` 日志** 排查。
