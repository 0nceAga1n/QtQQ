# QtQQ 仿QQ即时通讯系统

基于 Qt 框架的 C/S 架构即时通讯软件，包含客户端与服务端，支持登录认证、单聊、群聊、图文混排消息、文件传输、离线消息等功能。

## 技术栈

- **语言/框架**：C++、Qt（信号槽、自定义控件、QSS、QWebEngineView、QSqlDatabase）
- **网络**：TCP 长连接 + 长度前缀分帧
- **协议**：JSON 结构化消息（`cmd` / `sender` / `receiver` / `segments` 段落数组）
- **存储**：MySQL（服务端持久化）

## 功能特性

- 登录认证：token 鉴权 + 密码加盐 SHA-256 哈希
- 单聊 / 群聊（公司群、部门群）
- 图文混排消息（文字 + 表情包，一条消息可同时包含多段）
- 文件传输：支持任意类型文件，Base64 编码传输，点击气泡即可下载打开
- 离线消息：服务端落库，用户上线/打开窗口时拉取历史
- 消息可靠性：TCP 定向转发替代 UDP 广播，长度前缀分帧解决粘包/拆包
- 动态气泡渲染：新增员工后无需更新 JS 脚本即可正常显示
- 客户端去数据库化：所有数据由服务端接口提供，客户端可独立打包发行
- 群聊按成员路由：仅群成员可收到群消息
- 界面：QWebEngineView 渲染气泡、系统托盘、皮肤切换、圆形头像等

## 架构

```
客户端 (QtQQ)                    服务端 (QtQQ_Server)
┌─────────────────┐            ┌──────────────────────┐
│  Qt GUI 界面     │   TCP     │  QTcpServer 连接池     │
│  QWebEngineView  │ ◄────────► │  分帧解码 (Decoder)     │
│  气泡渲染         │   JSON    │  token 鉴权             │
│  SQLite/缓存      │            │  定向路由 / 成员路由    │
└─────────────────┘            │  MySQL 消息持久化       │
                                └──────────────────────┘
```

- **客户端**：不直连业务数据库，登录、联系人、历史消息等数据均由服务端接口提供。
- **服务端**：维护「员工ID ↔ 连接」映射，按接收者/群成员定向转发，所有消息落库。

## 目录结构

```
QTProject/
├── QtQQ/                  # 客户端（Qt 工程）
│   ├── QtQQ/              # 源码、资源、UI 文件
│   │   ├── Resources/     # 图片、QSS、表情、气泡模板
│   │   └── *.cpp / *.h    # 各模块源码
│   └── QtQQ.sln
├── QtQQ_Server/           # 服务端（Qt 工程）
│   ├── QtQQ_Server/       # 源码（tcpserver、tcpsocket、协议等）
│   └── QtQQ_Server.sln
├── .gitignore
└── README.md
```

## 核心模块

| 模块 | 说明 |
|---|---|
| `msgprotocol.h` | 统一通信协议：分帧（长度前缀）、消息打包、`isValidMsg` 校验 |
| `tcpserver.cpp` | 连接池、登录鉴权、消息定向路由、历史拉取、落库 |
| `tcpsocket.cpp` | 单个连接的分帧解码 |
| `talkwindowshell.cpp` | 聊天主窗口：收发消息、渲染气泡、拉取历史/会话信息 |
| `msgwebview.cpp` | 基于 QWebEngineView 的气泡渲染 + C++/JS 交互 + 文件下载 |
| `passwordutils.h` | 密码加盐哈希（SHA-256） |

## 环境依赖

- Qt 5.15 及以上（使用 QWebEngineView、QSqlDatabase 模块）
- MySQL 5.7 及以上
- MSVC 编译器（Visual Studio）

## 数据库

### 消息表（需手动创建）

```sql
CREATE TABLE IF NOT EXISTS tab_message (
    msg_id     INT AUTO_INCREMENT PRIMARY KEY,
    sender     VARCHAR(20) NOT NULL,        -- 发送者员工ID
    receiver   VARCHAR(20) NOT NULL,        -- 接收者：单聊=员工ID，群聊=群ID
    is_group   TINYINT     NOT NULL DEFAULT 0,  -- 0=单聊 1=群聊
    segments   LONGTEXT    NOT NULL,        -- 消息内容（JSON 字符串）
    send_time  DATETIME    NOT NULL,        -- 发送时间
    is_read    TINYINT     NOT NULL DEFAULT 0   -- 0=未读 1=已读
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

其余表（`tab_accounts`、`tab_employees`、`tab_department`）为既有结构，字段如下：

- `tab_accounts`：`employeeID`、`account`、`code`（密码哈希，`VARCHAR(64)`）
- `tab_employees`：`departmentID`、`employeeID`、`employee_name`、`employee_sign`、`status`、`picture`
- `tab_department`：`departmentID`、`department_name`、`sign`、`picture`

> 若从旧版本升级，密码需迁移为哈希：`UPDATE tab_accounts SET code = SHA2(CONCAT(code, 'QtQQ_Salt_2024'), 256);`

## 构建运行

1. 用 Visual Studio 分别打开 `QtQQ/QtQQ.sln` 和 `QtQQ_Server/QtQQ_Server.sln` 编译。
2. 确认服务端 `tcpserver.cpp` 中的 MySQL 连接信息正确。
3. 先启动服务端，再启动客户端。
4. 客户端登录（默认端口 `8888`，服务端地址当前为 `127.0.0.1`）。

## 优化记录

从简易版到当前版本的主要改造：

| 方向 | 改造内容 |
|---|---|
| 消息可靠性 | UDP 广播 → TCP 长连接 + 长度前缀分帧 + 定向路由 |
| 消息格式 | 自定义字符串 → JSON 结构化消息（图文混排） |
| 文件传输 | 仅 txt → 任意类型（Base64 + 点击气泡打开） |
| 消息持久化 | 无 → 服务端落库 + 离线拉取 |
| 客户端架构 | 直连数据库 → 全量去数据库化（服务端统一提供数据） |
| 气泡渲染 | 静态生成 JS → 动态通用渲染 |
| 安全 | 加 token 鉴权、SQL 参数化、密码加盐哈希、群聊按成员路由 |

## 后续规划

- [ ] 文件存磁盘、数据库存路径（当前 Base64 存库）
- [ ] 大文件分块传输 + 进度条
- [ ] 随机盐 + bcrypt 密码方案
- [ ] 历史消息分页 + 未读角标
- [ ] 服务端地址配置文件化
