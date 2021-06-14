# CMakeList.txt: проект CMake для simple-cpp-logger-example; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.8)

set(LOG_SRC
    "../../../src/SimpleCppLogger.cpp"
    "../../../strategy-extensions/ConfigurableLoggerStrategy.cpp"
)

include_directories("../../../include/")
include_directories("../../../strategy-extensions/ConfigurableLoggerStrategy.hpp")

include_directories("../../../third-party-libs/")

# Добавьте источник в исполняемый файл этого проекта.
add_executable (simple-cpp-logger-example "simple-cpp-logger-example.cpp" "simple-cpp-logger-example.h" ${LOG_SRC})

# TODO: Добавьте тесты и целевые объекты, если это необходимо.

set_target_properties (simple-cpp-logger-example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)