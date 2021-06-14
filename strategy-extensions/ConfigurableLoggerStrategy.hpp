#ifndef configurable_logger_strategy
#define configurable_logger_strategy

#include "../include/SimpleCppLogger.hpp"
#include <memory>

namespace scl::extensions {

	class ConfigurableLoggerStrategy : public scl::interface::ILogStrategy
	{
	private:
		std::vector<std::shared_ptr<scl::interface::ILogStrategy>> _log_strategy_collection {};
	public:
		ConfigurableLoggerStrategy(std::string configuration_file_path);

		auto log(msg::LogMsg log_msg) -> void override;

        auto flush() -> void override;
	};

}

#endif configurable_logger_strategy