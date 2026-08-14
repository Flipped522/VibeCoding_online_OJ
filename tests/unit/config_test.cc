#include "utils/config.h"

#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>
#include <unistd.h>

namespace OJ {

// 测试夹具：每个测试前 reset 单例，保证状态隔离
class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigManager::getInstance().reset();
    }

    void TearDown() override {
        ConfigManager::getInstance().reset();
        // 清理临时文件
        for (const auto& path : temp_files_) {
            std::remove(path.c_str());
        }
        temp_files_.clear();
    }

    // 辅助：在 /tmp 创建临时 YAML 文件，返回路径
    std::string createTempFile(const std::string& content) {
        static int counter = 0;
        std::string path = "/tmp/oj_config_test_" +
                           std::to_string(getpid()) + "_" +
                           std::to_string(counter++) + ".yaml";
        std::ofstream file(path);
        file << content;
        file.close();
        temp_files_.push_back(path);
        return path;
    }

    std::vector<std::string> temp_files_;
};

// ===================================================================
// 1. 默认值测试
// ===================================================================

// 测试 reset 后所有字段恢复默认值
TEST_F(ConfigTest, DefaultValuesAfterReset) {
    auto& mgr = ConfigManager::getInstance();
    const auto& cfg = mgr.getConfig();

    EXPECT_EQ(cfg.server.host, "0.0.0.0");
    EXPECT_EQ(cfg.server.port, 8080);
    EXPECT_EQ(cfg.server.thread_pool_size, 4);

    EXPECT_EQ(cfg.database.host, "localhost");
    EXPECT_EQ(cfg.database.port, 3306);
    EXPECT_EQ(cfg.database.username, "");
    EXPECT_EQ(cfg.database.password, "");
    EXPECT_EQ(cfg.database.database, "oj_system");
    EXPECT_EQ(cfg.database.max_connections, 10);

    EXPECT_EQ(cfg.execution.timeout_seconds, 5);
    EXPECT_EQ(cfg.execution.max_memory_mb, 256);
    EXPECT_EQ(cfg.execution.compile_command, "g++ -std=c++17 -O2");
}

// ===================================================================
// 2. 单例测试
// ===================================================================

// 测试 getInstance 返回同一实例
TEST_F(ConfigTest, SingletonReturnsSameInstance) {
    auto& inst1 = ConfigManager::getInstance();
    auto& inst2 = ConfigManager::getInstance();
    EXPECT_EQ(&inst1, &inst2);
}

// ===================================================================
// 3. loadFromYaml 测试
// ===================================================================

// 测试加载完整的 YAML（所有 section）
TEST_F(ConfigTest, LoadFromYamlAllSections) {
    std::string yaml = R"(
server:
  host: 127.0.0.1
  port: 9090
  thread_pool_size: 8

database:
  host: db.example.com
  port: 3307
  username: admin
  password: secret
  database: oj_prod
  max_connections: 20

execution:
  timeout_seconds: 10
  max_memory_mb: 512
  compile_command: g++ -std=c++20 -O3
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "127.0.0.1");
    EXPECT_EQ(cfg.server.port, 9090);
    EXPECT_EQ(cfg.server.thread_pool_size, 8);

    EXPECT_EQ(cfg.database.host, "db.example.com");
    EXPECT_EQ(cfg.database.port, 3307);
    EXPECT_EQ(cfg.database.username, "admin");
    EXPECT_EQ(cfg.database.password, "secret");
    EXPECT_EQ(cfg.database.database, "oj_prod");
    EXPECT_EQ(cfg.database.max_connections, 20);

    EXPECT_EQ(cfg.execution.timeout_seconds, 10);
    EXPECT_EQ(cfg.execution.max_memory_mb, 512);
    EXPECT_EQ(cfg.execution.compile_command, "g++ -std=c++20 -O3");
}

// 测试仅加载 server section，其余字段保持默认
TEST_F(ConfigTest, LoadFromYamlPartialServerOnly) {
    std::string yaml = R"(
server:
  host: 192.168.1.100
  port: 3000
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "192.168.1.100");
    EXPECT_EQ(cfg.server.port, 3000);
    EXPECT_EQ(cfg.server.thread_pool_size, 4);  // 默认值

    // database 保持默认
    EXPECT_EQ(cfg.database.host, "localhost");
    EXPECT_EQ(cfg.database.port, 3306);
    EXPECT_EQ(cfg.database.database, "oj_system");

    // execution 保持默认
    EXPECT_EQ(cfg.execution.timeout_seconds, 5);
    EXPECT_EQ(cfg.execution.max_memory_mb, 256);
}

// 测试仅加载 database section
TEST_F(ConfigTest, LoadFromYamlDatabaseOnly) {
    std::string yaml = R"(
database:
  host: db.test.com
  port: 3306
  username: tester
  password: pass123
  database: test_db
  max_connections: 5
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.database.host, "db.test.com");
    EXPECT_EQ(cfg.database.port, 3306);
    EXPECT_EQ(cfg.database.username, "tester");
    EXPECT_EQ(cfg.database.password, "pass123");
    EXPECT_EQ(cfg.database.database, "test_db");
    EXPECT_EQ(cfg.database.max_connections, 5);

    // server 保持默认
    EXPECT_EQ(cfg.server.host, "0.0.0.0");
    EXPECT_EQ(cfg.server.port, 8080);
}

// 测试仅加载 execution section
TEST_F(ConfigTest, LoadFromYamlExecutionOnly) {
    std::string yaml = R"(
execution:
  timeout_seconds: 15
  max_memory_mb: 1024
  compile_command: clang++ -std=c++17
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.execution.timeout_seconds, 15);
    EXPECT_EQ(cfg.execution.max_memory_mb, 1024);
    EXPECT_EQ(cfg.execution.compile_command, "clang++ -std=c++17");

    // 其余保持默认
    EXPECT_EQ(cfg.server.host, "0.0.0.0");
    EXPECT_EQ(cfg.database.host, "localhost");
}

// 测试空内容加载：返回 true，保持默认值
TEST_F(ConfigTest, LoadFromYamlEmptyContent) {
    auto& mgr = ConfigManager::getInstance();
    EXPECT_TRUE(mgr.loadFromYaml(""));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "0.0.0.0");
    EXPECT_EQ(cfg.server.port, 8080);
    EXPECT_EQ(cfg.database.host, "localhost");
    EXPECT_EQ(cfg.execution.timeout_seconds, 5);
}

// 测试带注释的 YAML
TEST_F(ConfigTest, LoadFromYamlWithComments) {
    std::string yaml = R"(
# Server configuration
server:
  host: 10.0.0.1    # bind address
  port: 8081
  # thread_pool_size: 16

database:
  host: localhost
  port: 3306
  username: root
  # password: hidden
  database: oj_test
  max_connections: 3
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "10.0.0.1");
    EXPECT_EQ(cfg.server.port, 8081);
    EXPECT_EQ(cfg.server.thread_pool_size, 4);  // 被注释，使用默认

    EXPECT_EQ(cfg.database.username, "root");
    EXPECT_EQ(cfg.database.password, "");  // 被注释，使用默认空
    EXPECT_EQ(cfg.database.database, "oj_test");
    EXPECT_EQ(cfg.database.max_connections, 3);
}

// 测试带大量空行的 YAML
TEST_F(ConfigTest, LoadFromYamlWithWhitespace) {
    std::string yaml = "\n\n   \nserver:\n\n   host: 0.0.0.0\n\n\n   port: 7777\n\n";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "0.0.0.0");
    EXPECT_EQ(cfg.server.port, 7777);
}

// 测试整数字段解析（port, thread_pool_size, max_connections, timeout, memory）
TEST_F(ConfigTest, IntegerFieldParsing) {
    std::string yaml = R"(
server:
  port: 12345
  thread_pool_size: 32

database:
  port: 33060
  max_connections: 50

execution:
  timeout_seconds: 30
  max_memory_mb: 2048
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.port, 12345);
    EXPECT_EQ(cfg.server.thread_pool_size, 32);
    EXPECT_EQ(cfg.database.port, 33060);
    EXPECT_EQ(cfg.database.max_connections, 50);
    EXPECT_EQ(cfg.execution.timeout_seconds, 30);
    EXPECT_EQ(cfg.execution.max_memory_mb, 2048);
}

// 测试 compile_command 包含空格和特殊参数
TEST_F(ConfigTest, CompileCommandWithSpaces) {
    std::string yaml = R"(
execution:
  compile_command: g++ -std=c++17 -O2 -Wall -Wextra -fsanitize=address
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    EXPECT_EQ(mgr.getExecutionConfig().compile_command,
              "g++ -std=c++17 -O2 -Wall -Wextra -fsanitize=address");
}

// ===================================================================
// 4. loadFromFile 测试
// ===================================================================

// 测试从有效文件加载
TEST_F(ConfigTest, LoadFromFileValid) {
    std::string yaml = R"(
server:
  host: 1.2.3.4
  port: 8888
  thread_pool_size: 16

database:
  host: mysql.local
  port: 3306
  username: oj_admin
  password: secure_pass
  database: oj_system
  max_connections: 15

execution:
  timeout_seconds: 8
  max_memory_mb: 512
  compile_command: g++ -O2
)";
    std::string path = createTempFile(yaml);

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromFile(path));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "1.2.3.4");
    EXPECT_EQ(cfg.server.port, 8888);
    EXPECT_EQ(cfg.server.thread_pool_size, 16);
    EXPECT_EQ(cfg.database.host, "mysql.local");
    EXPECT_EQ(cfg.database.username, "oj_admin");
    EXPECT_EQ(cfg.database.password, "secure_pass");
    EXPECT_EQ(cfg.database.database, "oj_system");
    EXPECT_EQ(cfg.database.max_connections, 15);
    EXPECT_EQ(cfg.execution.timeout_seconds, 8);
    EXPECT_EQ(cfg.execution.max_memory_mb, 512);
    EXPECT_EQ(cfg.execution.compile_command, "g++ -O2");
}

// 测试文件不存在：返回 false
TEST_F(ConfigTest, LoadFromFileNonExistent) {
    auto& mgr = ConfigManager::getInstance();
    EXPECT_FALSE(mgr.loadFromFile("/tmp/oj_nonexistent_config_file_99999.yaml"));

    // 默认值应保持不变
    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.port, 8080);
    EXPECT_EQ(cfg.database.host, "localhost");
}

// 测试空文件：返回 true，保持默认值
TEST_F(ConfigTest, LoadFromFileEmptyFile) {
    std::string path = createTempFile("");

    auto& mgr = ConfigManager::getInstance();
    EXPECT_TRUE(mgr.loadFromFile(path));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "0.0.0.0");
    EXPECT_EQ(cfg.server.port, 8080);
}

// ===================================================================
// 5. reset 测试
// ===================================================================

// 测试 reset 后恢复默认值
TEST_F(ConfigTest, ResetRestoresDefaults) {
    auto& mgr = ConfigManager::getInstance();

    // 加载自定义配置
    std::string yaml = R"(
server:
  host: 9.9.9.9
  port: 9999
  thread_pool_size: 99

database:
  host: far.db.com
  port: 9999
  username: custom
  password: custom_pass
  database: custom_db
  max_connections: 99

execution:
  timeout_seconds: 99
  max_memory_mb: 999
  compile_command: custom-compiler
)";
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    // 验证自定义值已生效
    EXPECT_EQ(mgr.getServerConfig().host, "9.9.9.9");
    EXPECT_EQ(mgr.getServerConfig().port, 9999);

    // 执行 reset
    mgr.reset();

    // 验证默认值恢复
    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "0.0.0.0");
    EXPECT_EQ(cfg.server.port, 8080);
    EXPECT_EQ(cfg.server.thread_pool_size, 4);
    EXPECT_EQ(cfg.database.host, "localhost");
    EXPECT_EQ(cfg.database.port, 3306);
    EXPECT_EQ(cfg.database.username, "");
    EXPECT_EQ(cfg.database.password, "");
    EXPECT_EQ(cfg.database.database, "oj_system");
    EXPECT_EQ(cfg.database.max_connections, 10);
    EXPECT_EQ(cfg.execution.timeout_seconds, 5);
    EXPECT_EQ(cfg.execution.max_memory_mb, 256);
    EXPECT_EQ(cfg.execution.compile_command, "g++ -std=c++17 -O2");
}

// ===================================================================
// 6. 多次加载测试
// ===================================================================

// 测试连续两次加载不同配置：第二次覆盖第一次
TEST_F(ConfigTest, MultipleLoadsOverwritePrevious) {
    auto& mgr = ConfigManager::getInstance();

    std::string yaml1 = R"(
server:
  host: 1.1.1.1
  port: 1111
database:
  host: db1.local
  database: first_db
execution:
  timeout_seconds: 3
)";

    ASSERT_TRUE(mgr.loadFromYaml(yaml1));
    EXPECT_EQ(mgr.getServerConfig().host, "1.1.1.1");
    EXPECT_EQ(mgr.getServerConfig().port, 1111);
    EXPECT_EQ(mgr.getDatabaseConfig().host, "db1.local");
    EXPECT_EQ(mgr.getDatabaseConfig().database, "first_db");
    EXPECT_EQ(mgr.getExecutionConfig().timeout_seconds, 3);

    std::string yaml2 = R"(
server:
  host: 2.2.2.2
  port: 2222
database:
  host: db2.local
  database: second_db
execution:
  timeout_seconds: 7
)";

    ASSERT_TRUE(mgr.loadFromYaml(yaml2));
    EXPECT_EQ(mgr.getServerConfig().host, "2.2.2.2");
    EXPECT_EQ(mgr.getServerConfig().port, 2222);
    EXPECT_EQ(mgr.getDatabaseConfig().host, "db2.local");
    EXPECT_EQ(mgr.getDatabaseConfig().database, "second_db");
    EXPECT_EQ(mgr.getExecutionConfig().timeout_seconds, 7);
}

// ===================================================================
// 7. Getter 方法测试
// ===================================================================

// 测试各 getter 返回正确引用
TEST_F(ConfigTest, GettersReturnCorrectValues) {
    std::string yaml = R"(
server:
  host: 5.5.5.5
  port: 5555
  thread_pool_size: 6
database:
  host: getter.db
  username: getter_user
  database: getter_db
  max_connections: 7
execution:
  timeout_seconds: 12
  max_memory_mb: 128
  compile_command: getter_compiler
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    // 测试 getServerConfig
    const auto& srv = mgr.getServerConfig();
    EXPECT_EQ(srv.host, "5.5.5.5");
    EXPECT_EQ(srv.port, 5555);
    EXPECT_EQ(srv.thread_pool_size, 6);

    // 测试 getDatabaseConfig
    const auto& db = mgr.getDatabaseConfig();
    EXPECT_EQ(db.host, "getter.db");
    EXPECT_EQ(db.username, "getter_user");
    EXPECT_EQ(db.database, "getter_db");
    EXPECT_EQ(db.max_connections, 7);

    // 测试 getExecutionConfig
    const auto& exec = mgr.getExecutionConfig();
    EXPECT_EQ(exec.timeout_seconds, 12);
    EXPECT_EQ(exec.max_memory_mb, 128);
    EXPECT_EQ(exec.compile_command, "getter_compiler");

    // 测试 getConfig 返回整体引用
    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "5.5.5.5");
    EXPECT_EQ(cfg.database.host, "getter.db");
    EXPECT_EQ(cfg.execution.compile_command, "getter_compiler");
}

// ===================================================================
// 8. 边界/边界场景测试
// ===================================================================

// 测试仅含 section 头、无键值对
TEST_F(ConfigTest, SectionHeaderOnly) {
    std::string yaml = R"(
server:
database:
execution:
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    const auto& cfg = mgr.getConfig();
    EXPECT_EQ(cfg.server.host, "0.0.0.0");
    EXPECT_EQ(cfg.database.host, "localhost");
    EXPECT_EQ(cfg.execution.timeout_seconds, 5);
}

// 测试值中包含冒号（如 compile_command 中的路径）
TEST_F(ConfigTest, ValueContainsColon) {
    std::string yaml = R"(
execution:
  compile_command: g++ -I/usr/local/include:hello
database:
  host: localhost:extra
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    // compile_command 应包含完整值（首个冒号之后的全内容）
    EXPECT_EQ(mgr.getExecutionConfig().compile_command, "g++ -I/usr/local/include:hello");
}

// 测试未知 section 被忽略
TEST_F(ConfigTest, UnknownSectionIgnored) {
    std::string yaml = R"(
unknown_section:
  foo: bar
  baz: qux

server:
  host: 7.7.7.7
  port: 7777
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    // 已知 section 正常解析
    EXPECT_EQ(mgr.getServerConfig().host, "7.7.7.7");
    EXPECT_EQ(mgr.getServerConfig().port, 7777);

    // 未知 section 不影响其他默认值
    EXPECT_EQ(mgr.getDatabaseConfig().host, "localhost");
    EXPECT_EQ(mgr.getExecutionConfig().timeout_seconds, 5);
}

// 测试同一 section 中字段被覆盖（最后一行生效）
TEST_F(ConfigTest, DuplicateKeysLastWins) {
    std::string yaml = R"(
server:
  host: first.host
  host: second.host
  port: 1000
  port: 2000
)";

    auto& mgr = ConfigManager::getInstance();
    ASSERT_TRUE(mgr.loadFromYaml(yaml));

    EXPECT_EQ(mgr.getServerConfig().host, "second.host");
    EXPECT_EQ(mgr.getServerConfig().port, 2000);
}

}  // namespace OJ
