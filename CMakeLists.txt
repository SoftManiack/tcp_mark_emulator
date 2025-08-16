cmake_minimum_required(VERSION 3.14)
project(tcp_mark_emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG        "v1.15.3"  # Версия
)
FetchContent_MakeAvailable(spdlog)

set(LOG_DIR "/home/artem/development/c++/tcp_mark_emulator/logs")  # или "${CMAKE_SOURCE_DIR}/logs"

# Создаём папку для логов (если её нет)
file(MAKE_DIRECTORY ${LOG_DIR})

# Передаём путь в код как определение препроцессора
add_compile_definitions(LOG_PATH="${LOG_DIR}/")

# Создание исполняемого файла
add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE spdlog::spdlog)


#mkdir build && cd build
#cmake .. 
#make
#./server

