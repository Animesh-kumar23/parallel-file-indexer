cmake_minimum_required(VERSION 4.4)
project(parallel_file_indexer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_TSAN AND (ENABLE_ASAN OR ENABLE_UBSAN))
    message(FATAL_ERROR "ThreadSanitizer must use a separate build")
endif()

function(file_indexer_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

function(file_indexer_enable_sanitizers target)
    if(MSVC AND (ENABLE_ASAN OR ENABLE_UBSAN OR ENABLE_TSAN))
        message(FATAL_ERROR "The configured sanitizer options require GCC or Clang")
    endif()
    if(ENABLE_ASAN)
        target_compile_options(${target} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target} PUBLIC -fsanitize=address)
    endif()
    if(ENABLE_UBSAN)
        target_compile_options(${target} PUBLIC -fsanitize=undefined -fno-omit-frame-pointer)
        target_link_options(${target} PUBLIC -fsanitize=undefined)
    endif()
    if(ENABLE_TSAN)
        target_compile_options(${target} PUBLIC -fsanitize=thread -fno-omit-frame-pointer)
        target_link_options(${target} PUBLIC -fsanitize=thread)
    endif()
endfunction()

find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

add_library(file_indexer_core
    src/FileProcessor.cpp
    src/FileScanner.cpp
    src/InvertedIndex.cpp
    src/MetadataStore.cpp
    src/QueryProcessor.cpp
    src/SearchEngine.cpp
    src/ThreadPool.cpp
    src/Tokenizer.cpp
)
target_include_directories(file_indexer_core PUBLIC include)
target_link_libraries(file_indexer_core PUBLIC Threads::Threads SQLite3::SQLite3)
file_indexer_enable_warnings(file_indexer_core)
file_indexer_enable_sanitizers(file_indexer_core)

add_executable(file-indexer src/main.cpp)
target_link_libraries(file-indexer PRIVATE file_indexer_core)
file_indexer_enable_warnings(file-indexer)

# Optional developer-local CMake extensions, if present.
include("${CMAKE_CURRENT_SOURCE_DIR}/local.cmake" OPTIONAL)
