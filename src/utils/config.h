#ifndef OJ_CONFIG_H
#define OJ_CONFIG_H

#include <string>
#include <map>

namespace OJ {

struct ServerConfig {
    std::string host;
    int port;
    int thread_pool_size;
};

struct DatabaseConfig {
    std::string host;
    int port;
    std::string username;
    std::string password;
    std::string database;
    int max_connections;
};

struct ExecutionConfig {
    int timeout_seconds;
    int max_memory_mb;
    std::string compile_command;
};

struct Config {
    ServerConfig server;
    DatabaseConfig database;
    ExecutionConfig execution;
};

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadFromFile(const std::string& filepath);
    bool loadFromYaml(const std::string& yaml_content);
    void reset();

    const Config& getConfig() const { return config_; }
    const ServerConfig& getServerConfig() const { return config_.server; }
    const DatabaseConfig& getDatabaseConfig() const { return config_.database; }
    const ExecutionConfig& getExecutionConfig() const { return config_.execution; }

private:
    ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool parseYaml(const std::string& content);
    std::string trim(const std::string& str);
    std::string stripInlineComment(const std::string& str);

    Config config_;
    bool loaded_;
};

} // namespace OJ

#endif // OJ_CONFIG_H