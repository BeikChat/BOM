# Unit tests of bom_core: entities and the analyzer.

# Sample datasets (TestData.h), shared with the unit tests of the application.
add_library(bom_core_test_data INTERFACE)
target_include_directories(bom_core_test_data INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(bom_core_test_data INTERFACE bom_core)

bom_add_test(tst_domain
    SOURCES tst_domain.cpp
    LIBS    bom_core_test_data)

bom_add_test(tst_bom_analyzer
    SOURCES tst_bom_analyzer.cpp
    LIBS    bom_core_test_data)
