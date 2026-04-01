cmake_minimum_required(VERSION 3.16)
project(AetherCopy VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" ON)

# Поиск библиотек
find_package(PkgConfig REQUIRED)
pkg_check_modules(URING REQUIRED liburing)
pkg_check_modules(MAGIC REQUIRED libmagic)
find_package(Threads REQUIRED)

# ============================================
# БИБЛИОТЕКА
# ============================================
add_library(libaethercopy STATIC           # ← переименовал в aethercopy_lib
    src/filter.cpp
    src/detector.cpp
    src/thread_pool.cpp
    src/async_copier.cpp
)

target_include_directories(libaethercopy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${URING_INCLUDE_DIRS}
    ${MAGIC_INCLUDE_DIRS}
)

target_link_libraries(libaethercopy PUBLIC
    Threads::Threads
    ${URING_LIBRARIES}
    ${MAGIC_LIBRARIES}
)

# ============================================
# ПРОГРАММА
# ============================================
add_executable(aethercopy src/main.cpp)     # ← программа с именем aethercopy

target_link_libraries(aethercopy PRIVATE libaethercopy)

# ============================================
# ТЕСТЫ
# ============================================
if(BUILD_TESTS)
    find_package(GTest REQUIRED)

    add_executable(aethercopy_tests
        tests/test_filter.cpp
    )

    target_link_libraries(aethercopy_tests
        libaethercopy
        GTest::gtest
        GTest::gtest_main
    )

    target_include_directories(aethercopy_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    enable_testing()
    add_test(NAME aethercopy_tests COMMAND aethercopy_tests)
endif()