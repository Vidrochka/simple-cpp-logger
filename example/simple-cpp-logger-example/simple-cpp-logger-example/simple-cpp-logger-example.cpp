// simple-cpp-logger-example.cpp: определяет точку входа для приложения.
//

#include "simple-cpp-logger-example.h"

using namespace std;

int main()
{
	try
	{
		setlocale(LC_ALL, "en_US.UTF-8");

		scl::interface::ILoggerManager* manager = new scl::logger_manager::LoggerManager();
		manager->create_logger("example", std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::FileLogStrategy("./example.log", std::shared_ptr<scl::msg::LogLevelMsgFormat>(new scl::msg::LogLevelMsgFormat{}), 0)), true);

		std::shared_ptr<scl::interface::ILogger> logger = manager->get_logger("example");
		logger->log_info("only file log");

		logger->add_strategy(std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::ConsoleLogStrategy(std::shared_ptr<scl::msg::LogLevelMsgFormat>(new scl::msg::LogLevelMsgFormat{}), 0)));

		logger->log_debug("file and console log debug");
		logger->log_info("file and console log info");
		logger->log_error("file and console log err");

		logger->log_info("log with stack trace", true);

		manager->create_logger("configurable", std::shared_ptr<scl::interface::ILogStrategy>(new scl::extensions::ConfigurableLoggerStrategy("./logger.cfg.json")), true);
		std::shared_ptr<scl::interface::ILogger> configurable_logger = manager->get_logger("configurable");
		configurable_logger->log_info("Configurable log info");
		configurable_logger->log_debug("Configurable log dbg", true);
	}
	catch (std::exception ex)
	{
		std::cout << ex.what() << std::endl;
	}
	return 0;
}
