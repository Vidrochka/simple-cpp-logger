# simple-cpp-logger
Небольшой расширяемый c++ логгер

Для подключения надо добавить файл include/SimpleCppLogger.hpp в проект    
Так же необходимо добавить папку включения third-party-libs/boost_1_75_0/boost (для трассировки стека)

Для начала надо создать менеджер логов

```cpp

scl::interface::ILoggerManager* manager = new scl::logger_manager::LoggerManager();

```

Менелжер логов позволяет создать логгер    
Параметры:
1 - Название логгера
2 - Стратегия логгирования (опциональный, по дефолту в файл ./default.log с буффером 1024 символов)
3 - Бросать ли ошибку при существовании логгера (опциональный, по дефолту true)

```cpp

manager->create_logger("example", std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::FileLogStrategy("./example.log", 0)));

```

После мы можем получить существующий логгер

```cpp

std::shared_ptr<scl::interface::ILogger> logger = manager->get_logger("example");

```

При необходимости можно добавить стратегию логгировния

```cpp
//до этого все логи пишутся только в первый логгер
logger->add_strategy(std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::ConsoleLogStrategy(0)));
//теперь логи пишутся в оба лога

```

Для логгирования доступны несколько методов, с возможностью вывода стека вызовов   
Указать необходимость вывода стека вызовов можно при помощи второго опционального параметра

```cpp

logger->log_debug("debug"); //только при сборке в _DEBUG
logger->log_info("info");
logger->log_warning("warning");
logger->log_error("error");
logger->log_fatal("fatal", 0); //требуется номер ошибки (не прекращает работу программы, только для дополнительной идентификации)

```