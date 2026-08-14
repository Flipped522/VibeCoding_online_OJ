# OJ 系统依赖安装指南

> 目标系统：空白 Ubuntu 22.04.5

---

## 一、依赖清单

| 依赖 | 用途 | 来源（SPEC 引用） |
|------|------|-------------------|
| build-essential | C++ 编译工具链（g++ 等） | Phase 3: "fork + g++" |
| cmake | 后端构建系统 | CMakeLists.txt |
| pkg-config | 库查找辅助 | CMake 依赖查找 |
| mysql-server | MySQL 8.0 数据库服务 | §5.1 数据库配置 |
| libmysqlclient-dev | MySQL C 客户端开发库 | db/connection_pool.cc |
| libyaml-cpp-dev | 解析 config.yaml 配置文件 | config/config.yaml |
| nlohmann-json3-dev | JSON 序列化/反序列化 | REST API 交互 |
| libbcrypt-dev | 密码 bcrypt 哈希 | §5: "bcrypt 哈希" |
| libssl-dev | OpenSSL 开发库（cpp-httplib 依赖） | cpp-httplib 编译需要 |
| git | 拉取 cpp-httplib 源码 | 架构图: "cpp-httplib" |
| cpp-httplib (header-only) | HTTP 服务器库 | 架构图: "cpp-httplib" |

---

## 二、一键安装

```bash
sudo apt update && sudo apt install -y \
    build-essential cmake pkg-config \
    mysql-server libmysqlclient-dev \
    libyaml-cpp-dev nlohmann-json3-dev \
    libbcrypt-dev libssl-dev git \
&& git clone https://github.com/yhirose/cpp-httplib.git /tmp/cpp-httplib \
&& sudo cp /tmp/cpp-httplib/include/httplib.h /usr/local/include/
```

---

## 三、分步安装

### 3.1 更新系统包索引

```bash
sudo apt update && sudo apt upgrade -y
```

### 3.2 安装编译工具链

```bash
sudo apt install -y build-essential cmake pkg-config
```

### 3.3 安装 MySQL 8.0 及开发库

```bash
sudo apt install -y mysql-server libmysqlclient-dev
```

### 3.4 安装 yaml-cpp（配置文件解析）

```bash
sudo apt install -y libyaml-cpp-dev
```

### 3.5 安装 nlohmann-json（JSON 处理）

```bash
sudo apt install -y nlohmann-json3-dev
```

### 3.6 安装 bcrypt（密码哈希）

```bash
sudo apt install -y libbcrypt-dev
```

> 若 `libbcrypt-dev` 不可用，执行 `apt-cache search bcrypt` 查找替代包名，或从源码编译：
> ```bash
> git clone https://github.com/trusch/libbcrypt.git /tmp/libbcrypt
> cd /tmp/libbcrypt && make && sudo make install
> ```

### 3.7 安装 OpenSSL 开发库

```bash
sudo apt install -y libssl-dev
```

### 3.8 安装 cpp-httplib（header-only，需手动获取）

```bash
sudo apt install -y git
git clone https://github.com/yhirose/cpp-httplib.git /tmp/cpp-httplib
sudo cp /tmp/cpp-httplib/include/httplib.h /usr/local/include/
```

---

## 四、MySQL 初始化

```bash
# 启动并设置开机自启
sudo systemctl start mysql
sudo systemctl enable mysql

# 创建用户（auth_socket 认证，按 SPEC §5.1）
sudo mysql -e "CREATE USER IF NOT EXISTS '$USER'@'localhost' IDENTIFIED WITH auth_socket; GRANT ALL ON *.* TO '$USER'@'localhost';"

# 验证连接
mysql -u $USER -e "SELECT 1;"

# 执行数据库初始化脚本
mysql -u $USER < database/init.sql
```

---

## 五、验证安装

```bash
# 编译器
g++ --version          # 期望 >= 11.x

# CMake
cmake --version        # 期望 >= 3.22

# MySQL
mysql --version        # 期望 8.0.x

# 验证开发库可被 pkg-config 发现
pkg-config --libs mysqlclient
pkg-config --libs yaml-cpp
pkg-config --libs openssl

# 验证 header-only 库
ls /usr/local/include/httplib.h
ls /usr/include/nlohmann/json.hpp
```
