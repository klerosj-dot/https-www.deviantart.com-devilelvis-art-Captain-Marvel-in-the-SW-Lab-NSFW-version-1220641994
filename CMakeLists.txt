cmake_minimum_required(VERSION 3.16)

project(uart_driver C)

add_library(uart STATIC src/uart.c)
target_include_directories(uart PUBLIC include)
target_compile_features(uart PUBLIC c_std_11)

add_library(fmcw STATIC src/fmcw.c)
target_include_directories(fmcw PUBLIC include)
target_compile_features(fmcw PUBLIC c_std_11)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(uart PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if (BUILD_TESTING)
    add_executable(uart_tests tests/test_uart.c)
    target_link_libraries(uart_tests PRIVATE uart)
    add_test(NAME uart_tests COMMAND uart_tests)

    add_executable(fmcw_tests tests/test_fmcw.c)
    target_link_libraries(fmcw_tests PRIVATE fmcw)
    add_test(NAME fmcw_tests COMMAND fmcw_tests)
endif()

add_executable(uart_demo examples/demo.c)
target_link_libraries(uart_demo PRIVATE uart)
