cmake_minimum_required(VERSION 3.16)
project(AetherCopy VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -g")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")

option(BUILD_TESTS "Build tests" ON)

# Поиск библиотек
find_package(PkgConfig REQUIRED)
pkg_check_modules(URING REQUIRED liburing)
pkg_check_modules(MAGIC REQUIRED libmagic)
pkg_check_modules(ARCHIVE REQUIRED libarchive)
find_package(absl REQUIRED CONFIG)
find_package(Threads REQUIRED)

# ============================================
# БИБЛИОТЕКА
# ============================================
add_library(libaethercopy STATIC
    src/filter.cpp
    src/detector.cpp

    src/mimeTypes.cpp
    include/aethercopy/mimeTypes.h

    include/aethercopy/ThreadPool.h
    src/ThreadPool.cpp

    include/aethercopy/copiers/ICopier.h
    include/aethercopy/copiers/SyncCopier.h
    src/copiers/SyncCopier.cpp



    include/aethercopy/BackupEngine.h
    src/BackupEngine.cpp

    include/aethercopy/ArchiveHandlers/IArchiveHandler.h
    include/aethercopy/ArchiveHandlers/LibarchiveArchiveHandler.h
    src/ArchiveHandlers/LibarchiveArchiveHandler.cpp

)

target_include_directories(libaethercopy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${URING_INCLUDE_DIRS}
    ${MAGIC_INCLUDE_DIRS}
    ${ARCHIVE_INCLUDE_DIRS}
)

target_link_libraries(libaethercopy PUBLIC
    Threads::Threads
    ${URING_LIBRARIES}
    ${MAGIC_LIBRARIES}
    ${ARCHIVE_LIBRARIES}
    absl::log
)

# ============================================
# ПРОГРАММА
# ============================================
add_executable(aethercopy src/main.cpp
)

target_link_libraries(aethercopy PRIVATE libaethercopy)

# ============================================
# ЛОГИРОВАНИЕ
# ============================================
add_definitions(-DABSL_MIN_LOG_LEVEL=3)

# ============================================
# ТЕСТЫ
# ============================================
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    add_definitions(-DTEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/")
    add_executable(aethercopy_tests
        tests/test_filter.cpp
        tests/test_ThreadPool.cpp
        tests/test_BackupEngine.cpp
        tests/test_LibarchiveHandler.cpp
    )

    target_link_libraries(aethercopy_tests
        libaethercopy
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
    )

    target_include_directories(aethercopy_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    enable_testing()
    add_test(NAME aethercopy_tests COMMAND aethercopy_tests)
endif()