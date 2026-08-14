#include "config.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace OJ {

ConfigManager::ConfigManager() : loaded_(false) {
    config_.server.host = "0.0.0.0";
    config_.server.port = 8080;
    config_.server.thread_pool_size = 4;

    config_.database.host = "localhost";
    config_.database.port = 3306;
    config_.database.username = "";
    config_.database.password = "";
    config_.database.database = "oj_system";
    config_.database.max_connections = 10;

    config_.execution.timeout_seconds = 5;
    config_.execution.max_memory_mb = 256;
    config_.execution.compile_command = "g++ -std=c++17 -O2";
}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return loadFromYaml(ss.str());
}

bool ConfigManager::loadFromYaml(const std::string& yaml_content) {
    return parseYaml(yaml_content);
}

void ConfigManager::reset() {
    config_.server.host = "0.0.0.0";
    config_.server.port = 8080;
    config_.server.thread_pool_size = 4;

    config_.database.host = "localhost";
    config_.database.port = 3306;
    config_.database.username = "";
    config_.database.password = "";
    config_.database.database = "oj_system";
    config_.database.max_connections = 10;

    config_.execution.timeout_seconds = 5;
    config_.execution.max_memory_mb = 256;
    config_.execution.compile_command = "g++ -std=c++17 -O2";

    loaded_ = false;
}

std::string ConfigManager::trim(const std::string& str) {
    size_t start = 0;
    while (start < str.length() && std::isspace(str[start])) {
        start++;
    }

    size_t end = str.length();
    while (end > start && std::isspace(str[end - 1])) {
        end--;
    }

    return str.substr(start, end - start);
}

std::string ConfigManager::stripInlineComment(const std::string& str) {
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '#' && i > 0 && std::isspace(str[i - 1])) {
            return str.substr(0, i);
        }
    }
    return str;
}

bool ConfigManager::parseYaml(const std::string& content) {
    std::map<std::string, std::map<std::string, std::string>> sections;
    std::string current_section;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line.back() == ':') {
            current_section = line.substr(0, line.length() - 1);
            current_section = trim(current_section);
            continue;
        }

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = trim(line.substr(0, colon_pos));
            std::string value = trim(stripInlineComment(line.substr(colon_pos + 1)));

            if (!current_section.empty()) {
                sections[current_section][key] = value;
            }
        }
    }

    auto it = sections.find("server");
    if (it != sections.end()) {
        if (it->second.count("host")) config_.server.host = it->second["host"];
        if (it->second.count("port")) config_.server.port = std::stoi(it->second["port"]);
        if (it->second.count("thread_pool_size")) config_.server.thread_pool_size = std::stoi(it->second["thread_pool_size"]);
    }

    it = sections.find("database");
    if (it != sections.end()) {
        if (it->second.count("host")) config_.database.host = it->second["host"];
        if (it->second.count("port")) config_.database.port = std::stoi(it->second["port"]);
        if (it->second.count("username")) config_.database.username = it->second["username"];
        if (it->second.count("password")) config_.database.password = it->second["password"];
        if (it->second.count("database")) config_.database.database = it->second["database"];
        if (it->second.count("max_connections")) config_.database.max_connections = std::stoi(it->second["max_connections"]);
    }

    it = sections.find("execution");
    if (it != sections.end()) {
        if (it->second.count("timeout_seconds")) config_.execution.timeout_seconds = std::stoi(it->second["timeout_seconds"]);
        if (it->second.count("max_memory_mb")) config_.execution.max_memory_mb = std::stoi(it->second["max_memory_mb"]);
        if (it->second.count("compile_command")) config_.execution.compile_command = it->second["compile_command"];
    }

    loaded_ = true;
    return true;
}

} // namespace OJ