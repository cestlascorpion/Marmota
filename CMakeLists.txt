cmake_minimum_required(VERSION 3.28)
project(Marmota)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Wsizeof-pointer-memaccess \
                     -Wfloat-equal -Wconversion-null -Woverflow -Wshadow \
                     -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -pthread -g -O0")

include_directories(src /usr/local/include)

link_directories(/usr/local/lib)

add_executable(Marmota main.cpp
        src/queue.h
        src/message.h
        src/message.cpp
        src/interceptor.h
        src/interceptor.cpp
        src/exporter.h
        src/exporter.cpp
        src/alarm.h
        src/alarm.cpp)

target_link_libraries(Marmota zlog)