include_directories(${algorithm_SOURCE_DIR}/src)

add_executable(gcd_test gcd_test.c)
target_link_libraries(gcd_test algorithm)

add_test(NAME gcd_test1 COMMAND gcd_test 4 6 2)
add_test(NAME gcd_test2 COMMAND gcd_test 6 4 2)
add_test(NAME gcd_test3 COMMAND gcd_test 27 36 9)
add_test(NAME gcd_test4 COMMAND gcd_test 99999 3333 3)

add_executable(bubble_sort_test bubble_sort_test.c)
target_link_libraries(bubble_sort_test algorithm)
add_test(NAME bubble_sort_test COMMAND bubble_sort_test)

add_executable(heap_sort_test heap_sort_test.c)
target_link_libraries(heap_sort_test algorithm)
add_test(NAME heap_sort_test COMMAND heap_sort_test)
