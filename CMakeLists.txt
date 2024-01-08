cmake_minimum_required(VERSION 3.1)
project(
    JoyChapter4
    LANGUAGES C
)

include(CTest)

add_library(array array.c)
add_library(linked_lists linked_lists.c)
add_library(chained_hash chained_hash.c linked_lists.c)
add_library(open_addressing open_addressing.c)

add_executable(array_test array_test_test.c)
target_link_libraries(array_test array)
add_test(
    NAME array_test 
    COMMAND array_test
)

add_executable(linked_lists_test linked_lists_test.c)
target_link_libraries(linked_lists_test linked_lists)
add_test(
    NAME linked_lists_test 
    COMMAND linked_lists_test
)

add_executable(chained_hash_test chained_hash_test.c)
target_link_libraries(chained_hash_test chained_hash)
add_test(
    NAME chained_hash_test 
    COMMAND chained_hash_test
)

add_executable(open_addressing_test open_addressing_test.c)
target_link_libraries(open_addressing_test open_addressing)
add_test(
    NAME open_addressing_test 
    COMMAND open_addressing_test
)
