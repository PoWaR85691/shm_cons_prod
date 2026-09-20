add_executable(producer producer_main.cpp)
target_link_libraries(producer PRIVATE fast_channel_core)

add_executable(consumer consumer_main.cpp)
target_link_libraries(consumer PRIVATE fast_channel_core)
