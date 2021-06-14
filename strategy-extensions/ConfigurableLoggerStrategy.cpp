#include "ConfigurableLoggerStrategy.hpp"
#include "../third-party-libs/json.hpp"
#include <iostream>
#include <vector>
#include <regex>
#include <ctime>

namespace scl::extensions {

    ConfigurableLoggerStrategy::ConfigurableLoggerStrategy(std::string configuration_file_path) {
        std::ifstream cfg_file_stream(configuration_file_path);

        if(!cfg_file_stream.is_open())
            throw exceptions::config_file_unavailable(configuration_file_path);

        json cfg;
        cfg_file_stream >> cfg;

        if (cfg.contains("file")) {
            for (auto& file_cfg : cfg["file"].items()) {    
                _log_strategy_collection.push_back(std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::FileLogStrategy(file_cfg.value())));
            }
        }

        if (cfg.contains("console")) {
            for (auto& file_cfg : cfg["console"].items()) {
                _log_strategy_collection.push_back(std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::ConsoleLogStrategy(file_cfg.value())));
            }
        }
    }

    auto ConfigurableLoggerStrategy::log(msg::LogMsg log_msg) -> void {
        for(auto &logger : _log_strategy_collection)
            logger->log(log_msg);
    }

    auto ConfigurableLoggerStrategy::flush() -> void {
        for(auto &logger : _log_strategy_collection)
            logger->flush();
    }
}