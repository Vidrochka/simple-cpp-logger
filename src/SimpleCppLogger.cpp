#include "../include/SimpleCppLogger.hpp"
#include <typeinfo>

using string = std::string;

#if defined(WIN32)
#include "windows.h"
#endif // WIN32

namespace scl
{
	using formatters_map = std::map<std::string, formatter_rf>;

	namespace utils
	{
		class FormattersCachedCollection {
		private:
			static scl::formatters_map _formatters_cach;
		public:
			template<typename T>
			static auto get_formatter(string name) -> scl::formatter_rf {
				if (_formatters_cach.count(name) == 0)
					_formatters_cach[name] = scl::formatter_rf(new T{});

				return _formatters_cach[name];
			}
		};

		scl::formatters_map FormattersCachedCollection::_formatters_cach = scl::formatters_map();

		class FormattersCollectionBuilder {
		private:
			scl::formatters_collection _formatters;
			std::shared_ptr<msg::LogLevelMsgFormat> _log_msg_format;

			auto format_regex_search(msg::MsgFormat& log_msg_format, const string& regex) {
				if (
					std::regex_search(log_msg_format.simple, std::regex{ regex }) ||
					std::regex_search(log_msg_format.simple_with_stacktrace, std::regex{ regex })
					) return true;

				return false;
			}

			auto log_level_format_regex_search(std::shared_ptr<msg::LogLevelMsgFormat>& log_msg_format, const string regex) {
				if (
					format_regex_search(log_msg_format->debug, regex) ||
					format_regex_search(log_msg_format->info, regex) ||
					format_regex_search(log_msg_format->warning, regex) ||
					format_regex_search(log_msg_format->error, regex) ||
					format_regex_search(log_msg_format->fatal, regex)
					) return true;

				return false;
			}

		public: 
			FormattersCollectionBuilder(std::shared_ptr<msg::LogLevelMsgFormat> log_msg_format, scl::formatters_collection addition_formatters = scl::formatters_collection{})
				: _log_msg_format(log_msg_format), _formatters(addition_formatters)  { }

			template<class T>
			auto set_avalable() -> FormattersCollectionBuilder& {
				T formatter{};
				if (log_level_format_regex_search(_log_msg_format, formatter.get_search_regex()))
					_formatters.push_back(FormattersCachedCollection::get_formatter<T>(typeid(T).name()));
				return *this;
			}

			auto build() -> scl::formatters_collection {
				return _formatters;
			}
		};
	}

	namespace msg
	{
		MsgFormat::MsgFormat(json& msg_format_cfg) : MsgFormat() {
			if (msg_format_cfg.contains("simple"))
				simple = msg_format_cfg["simple"].get<string>();

			if (msg_format_cfg.contains("simple_with_stacktrace"))
				simple_with_stacktrace = msg_format_cfg["simple_with_stacktrace"].get<string>();
		}

		MsgFormat::MsgFormat(
			std::string simple,
			std::string simple_with_stacktrace
		) : 
			simple(simple), 
			simple_with_stacktrace(simple_with_stacktrace)
		{}

		LogLevelMsgFormat::LogLevelMsgFormat(json& log_level_msg_format_cfg) : LogLevelMsgFormat() {
			if (log_level_msg_format_cfg.contains("base"))
			{
				MsgFormat format = MsgFormat{log_level_msg_format_cfg["base"]};
				debug = format;
				info = format;
				warning = format;
				error = format;
				fatal = format;
			}

			if (log_level_msg_format_cfg.contains("debug"))
				debug = MsgFormat{ log_level_msg_format_cfg["debug"] };

			if (log_level_msg_format_cfg.contains("info"))
				info = MsgFormat{ log_level_msg_format_cfg["info"] };

			if (log_level_msg_format_cfg.contains("warning"))
				warning = MsgFormat{ log_level_msg_format_cfg["warning"] };

			if (log_level_msg_format_cfg.contains("error"))
				error = MsgFormat{ log_level_msg_format_cfg["error"] };

			if (log_level_msg_format_cfg.contains("fatal"))
				fatal = MsgFormat{ log_level_msg_format_cfg["fatal"] };
		}

		LogLevelMsgFormat::LogLevelMsgFormat(
			MsgFormat debug_format, 
			MsgFormat info_format,
			MsgFormat warning_format,
			MsgFormat error_format,
			MsgFormat fatal_format) : 
			debug(debug_format), info(info_format), warning(warning_format), error(error_format), fatal(fatal_format)
		{
		}

		auto LogLevelMsgFormat::get_format(LogMsg& log_msg) -> std::string {
			MsgFormat* msg_format;

			switch (log_msg.event_type)
			{
			case msg::EVENT_TYPE::LOG_DEBUG:
				msg_format = &debug;
				break;
			case msg::EVENT_TYPE::LOG_INFO:
				msg_format = &info;
				break;
			case msg::EVENT_TYPE::LOG_WARNING:
				msg_format = &warning;
				break;
			case msg::EVENT_TYPE::LOG_ERROR:
				msg_format = &error;
				break;
			case msg::EVENT_TYPE::LOG_FATAL:
				msg_format = &fatal;
				break;
			}

			if (log_msg.has_stack_trace)
				return msg_format->simple_with_stacktrace;
			else
				return msg_format->simple;
		}
	}

	namespace strategy
	{

		FileLogStrategy::FileLogStrategy(string file_path, std::shared_ptr<msg::LogLevelMsgFormat> log_format, size_t buffer_size, scl::formatters_collection addition_formatters) : _buffer_size(buffer_size) {
			_file_path = file_path;
			_file_stream = std::ofstream{ file_path, std::ios::app };
			_log_format = log_format;

			utils::FormattersCollectionBuilder formatters_builder{log_format, addition_formatters};
			formatters_builder.set_avalable<formatter::DateFormatter>();
			formatters_builder.set_avalable<formatter::MessageFormatter>();
			formatters_builder.set_avalable<formatter::StackTraceFormatter>();
			formatters_builder.set_avalable<formatter::LogLevelFormatter>();
			formatters_builder.set_avalable<formatter::NewLineFormatter>();
			formatters_builder.set_avalable<formatter::ErrorCodeFormatter>();
			_formatters = formatters_builder.build();

			if (!_file_stream.is_open())
				throw exceptions::log_file_unawalable_exception("Can't open required file", _file_path);
		}

		FileLogStrategy::FileLogStrategy(json& cfg, scl::formatters_collection addition_formatters)
			: FileLogStrategy(
				cfg.contains("file_path") ? cfg["file_path"].get<string>() : "./default.log",
				cfg.contains("log_format") ? std::shared_ptr<msg::LogLevelMsgFormat>(new msg::LogLevelMsgFormat{ cfg["log_format"] }) : std::shared_ptr<msg::LogLevelMsgFormat>(new msg::LogLevelMsgFormat{}),
				cfg.contains("buffer_size") ? cfg["buffer_size"].get<size_t>() : 1024,
				addition_formatters
			)
		{ }

		auto FileLogStrategy::log(msg::LogMsg log_msg) -> void {
			std::string log_format = _log_format->get_format(log_msg);

			for (auto& formatter : _formatters)
				formatter->format(log_format, log_msg);

			_log_buffer << log_format;

			_actual_buffer_filling += log_format.size();

			if (_actual_buffer_filling >= _buffer_size) {
				_actual_buffer_filling = 0;
				flush();
			}
		}
		auto FileLogStrategy::flush() -> void {
			_file_stream << _log_buffer.str();
			_file_stream.flush();
			_log_buffer.str(string());
		}

		ConsoleLogStrategy::ConsoleLogStrategy(std::shared_ptr<msg::LogLevelMsgFormat> log_format, size_t buffer_size, scl::formatters_collection addition_formatters) : _buffer_size(buffer_size) {
			_log_format = log_format;
			
			utils::FormattersCollectionBuilder formatters_builder{log_format, addition_formatters};
			formatters_builder.set_avalable<formatter::DateFormatter>();
			formatters_builder.set_avalable<formatter::MessageFormatter>();
			formatters_builder.set_avalable<formatter::StackTraceFormatter>();
			formatters_builder.set_avalable<formatter::LogLevelFormatter>();
			formatters_builder.set_avalable<formatter::NewLineFormatter>();
			formatters_builder.set_avalable<formatter::ErrorCodeFormatter>();
			_formatters = formatters_builder.build();
		}

		ConsoleLogStrategy::ConsoleLogStrategy(json& cfg, scl::formatters_collection addition_formatters) 
			: ConsoleLogStrategy
			(
				cfg.contains("log_format") ? std::shared_ptr<msg::LogLevelMsgFormat>(new msg::LogLevelMsgFormat{ cfg["log_format"] }) : std::shared_ptr<msg::LogLevelMsgFormat>(new msg::LogLevelMsgFormat{}),
				cfg.contains("buffer_size") ? cfg["buffer_size"].get<size_t>() : 1024,
				addition_formatters
			)
		{ }

		auto ConsoleLogStrategy::log(msg::LogMsg log_msg) -> void {
			std::string log_format = _log_format->get_format(log_msg);

			for (auto& formatter : _formatters)
				formatter->format(log_format, log_msg);

			_log_buffer << log_format;

			_actual_buffer_filling += log_format.size();

			if (_actual_buffer_filling >= _buffer_size) {
				_actual_buffer_filling = 0;
				flush();
			}
		}

#if defined(WIN32)
		enum windows_colors
        {
            Black       = 0,
            Grey        = FOREGROUND_INTENSITY,
            LightGrey   = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE,
            White       = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
            Blue        = FOREGROUND_BLUE,
            Green       = FOREGROUND_GREEN,
            Cyan        = FOREGROUND_GREEN | FOREGROUND_BLUE,
            Red         = FOREGROUND_RED,
            Purple      = FOREGROUND_RED   | FOREGROUND_BLUE,
            LightBlue   = FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
            LightGreen  = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            LightCyan   = FOREGROUND_GREEN | FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
            LightRed    = FOREGROUND_RED   | FOREGROUND_INTENSITY,
            LightPurple = FOREGROUND_RED   | FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
            Orange      = FOREGROUND_RED   | FOREGROUND_GREEN,
            Yellow      = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        };
#endif // WIN32

		auto ConsoleLogStrategy::flush() -> void {
			string log = _log_buffer.str();
			_log_buffer.str(string());

			std::regex r{ string("\\{set-color:(\\w+)\\}")};

			const std::vector<std::smatch> matches{
				std::sregex_iterator{cbegin(log), cend(log), r},
				std::sregex_iterator{}
			};

			if(matches.empty())
			{
				std::cout << log << std::flush;
				return;
			}
			
			std::vector<std::string> log_parts = std::vector<string>{
        		std::sregex_token_iterator(cbegin(log), cend(log), r, -1),
        		std::sregex_token_iterator()
			};

			std::cout << log_parts[0] << std::flush;		

#if defined(WIN32)

			HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

			for(int i = 0; i < matches.size(); i++){

#define CheckAndSetColor(color, color_name) if(color == std::string(#color_name)) SetConsoleTextAttribute(console, windows_colors::color_name)

				string color = matches[i][1].str();

				CheckAndSetColor(color, Black);
				CheckAndSetColor(color, Grey);
				CheckAndSetColor(color, LightGrey);
				CheckAndSetColor(color, Green);
				CheckAndSetColor(color, Cyan);
				CheckAndSetColor(color, Red);
				CheckAndSetColor(color, Purple);
				CheckAndSetColor(color, LightBlue);
				CheckAndSetColor(color, LightGreen);
				CheckAndSetColor(color, LightCyan);
				CheckAndSetColor(color, LightRed);
				CheckAndSetColor(color, LightPurple);
				CheckAndSetColor(color, Orange);
				CheckAndSetColor(color, Yellow);

				std::cout << log_parts[i+1] << std::flush;
			}

			SetConsoleTextAttribute(console, windows_colors::White);

#endif // WIN32		

		}
	}

	namespace formatter
	{
		auto DateFormatter::format(std::string& log_text, msg::LogMsg log_msg) -> void {
			std::smatch matches;

			if (!std::regex_search(log_text, matches, std::regex{ get_search_regex() }))
				return;

			for (auto& match : matches)
			{
				std::string format = match.str();
				std::string changed_format_str = format.substr(2, format.length() - 3);

				time_t now = time(0);
				tm* ltm = localtime(&now);

				changed_format_str = std::regex_replace(changed_format_str, std::regex("yyyy"), std::to_string(ltm->tm_year));
				changed_format_str = std::regex_replace(changed_format_str, std::regex("yy"), std::to_string(ltm->tm_year).substr(2, 2));
				changed_format_str = std::regex_replace(changed_format_str, std::regex("MM"), std::to_string(ltm->tm_mon));
				changed_format_str = std::regex_replace(changed_format_str, std::regex("dd"), std::to_string(ltm->tm_mday));
				changed_format_str = std::regex_replace(changed_format_str, std::regex("hh"), std::to_string(ltm->tm_hour));
				changed_format_str = std::regex_replace(changed_format_str, std::regex("mm"), std::to_string(ltm->tm_min));
				changed_format_str = std::regex_replace(changed_format_str, std::regex("ss"), std::to_string(ltm->tm_sec));

				format = std::regex_replace(format, std::regex("\\{"), "\\{");
				format = std::regex_replace(format, std::regex("\\}"), "\\}");
				log_text = std::regex_replace(log_text, std::regex(format), changed_format_str);
			}
		}

		auto DateFormatter::get_search_regex() -> std::string {
			return "d\\{[^\\{\\}]+\\}";
		}

		auto MessageFormatter::format(std::string& log_text, msg::LogMsg log_msg) -> void {
			std::smatch matches;

			std::regex r{ get_search_regex() };

			if (std::regex_search(log_text, matches, r)) {
				log_text = std::regex_replace(log_text, r, log_msg.msg);
			}
		}

		auto MessageFormatter::get_search_regex() -> std::string {
			return "\\{msg\\}";
		}

		auto StackTraceFormatter::format(std::string& log_text, msg::LogMsg log_msg) -> void {
			std::smatch matches;

			std::regex r{ get_search_regex() };

			if (!std::regex_search(log_text, matches, r))
				return;

			if (log_msg.has_stack_trace)
				log_text = std::regex_replace(log_text, r, boost::stacktrace::to_string(log_msg.stack_trace));
			else
				log_text = std::regex_replace(log_text, r, "");
		}

		auto StackTraceFormatter::get_search_regex() -> std::string {
			return "\\{stacktrace\\}|\\{st\\}";
		}

		auto LogLevelFormatter::format(std::string& log_text, msg::LogMsg log_msg) -> void {
			std::smatch matches;

			std::regex r{ get_search_regex() };

			if (!std::regex_search(log_text, matches, r))
				return;

			std::string log_level;
			switch (log_msg.event_type)
			{
			case msg::EVENT_TYPE::LOG_DEBUG:
				log_level = TO_STRING(EVENT_TYPE::LOG_DEBUG);
				break;
			case msg::EVENT_TYPE::LOG_INFO:
				log_level = TO_STRING(EVENT_TYPE::LOG_INFO);
				break;
			case msg::EVENT_TYPE::LOG_WARNING:
				log_level = TO_STRING(EVENT_TYPE::LOG_WARNING);
				break;
			case msg::EVENT_TYPE::LOG_ERROR:
				log_level = TO_STRING(EVENT_TYPE::LOG_ERROR);
				break;
			case msg::EVENT_TYPE::LOG_FATAL:
				log_level = TO_STRING(EVENT_TYPE::LOG_FATAL);
				break;
			}
			log_text = std::regex_replace(log_text, r, log_level);
		}

		auto LogLevelFormatter::get_search_regex() -> std::string {
			return "\\{loglevel\\}|\\{ll\\}";
		}

		auto NewLineFormatter::format(std::string& log_text, msg::LogMsg log_msg) -> void {
			std::smatch matches;

			std::regex r{ get_search_regex() };

			if (std::regex_search(log_text, matches, r)) {
				log_text = std::regex_replace(log_text, r, "\n");
			}
		}

		auto NewLineFormatter::get_search_regex() -> std::string {
			return "\\{nl\\}|\\{newline\\}";
		}

		auto ErrorCodeFormatter::format(std::string& log_text, msg::LogMsg log_msg) -> void {
			std::smatch matches;

			std::regex r{ get_search_regex() };

			if (std::regex_search(log_text, matches, r)) {
				if(log_msg.has_error_code)
					log_text = std::regex_replace(log_text, r, std::to_string(log_msg.error_code));
				else
					log_text = std::regex_replace(log_text, r,"");
			}
		}

		auto ErrorCodeFormatter::get_search_regex() -> std::string {
			return "\\{errcode\\}|\\{ec\\}";
		}
	}

	namespace logger
	{
		auto Logger::_log(msg::LogMsg log_msg) -> void {
			std::lock_guard<std::mutex> lg(_logger_context->write_mutex);

			for (auto strategy : _logger_context->strategy_list)
				strategy->log(log_msg);
		}

		auto Logger::_flush() -> void {
			std::lock_guard<std::mutex> lg(_logger_context->write_mutex);

			for (auto strategy : _logger_context->strategy_list)
				strategy->flush();
		}

		auto Logger::_add_strategy(std::shared_ptr<interface::ILogStrategy> strategy) -> void {
			std::lock_guard<std::mutex> lg(_logger_context->write_mutex);
			_logger_context->strategy_list.push_back(strategy);
		}

		//logger with logging in log_section
		Logger::Logger(std::shared_ptr<context::LoggerContext> context) : interface::ILogger(context) {
		}

		Logger::~Logger() { flush(); }


		//log debug to corrent logger
		auto Logger::log_debug(string msg, bool is_need_stack_trace = false) -> void {
#if defined(_DEBUG) || defined(DEBUG)
			msg::LogMsg log_msg{
				msg,
				msg::EVENT_TYPE::LOG_DEBUG,
				false,
				0,
				is_need_stack_trace
			};

			if (is_need_stack_trace)
				log_msg.stack_trace = boost::stacktrace::stacktrace();

			_log(log_msg);
#endif // _DEBUG 
		}

		//log info to corrent logger
		auto Logger::log_info(string msg, bool is_need_stack_trace = false) -> void {
			msg::LogMsg log_msg{
				msg,
				msg::EVENT_TYPE::LOG_INFO,
				false,
				0,
				is_need_stack_trace
			};

			if (is_need_stack_trace)
				log_msg.stack_trace = boost::stacktrace::stacktrace();

			_log(log_msg);
		}

		//log debug to corrent logger
		auto Logger::log_warning(string msg, bool is_need_stack_trace = true) -> void {
			msg::LogMsg log_msg{
				msg,
				msg::EVENT_TYPE::LOG_WARNING,
				false,
				0,
				is_need_stack_trace
			};

			if (is_need_stack_trace)
				log_msg.stack_trace = boost::stacktrace::stacktrace();

			_log(log_msg);
		}

		//log error to corrent logger
		auto Logger::log_error(string msg, bool is_need_stack_trace = true) -> void {
			msg::LogMsg log_msg{
				msg,
				msg::EVENT_TYPE::LOG_ERROR,
				false,
				0,
				is_need_stack_trace
			};

			if (is_need_stack_trace)
				log_msg.stack_trace = boost::stacktrace::stacktrace();

			_log(log_msg);
		}

		//log fatal to corrent logger
		auto Logger::log_fatal(string msg, int error_code, bool is_need_stack_trace = true) -> void {
			msg::LogMsg log_msg{
				msg,
				msg::EVENT_TYPE::LOG_FATAL,
				true,
				error_code,
				is_need_stack_trace
			};

			if (is_need_stack_trace)
				log_msg.stack_trace = boost::stacktrace::stacktrace();

			_log(log_msg);
			_flush();
		}

		auto Logger::flush() -> void {
			_flush();
		}

		auto Logger::add_strategy(std::shared_ptr<interface::ILogStrategy> strategy = std::shared_ptr<interface::ILogStrategy>(new strategy::FileLogStrategy{})) -> void {
			_add_strategy(strategy);
		}
	}

	namespace logger_manager
	{
		LoggerManager::LoggerManager() {
			_modify_context_collection_mx = new std::mutex();
		}

		LoggerManager::~LoggerManager() {
			flush_all();

			delete _modify_context_collection_mx;
		}

		auto LoggerManager::flush_all() -> void {
			for (auto logger_context : _logger_context_map)
			{
				auto log_info = logger_context.second;

				std::lock_guard<std::mutex> lg(log_info->write_mutex);

				for (auto strategy : log_info->strategy_list)
					strategy->flush();
			}
		}

		auto LoggerManager::create_logger(string log_name, std::shared_ptr<interface::ILogStrategy> strategy = std::shared_ptr<interface::ILogStrategy>(new strategy::FileLogStrategy{}), bool throw_if_exist = true) -> void {
			if (_logger_context_map.count(log_name) > 0)
			{
				if (throw_if_exist)
					throw exceptions::logger_context_already_exist_exception("Log section already exist");
				else
					return;
			}

			std::lock_guard<std::mutex> lg(*_modify_context_collection_mx);

			_logger_context_map[log_name] = std::shared_ptr<context::LoggerContext>(new context::LoggerContext(strategy));
		}

		auto LoggerManager::is_logger_exist(std::string log_name, bool need_create_if_not_exist = false, std::shared_ptr<interface::ILogStrategy> strategy = std::shared_ptr<interface::ILogStrategy>(new strategy::FileLogStrategy{})) -> bool {
			if (_logger_context_map.count(log_name) > 0)
				return true;

			std::lock_guard<std::mutex> lg(*_modify_context_collection_mx);

			if (need_create_if_not_exist)
				_logger_context_map[log_name] = std::shared_ptr<context::LoggerContext>(new context::LoggerContext(strategy));

			return false;
		}

		auto LoggerManager::get_logger(std::string log_name) -> std::shared_ptr<interface::ILogger> {
			auto context = _logger_context_map[log_name];

			std::lock_guard<std::mutex> lg(context->write_mutex);

			interface::ILogger* logger = new logger::Logger(std::shared_ptr<context::LoggerContext>(context));
			return std::shared_ptr<interface::ILogger>(logger);
		}
	}
}