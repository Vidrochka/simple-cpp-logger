# simple-cpp-logger
Небольшой расширяемый c++ логгер    

Для подключения надо добавить файлы из папок include, src в проект    
Так же необходимо добавить папку включения third-party-libs/boost (для трассировки стека) и файл json.hpp (nlohman/json)    

Для начала надо создать менеджер логов    

```cpp

scl::interface::ILoggerManager* manager = new scl::logger_manager::LoggerManager();

```

Менелжер логов позволяет создать контекст логгера    
Параметры:    
1 - Название логгера     
2 - Стратегия логгирования    
3 - Бросать ли ошибку при существовании логгера   

```cpp

manager->create_logger("example", std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::FileLogStrategy("./example.log", std::shared_ptr<scl::msg::LogLevelMsgFormat>(new scl::msg::LogLevelMsgFormat{}), 0)), true);

```

После мы можем получить логгер и связать его с существующим контекстом     
Безопасно использовать логгеры указывающие на 1 контекст из нескольких потоков    
Не безопасно использовать один логгер из нескольких потоков    

```cpp

std::shared_ptr<scl::interface::ILogger> logger = manager->get_logger("example");

```

При необходимости можно добавить стратегию логгировния     

```cpp
//до этого все логи пишутся только в первую стратегию
logger->add_strategy(std::shared_ptr<scl::interface::ILogStrategy>(new scl::strategy::ConsoleLogStrategy(std::shared_ptr<scl::msg::LogLevelMsgFormat>(new scl::msg::LogLevelMsgFormat{}), 0)));
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

Для расширения функционала логгирования необходимо определить свои стратегии логгирования    
Необходимо реализовать интерфейс scl::interface::ILogStrategy    

По дефолту доступно 2 стратеги:    
 * ConsoleLogStrategy (stdout)    
 * FileLogStrategy (файл)    

ConsoleLogStrategy принмает параметры:    
 1. std::shared_ptr<scl::msg::LogLevelMsgFormat>
 2. размер буффера
 3. дополнительные форматтеры

FileLogStrategy принмает параметры:    
 1. путь до файла
 2. std::shared_ptr<scl::msg::LogLevelMsgFormat>
 3. размер буффера
 4. дополнительные форматтеры

Структура scl::msg::MsgFormat содержит строки для обычного лога и с трассировкой стерв    
Структура scl::msg::LogLevelMsgFormat содержит набор элементов scl::msg::MsgFormat для всех видов логов (debug/info/warning/error/fatal)   

При записи логгера в буффер, сообщение обрабатывается коллекцией форматтеров     
Форматтер модифицирует строку формата (см. scl::msg::LogLevelMsgFormat), форматирует строку, добавляя информация сообщения (к примеру {msg} заменяется на сообщение лога)     
В конструкторы страндартных стратегий можно передать дополнительные форматтеры, помимо основных     
Форматтеры реализуют интерфейс scl::interface::ILogFormatter     

Опционально можно подключить конфигурирование из файла конфига. Для этого надо включить файлы из папки strategy-extensions     

Стратегия конфигугируемого логгера создается следующим образом:

```cpp

manager->create_logger("configurable", std::shared_ptr<scl::interface::ILogStrategy>(new scl::extensions::ConfigurableLoggerStrategy("./logger.cfg.json")), true);
		std::shared_ptr<scl::interface::ILogger> configurable_logger = manager->get_logger("configurable");

```

В конструкторе ConfigurableLoggerStrategy принимает путь до файла с конфигом      
Файл должен быть в подобном виде:

```json

{
	"file": [
		{
			"file_path": "ConfigurableLog.log",
			"log_format": 
			{
				"debug":
				{
					"simple":"{ll} | {msg} | d{hh:mm::ss}",
					"simple_with_stacktrace":"{ll} | {msg} | {st} | d{hh:mm::ss}"
				}
			}
		}
	],
	"console": [
		{
			"log_format":
			{
				"debug":
				{
					"simple":"{ll} | {msg} | d{hh:mm::ss}",
					"simple_with_stacktrace":"{set-color:Grey}{ll} | {set-color:Yellow}{msg} | {set-color:LightCyan}{st} | d{hh:mm::ss}"
				}
			}
		}
	]
}

```

Конфиг содержит сколлекции массив стратегий "file" и "console"    

Для конфига стратегии файла допустимы поля:    
 * file_path
 * log_format
 * buffer_size

Для конфига стратегии консоли допустимы поля:    
 * log_format
 * buffer_size

log_format содеожит настройки для scl::msg::LogLevelMsgFormat    
log_format может содержать секцию base. Настройки из base будут применены ко всем форматам для которых явно не указан формат
log_format может не содержать формат типа логов, тогда формат берется предустановленный:    

```json

{
    "debug": {
        "simple":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Grey}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}",
        "simple_with_stacktrace":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Grey}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
    },
    "info": {
        "simple":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Green}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}",
        "simple_with_stacktrace":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Green}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
    },
    "warning": {
        "simple":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Orange}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}",
        "simple_with_stacktrace":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Orange}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
    },
    "error": {
        "simple":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Red}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}",
        "simple_with_stacktrace":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Red}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
    },
    "fatal": {
        "simple":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Red}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Red} -> \"error_code\": {errcode}{nl}",
        "simple_with_stacktrace":"{set-color:Cyan}d{hh:mm:ss dd:MM:yyyy} {set-color:Red}<{ll}> {nl}{set-color:Yellow} -> \"message\": \"{msg}\"{nl}{set-color:Red} -> \"error_code\": {errcode}{nl}{set-color:Orange} -> \"stack_trace\":{nl}{st}{nl}"
    }
}

```

На данный момент доступны следующие форматтеры:
 * d{*тут формат даты-времени*} доступны следующие элементы (yyyy, yy, MM, dd, hh, mm, ss) //TODO: год криво пишется, надо пофиксить
 * {msg}
 * {stacktrace} или {st}
 * {loglevel} или {ll}
 * {nl} или {newline}
 * {errcode} или {ec}
 * {set-color:*цвет*} доступны следующие элементы (Black/Grey/LightGrey/White/Blue/Green/Cyan/Red/Purple/LightBlue/LightGreen/LightCyan/LightRed/LightPurple/Orange/Yellow) //TODO: надо сделать замену на пустую строку для файлов, а так же добавть поддержку UNIX терминалов, пока логика цвета только для windows)