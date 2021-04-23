#ifndef simple_cpp_logger
#define simple_cpp_logger

#include "../third-party-libs/boost_1_75_0/boost/stacktrace.hpp"

#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <mutex>
#include <map>
#include <exception>
#include <iostream>

#undef interface

#define TO_STRING(obj) std::string(#obj)

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
    }

    namespace utils
    {
        auto _get_datetime_prefix() -> std::string {
            time_t now = time(0);
#pragma warning(suppress : 4996)
            std::string raw_dt{ctime(&now)};
            std::string s_dt = std::string(raw_dt.begin(), raw_dt.end());
            return "[" + s_dt.substr(0, s_dt.length() - 1) + "]";
        }
    }

    namespace msg
    {
        enum EVENT_TYPE : std::uint_fast8_t
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
    }

    namespace interface
    {
        class ILogStrategy
        {
        private:
            ILogStrategy(ILogStrategy &) = delete;
            ILogStrategy &operator=(const ILogStrategy &) = delete;
        public:
            ILogStrategy(){};
            virtual ~ILogStrategy() {}
            virtual auto log(msg::LogMsg log_msg) -> void = 0;
            virtual auto flush() -> void = 0;
        };
    }

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

        public:
            FileLogStrategy(std::string file_path = "./default.log", size_t buffer_size = 1024) : _buffer_size(buffer_size) {
                _file_path = file_path;
                _file_stream = std::ofstream{file_path, std::ios::app};

                if (!_file_stream.is_open())
                    throw exceptions::log_file_unawalable_exception("Can't open required file", _file_path);
            }

            ~FileLogStrategy() override {
                _file_stream.flush();
                _file_stream.close();
            }

            auto log(msg::LogMsg log_msg) -> void override {
                std::string pre_log_buffer;
                pre_log_buffer += utils::_get_datetime_prefix() + " ";

                switch (log_msg.event_type)
                {
                case msg::EVENT_TYPE::LOG_DEBUG:
                    pre_log_buffer += TO_STRING(<EVENT_TYPE::LOG_DEBUG>);
                    break;
                case msg::EVENT_TYPE::LOG_INFO:
                    pre_log_buffer += TO_STRING(<EVENT_TYPE::LOG_INFO>);
                    break;
                case msg::EVENT_TYPE::LOG_WARNING:
                    pre_log_buffer += TO_STRING(<EVENT_TYPE::LOG_WARNING>);
                    break;
                case msg::EVENT_TYPE::LOG_ERROR:
                    pre_log_buffer += TO_STRING(<EVENT_TYPE::LOG_ERROR>);
                    break;
                case msg::EVENT_TYPE::LOG_FATAL:
                    pre_log_buffer += TO_STRING(<EVENT_TYPE::LOG_FATAL>);
                    break;
                }

                pre_log_buffer += "\n -> \"message\": \"" + log_msg.msg + "\"";

                if (log_msg.has_error_code)
                    pre_log_buffer += "\n -> \"error_code\": " + std::to_string(log_msg.error_code);

                if (log_msg.has_stack_trace)
                    pre_log_buffer += "\n -> \"stack_trace\":\n" + boost::stacktrace::to_string(log_msg.stack_trace) + "\n\n";
                else
                    pre_log_buffer += "\n\n";

                _log_buffer << pre_log_buffer;

                _actual_buffer_filling += pre_log_buffer.size();

                if (_actual_buffer_filling >= _buffer_size)
                {
                    _actual_buffer_filling = 0;
                    flush();
                }
            }
            auto flush() -> void override {
                _file_stream << _log_buffer.str();
                _file_stream.flush();
                _log_buffer.str(std::string());
            }
        };

#define RED_CONSOLE_COLOR(text) std::string("\x1b[31m") + text + "\x1b[0m"
#define GREEN_CONSOLE_COLOR(text) std::string("\x1b[32m") + text + "\x1b[0m"
#define ORANGE_CONSOLE_COLOR(text) std::string("\x1b[33m") + text + "\x1b[0m"
#define YELOW_CONSOLE_COLOR(text) std::string("\x1b[33;1m") + text + "\x1b[0m"
#define WHITE_CONSOLE_COLOR(text) std::string("\x1b[37m") + text + "\x1b[0m"
#define MAGENTA_CONSOLE_COLOR(text) std::string("\x1b[35m") + text + "\x1b[0m"
#define BLUE_CONSOLE_COLOR(text) std::string("\x1b[34m") + text + "\x1b[0m"
#define CYAN_CONSOLE_COLOR(text) std::string("\x1b[36m") + text + "\x1b[0m"
#define GRAY_CONSOLE_COLOR(text) std::string("\x1b[30;1m") + text + "\x1b[0m"

        class ConsoleLogStrategy : public interface::ILogStrategy
        {
        private:
            std::stringstream _log_buffer;
            const size_t _buffer_size;
            size_t _actual_buffer_filling = 0;

        public:
            ConsoleLogStrategy(size_t buffer_size = 1024) : _buffer_size(buffer_size) { }

            ~ConsoleLogStrategy() override {}

            auto log(msg::LogMsg log_msg) -> void override {
                std::string pre_log_buffer;
                pre_log_buffer += GRAY_CONSOLE_COLOR(utils::_get_datetime_prefix()) + " ";

                switch (log_msg.event_type)
                {
                case msg::EVENT_TYPE::LOG_DEBUG:
                    pre_log_buffer += WHITE_CONSOLE_COLOR(TO_STRING(<EVENT_TYPE::LOG_DEBUG>));
                    break;
                case msg::EVENT_TYPE::LOG_INFO:
                    pre_log_buffer += GREEN_CONSOLE_COLOR(TO_STRING(<EVENT_TYPE::LOG_INFO>));
                    break;
                case msg::EVENT_TYPE::LOG_WARNING:
                    pre_log_buffer += YELOW_CONSOLE_COLOR(TO_STRING(<EVENT_TYPE::LOG_WARNING>));
                    break;
                case msg::EVENT_TYPE::LOG_ERROR:
                    pre_log_buffer += RED_CONSOLE_COLOR(TO_STRING(<EVENT_TYPE::LOG_ERROR>));
                    break;
                case msg::EVENT_TYPE::LOG_FATAL:
                    pre_log_buffer += RED_CONSOLE_COLOR(TO_STRING(<EVENT_TYPE::LOG_FATAL>));
                    break;
                }

                pre_log_buffer += "\n -> " + YELOW_CONSOLE_COLOR("\"message\"") + ": \"" + log_msg.msg + "\"";

                if (log_msg.has_error_code)
                    pre_log_buffer += "\n -> " + YELOW_CONSOLE_COLOR("\"error_code\"") + ": " + std::to_string(log_msg.error_code);

                if (log_msg.has_stack_trace)
                    pre_log_buffer += "\n -> " + YELOW_CONSOLE_COLOR("\"stack_trace\"") + ":\n" + ORANGE_CONSOLE_COLOR(boost::stacktrace::to_string(log_msg.stack_trace)) + "\n\n";
                else
                    pre_log_buffer += "\n\n";

                _log_buffer << pre_log_buffer;

                _actual_buffer_filling += pre_log_buffer.size();

                if (_actual_buffer_filling >= _buffer_size) {
                    _actual_buffer_filling = 0;
                    flush();
                }
            }

            auto flush() -> void override {
                std::cout << _log_buffer.str() << std::flush;
                _log_buffer.str(std::string());
            }
        };
    }

    namespace context
    {
        struct LoggerContext
        {
        public:
            LoggerContext() {}
            LoggerContext(std::shared_ptr<interface::ILogStrategy> strategy) : strategy_list(std::vector{strategy}) {
            }
            ~LoggerContext() {}
            std::vector<std::shared_ptr<interface::ILogStrategy>> strategy_list;
            std::mutex write_mutex{};
        };
    }

    class strategy::FileLogStrategy;

    namespace interface
    {
        class ILogger
        {
        protected:
            std::shared_ptr<context::LoggerContext> _logger_context;

        private:
            ILogger() = delete;
            ILogger(ILogger &) = delete;
            ILogger &operator=(const ILogger &) = delete;

        public:
            ILogger(std::shared_ptr<context::LoggerContext> context) {
                _logger_context = context;
            }
            virtual ~ILogger(){};
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

        class ILoggerManager
        {
        private:
            ILoggerManager(ILoggerManager &) = delete;
            ILoggerManager &operator=(const ILoggerManager &) = delete;
        public:
            ILoggerManager() {}
            virtual ~ILoggerManager() {}
            virtual auto flush_all() -> void = 0;
            virtual auto create_logger(std::string log_name, std::shared_ptr<ILogStrategy> strategy = std::shared_ptr<interface::ILogStrategy>(new strategy::FileLogStrategy{}), bool throw_if_exist = true) -> void = 0;
            //check existanse log;
            //if need_create_if_not_exist == true create and return false;
            //if need_create_if_not_exist == false and not exist return false;
            //if need_create_if_not_exist == anything and exist return true
            virtual auto is_logger_exist(std::string log_name, bool need_create_if_not_exist = false, std::shared_ptr<interface::ILogStrategy> strategy = std::shared_ptr<interface::ILogStrategy>(new strategy::FileLogStrategy{})) -> bool = 0;
            virtual auto get_logger(std::string log_name) -> std::shared_ptr<ILogger> = 0;
        };
    }

    namespace logger
    {
        class Logger : public interface::ILogger
        {
        private:
            auto _log(msg::LogMsg log_msg) -> void{
                std::lock_guard<std::mutex> lg(_logger_context->write_mutex);

                for (auto strategy : _logger_context->strategy_list)
                    strategy->log(log_msg);
            }
            
            auto _flush() -> void {
                std::lock_guard<std::mutex> lg(_logger_context->write_mutex);

                for (auto strategy : _logger_context->strategy_list)
                    strategy->flush();
            }

            auto _add_strategy(std::shared_ptr<interface::ILogStrategy> strategy) -> void {
                std::lock_guard<std::mutex> lg(_logger_context->write_mutex);
                _logger_context->strategy_list.push_back(strategy);
            }

        public:
            //logger with logging in log_section
            Logger(std::shared_ptr<context::LoggerContext> context) : interface::ILogger(context) {
            }

            ~Logger() override { flush(); }

            //log debug to corrent logger
            auto log_debug(std::string msg, bool is_need_stack_trace = false) -> void override {
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
            auto log_info(std::string msg, bool is_need_stack_trace = false) -> void override {
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
            auto log_warning(std::string msg, bool is_need_stack_trace = true) -> void override {
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
            auto log_error(std::string msg, bool is_need_stack_trace = true) -> void override {
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
            auto log_fatal(std::string msg, int error_code, bool is_need_stack_trace = true) -> void override {
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

            auto flush() -> void override {
                _flush();
            }

            auto add_strategy(std::shared_ptr<interface::ILogStrategy> strategy = std::shared_ptr<interface::ILogStrategy>(new strategy::FileLogStrategy{})) -> void override {
                _add_strategy(strategy);
            }
        };
    }

    namespace logger_manager
    {
        class LoggerManager : public interface::ILoggerManager
        {
        private:
            std::map<std::string, std::shared_ptr<context::LoggerContext>> _logger_context_map;
            std::mutex *_modify_context_collection_mx;

        public:
            LoggerManager() {
                _modify_context_collection_mx = new std::mutex();
            }

            ~LoggerManager() override {
                flush_all();

                delete _modify_context_collection_mx;
            }

            auto flush_all() -> void override {
                for (auto logger_context : _logger_context_map)
                {
                    auto log_info = logger_context.second;

                    std::lock_guard<std::mutex> lg(log_info->write_mutex);

                    for (auto strategy : log_info->strategy_list)
                        strategy->flush();
                }
            }

            auto create_logger(std::string log_name, std::shared_ptr<interface::ILogStrategy> strategy = std::shared_ptr<interface::ILogStrategy>(new strategy::FileLogStrategy{}), bool throw_if_exist = true) -> void override {
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

            auto is_logger_exist(std::string log_name, bool need_create_if_not_exist = false, std::shared_ptr<interface::ILogStrategy> strategy = std::shared_ptr<interface::ILogStrategy>(new strategy::FileLogStrategy{})) -> bool override {
                if (_logger_context_map.count(log_name) > 0)
                    return true;

                std::lock_guard<std::mutex> lg(*_modify_context_collection_mx);

                if (need_create_if_not_exist)
                    _logger_context_map[log_name] = std::shared_ptr<context::LoggerContext>(new context::LoggerContext(strategy));

                return false;
            }

            auto get_logger(std::string log_name) -> std::shared_ptr<interface::ILogger> override {
                auto context = _logger_context_map[log_name];

                std::lock_guard<std::mutex> lg(context->write_mutex);

                interface::ILogger *logger = new logger::Logger(std::shared_ptr<context::LoggerContext>(context));
                return std::shared_ptr<interface::ILogger>(logger);
            }
        };
    }
}

#endif //simple_cpp_logger