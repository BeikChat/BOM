find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_library(bom_core STATIC
    include/bom/core/domain/BomItem.h
    include/bom/core/domain/Bom.h
    include/bom/core/analytics/AnalyticsReport.h
    include/bom/core/analytics/BomAnalyzer.h
    src/domain/BomItem.cpp
    src/domain/Bom.cpp
    src/analytics/BomAnalyzer.cpp
)

target_include_directories(bom_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(bom_core PUBLIC Qt6::Core)

if(BOM_BUILD_TESTS)
    add_subdirectory(tests)
endif()
