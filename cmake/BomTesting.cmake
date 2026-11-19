include_guard(GLOBAL)

# bom_add_test(<name> SOURCES <file>... [LIBS <target>...])
function(bom_add_test name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;LIBS")

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBS})
    target_compile_definitions(${name} PRIVATE BOM_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
    set_target_properties(${name} PROPERTIES AUTOMOC ON)

    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()
