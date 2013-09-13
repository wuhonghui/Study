include_directories(${algorithm_SOURCE_DIR}/src)

aux_source_directory(. TEST_CASES)

foreach(test_case ${TEST_CASES})
	string(REGEX REPLACE "(.*)\\.c$" "\\1" test_exe ${test_case})
	MESSAGE(${test_case})
	MESSAGE(${test_exe})
	add_executable(${test_exe} ${test_case})
	target_link_libraries(${test_exe} algorithm)
	add_test(NAME ${test_exe} COMMAND ${test_exe})
endforeach(test_case)
