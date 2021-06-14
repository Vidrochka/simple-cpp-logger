#ifndef simple_cpp_logger
#define simple_cpp_logger

#include "../third-party-libs/boost/stacktrace.hpp"

#include "../third-party-libs/json.hpp"
using json = nlohmann::json;

#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <mutex>
#include <map>
#include <exception>
#include <iostream>
#include <regex>

#undef interface

#define TO_STRING(obj) std::string(#obj)
#define FMT_REGEX(name, regex_str) static const std::regex name {regex_str};

namespace scl
{
	namespace exceptions
	{
		class log_file_unawalable_exception : public std::exception {
		public:
			log_file_unawalable_exception(std::string description, std::string file_name) :
				std::exception((description + ":" + file_name).c_str()) {}
		};

		class logger_context_already_exist_exception : public std::exception {
		public:
			logger_context_already_exist_exception(std::string error) : std::exception(error.c_str()) {}
		};

		class config_file_unavailable : public std::exception {
		public:
			config_file_unavailable(std::string file_path) :
				std::exception(("Config file [" + file_path + "] unavailable").c_str()) {}
		};

		class incorrect_config_format : public std::exception {
		public:
			incorrect_config_format(std::string description) :
				std::exception(description.c_str()) {}
		};
	}

	namespace utils
	{
		auto _get_datetime_prefix()->std::string;
	}

	namespace msg
	{
		enum class EVENT_TYPE : std::uint_fast8_t
		{
			LOG_DEBUG = 0,
			LOG_INFO = 1,
			LOG_WARNING = 2,
			LOG_ERROR = 3,
			LOG_FATAL = 4,
		};

		struct LogMsg
		{
			std::string msg;
			EVENT_TYPE event_type;
			bool has_error_code = false;
			int error_code;
			bool has_stack_trace = false;
			boost::stacktrace::stacktrace stack_trace;
		};

		struct MsgFormat {
			std::string simple;
			std::string simple_with_stacktrace;

			MsgFormat(json& msg_format_cfg);
			MsgFormat(
				std::string simple = "{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Green}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}",
				std::string simple_with_stacktrace = "{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Green}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
			);
		};

		struct LogLevelMsgFormat {
			MsgFormat debug = MsgFormat{};
			MsgFormat info = MsgFormat{};
			MsgFormat warning = MsgFormat{};
			MsgFormat error = MsgFormat{};
			MsgFormat fatal = MsgFormat{};
			LogLevelMsgFormat(json& log_level_msg_format_cfg);
			LogLevelMsgFormat(
				MsgFormat debug_format = MsgFormat{
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Grey}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}",
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Grey}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
				},
				MsgFormat info_format = MsgFormat{
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Green}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}",
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Green}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
				
				},
				MsgFormat warning_format = MsgFormat{
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Orange}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}",
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Orange}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
				},
				MsgFormat error_format = MsgFormat{
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Red}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}",
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Red}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
				},
				MsgFormat fatal_format = MsgFormat{
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Red}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Red} -> \"error_code\": {errcode}{nl}",
					"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Red}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Red} -> \"error_code\": {errcode}{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
				});

			auto get_format(LogMsg& log_msg)->std::string;
		};
	}

	namespace interface
	{
		class ILogStrategy
		{
		private:
			ILogStrategy(ILogStrategy&) = delete;
			ILogStrategy& operator=(const ILogStrategy&) = delete;
		public:
			ILogStrategy() {};
			virtual ~ILogStrategy() {}
			virtual auto log(msg::LogMsg log_msg) -> void = 0;
			virtual auto flush() -> void = 0;
		};
	}

	namespace context
	{
		struct LoggerContext
		{
		public:
			LoggerContext() {}
			LoggerContext(std::shared_ptr<interface::ILogStrategy> strategy) : strategy_list(std::vector{ strategy }) {
			}
			~LoggerContext() {}
			std::vector<std::shared_ptr<interface::ILogStrategy>> strategy_list;
			std::mutex write_mutex{};
		};
	}

	namespace interface
	{
		class ILogFormatter {
		public:
			ILogFormatter() {};
			virtual ~ILogFormatter() {};
			virtual auto format(std::string& log_text, msg::LogMsg log_msg) -> void = 0;
			virtual auto get_search_regex() -> std::string = 0;
		};

		class ILogger
		{
		protected:
			std::shared_ptr<context::LoggerContext> _logger_context;

		private:
			ILogger() = delete;
			ILogger(ILogger&) = delete;
			ILogger& operator=(const ILogger&) = delete;

		public:
			ILogger(std::shared_ptr<context::LoggerContext> context) {
				_logger_context = context;
			}
			virtual ~ILogger() {};
			//log debug to corrent logger
			virtual auto log_debug(std::string msg, bool is_need_stack_trace = false) -> void = 0;
			//log info to corrent logger
			virtual auto log_info(std::string msg, bool is_need_stack_trace = false) -> void = 0;
			//log debug to corrent logger
			virtual auto log_warning(std::string msg, bool is_need_stack_trace = true) -> void = 0;
			//log error to corrent logger
			virtual auto log_error(std::string msg, bool is_need_stack_trace = true) -> void = 0;
			//log fatal to corrent logger
			virtual auto log_fatal(std::string msg, int error_code, bool is_need_stack_trace = true) -> void = 0;
			//flush corrent logger_section
			virtual auto flush() -> void = 0;

			virtual auto add_strategy(std::shared_ptr<ILogStrategy> strategy) -> void = 0;
		};
	}

	using formatter_rf = std::shared_ptr<scl::interface::ILogFormatter>;
	using formatters_collection = std::vector<formatter_rf>;

	namespace strategy
	{
		class FileLogStrategy : public interface::ILogStrategy
		{
		private:
			std::stringstream _log_buffer;
			std::string _file_path;
			std::ofstream _file_stream;
			const size_t _buffer_size;
			size_t _actual_buffer_filling = 0;
			std::shared_ptr<msg::LogLevelMsgFormat> _log_format;
			scl::formatters_collection _formatters;

		public:
			FileLogStrategy(std::string file_path = "./default.log", std::shared_ptr<msg::LogLevelMsgFormat> log_format = std::shared_ptr<msg::LogLevelMsgFormat>(new msg::LogLevelMsgFormat{}), size_t buffer_size = 1024, scl::formatters_collection addition_formatters = scl::formatters_collection{});
			FileLogStrategy(json& cfg, scl::formatters_collection addition_formatters = scl::formatters_collection{});

			~FileLogStrategy() override {
				_file_stream.flush();
				_file_stream.close();
			}

			auto log(msg::LogMsg log_msg) -> void override;

			auto flush() -> void override;
		};

		class ConsoleLogStrategy : public interface::ILogStrategy
		{
		private:
			std::stringstream _log_buffer;
			const size_t _buffer_size;
			size_t _actual_buffer_filling = 0;
			std::shared_ptr<msg::LogLevelMsgFormat> _log_format;
			scl::formatters_collection _formatters;

		public:
			ConsoleLogStrategy(std::shared_ptr<msg::LogLevelMsgFormat> log_format = std::shared_ptr<msg::LogLevelMsgFormat>(new msg::LogLevelMsgFormat{}), size_t buffer_size = 1024, scl::formatters_collection addition_formatters = scl::formatters_collection{});
			ConsoleLogStrategy(json& cfg, scl::formatters_collection addition_formatters = scl::formatters_collection{});

			~ConsoleLogStrategy() override {}

			auto log(msg::LogMsg log_msg) -> void override;

			auto flush() -> void override;
		};
	}

	namespace interface
	{
		class ILoggerManager
		{
		private:
			ILoggerManager(ILoggerManager&) = delete;
			ILoggerManager& operator=(const ILoggerManager&) = delete;
		public:
			ILoggerManager() {}
			virtual ~ILoggerManager() {}
			virtual auto flush_all() -> void = 0;
			virtual auto create_logger(std::string log_name, std::shared_ptr<ILogStrategy> strategy, bool throw_if_exist) -> void = 0;
			//check existanse log;
			//if need_create_if_not_exist == true create and return false;
			//if need_create_if_not_exist == false and not exist return false;
			//if need_create_if_not_exist == anything and exist return true
			virtual auto is_logger_exist(std::string log_name, bool need_create_if_not_exist, std::shared_ptr<interface::ILogStrategy> strategy) -> bool = 0;
			virtual auto get_logger(std::string log_name)->std::shared_ptr<ILogger> = 0;
		};
	}

	namespace formatter
	{
		class DateFormatter : public interface::ILogFormatter {
		public:
			auto format(std::string& log_text, msg::LogMsg log_msg) -> void override;
			auto get_search_regex() -> std::string override;
		};

		class MessageFormatter : public interface::ILogFormatter {
		public:
			auto format(std::string& log_text, msg::LogMsg log_msg) -> void override;
			auto get_search_regex() -> std::string override;
		};

		class StackTraceFormatter : public interface::ILogFormatter {
		public:
			auto format(std::string& log_text, msg::LogMsg log_msg) -> void override;
			auto get_search_regex() -> std::string override;
		};

		class LogLevelFormatter : public interface::ILogFormatter {
		public:
			auto format(std::string& log_text, msg::LogMsg log_msg) -> void override;
			auto get_search_regex() -> std::string override;
		};

		class NewLineFormatter : public interface::ILogFormatter {
		public:
			auto format(std::string& log_text, msg::LogMsg log_msg) -> void override;
			auto get_search_regex() -> std::string override;
		};

		class ErrorCodeFormatter : public interface::ILogFormatter {
		public:
			auto format(std::string& log_text, msg::LogMsg log_msg) -> void override;
			auto get_search_regex() -> std::string override;
		};
	}

	namespace logger
	{
		class Logger : public interface::ILogger
		{
		private:
			auto _log(msg::LogMsg log_msg) -> void;

			auto _flush() -> void;

			auto _add_strategy(std::shared_ptr<interface::ILogStrategy> strategy) -> void;

		public:
			//logger with logging in log_section
			Logger(std::shared_ptr<context::LoggerContext> context);

			~Logger() override;

			//log debug to corrent logger
			auto log_debug(std::string msg, bool is_need_stack_trace) -> void override;

			//log info to corrent logger
			auto log_info(std::string msg, bool is_need_stack_trace) -> void override;

			//log debug to corrent logger
			auto log_warning(std::string msg, bool is_need_stack_trace) -> void override;

			//log error to corrent logger
			auto log_error(std::string msg, bool is_need_stack_trace) -> void override;

			//log fatal to corrent logger
			auto log_fatal(std::string msg, int error_code, bool is_need_stack_trace) -> void override;

			auto flush() -> void override;

			auto add_strategy(std::shared_ptr<interface::ILogStrategy> strategy) -> void override;
		};
	}

	namespace logger_manager
	{
		class LoggerManager : public interface::ILoggerManager
		{
		private:
			std::map<std::string, std::shared_ptr<context::LoggerContext>> _logger_context_map;
			std::mutex* _modify_context_collection_mx;

		public:
			LoggerManager();

			~LoggerManager() override;

			auto flush_all() -> void override;

			auto create_logger(std::string log_name, std::shared_ptr<interface::ILogStrategy> strategy, bool throw_if_exist) -> void override;

			auto is_logger_exist(std::string log_name, bool need_create_if_not_exist, std::shared_ptr<interface::ILogStrategy> strategy) -> bool override;

			auto get_logger(std::string log_name)->std::shared_ptr<interface::ILogger> override;
		};
	}
}

#endif //simple_cpp_logger