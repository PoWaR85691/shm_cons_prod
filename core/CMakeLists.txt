find_package(Threads REQUIRED)

find_library(ZFP_LIBRARY NAMES zfp REQUIRED)
find_path(ZFP_INCLUDE_DIR NAMES zfp.h REQUIRED)

add_library(fast_channel_core
    src/channel/block_protocol.cpp
    src/compression/codec_factory.cpp
    src/compression/none_codec.cpp
    src/compression/options.cpp
    src/compression/zfp_codec.cpp
    src/compression/zstd_codec.cpp
    src/io/float_file_reader.cpp
    src/io/float_file_writer.cpp
    src/ipc/ipc_error.cpp
    src/ipc/shared_memory_layout.cpp
    src/ipc/spin_lock.cpp
    src/ipc/shared_semaphore.cpp
    src/ipc/shared_memory_segment.cpp
    src/ipc/shared_memory_channel.cpp
    src/pipeline/producer_pipeline.cpp
    src/pipeline/consumer_pipeline.cpp)

target_include_directories(fast_channel_core
    PUBLIC include
    PRIVATE ${ZFP_INCLUDE_DIR})

if(FAST_CHANNEL_SYNC STREQUAL "spinlock")
  target_compile_definitions(fast_channel_core PUBLIC FAST_CHANNEL_SYNC_SPINLOCK)
elseif(FAST_CHANNEL_SYNC STREQUAL "semaphore")
  target_compile_definitions(fast_channel_core PUBLIC FAST_CHANNEL_SYNC_SEMAPHORE)
else()
  message(FATAL_ERROR
        "FAST_CHANNEL_SYNC must be 'spinlock' or 'semaphore', got '${FAST_CHANNEL_SYNC}'")
endif()

target_link_libraries(fast_channel_core PUBLIC zstd ${ZFP_LIBRARY} Threads::Threads)
