// simple-cpp-logger-example.cpp: определяет точку входа для приложения.
//

#include "simple-cpp-logger-example.h"

using namespace std;

int main()
{
	scl::interface::ILoggerManager* manager = new scl::logger_manager::LoggerManager();
	manager->create_logger("example", std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::FileLogStrategy("./example.log", 0)));

	std::shared_ptr<scl::interface::ILogger> logger = manager->get_logger("example");
	logger->log_info("only file log");

	logger->add_strategy(std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::ConsoleLogStrategy(0)));

	logger->log_info("file and console log");

	logger->log_info("log with stack trace", true);

	return 0;
}
