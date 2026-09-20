cmake_minimum_required(VERSION 3.20)
project(fast_channel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Which inter-process synchronization backend the channel is built with:
#   spinlock   one std::atomic<bool> per lock (1 byte), busy wait
#   semaphore  one POSIX sem_t per lock (32 bytes), kernel wait
set(FAST_CHANNEL_SYNC "spinlock" CACHE STRING "Inter-process synchronization backend")
set_property(CACHE FAST_CHANNEL_SYNC PROPERTY STRINGS spinlock semaphore)

add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -O3 -march=native)

enable_testing()

add_subdirectory(core)
add_subdirectory(apps)
add_subdirectory(tests)
