function(create_test exec_name file_name lib_list test_name)
    add_executable(${exec_name} ${file_name})
    target_link_libraries(${exec_name} PRIVATE ${lib_list})
    add_test(NAME ${test_name} COMMAND ${exec_name})
endfunction()

create_test(example-test test_main.c "cmocka" ExampleTest)
