find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_library(bom_app STATIC
    src/MainWindow.h
    src/MainWindow.cpp
    src/models/BomTreeModel.h
    src/models/BomTreeModel.cpp
    src/models/BomItemDelegate.h
    src/models/BomItemDelegate.cpp
    src/widgets/SummaryPanel.h
    src/widgets/SummaryPanel.cpp
    src/widgets/LoadIssuesView.h
    src/widgets/LoadIssuesView.cpp
    src/dialogs/ProfileAnalysisDialog.h
    src/dialogs/ProfileAnalysisDialog.cpp
    src/infrastructure/CsvBomReader.h
    src/infrastructure/CsvBomReader.cpp
    src/infrastructure/ReportJsonWriter.h
    src/infrastructure/ReportJsonWriter.cpp
    src/services/ProfileAnalyzerLibrary.h
    src/services/ProfileAnalyzerLibrary.cpp
)
target_include_directories(bom_app PUBLIC src/)
target_link_libraries(bom_app PUBLIC bom_core Qt6::Widgets profile_analyzer_headers)

qt_add_executable(bom_viewer WIN32 src/main.cpp)
target_link_libraries(bom_viewer PRIVATE bom_app)
target_compile_definitions(bom_viewer PRIVATE BOM_APP_VERSION="${PROJECT_VERSION}")

add_dependencies(bom_viewer profile_analyzer)

install(TARGETS bom_viewer
    RUNTIME_DEPENDENCY_SET bom_viewer_runtime
    RUNTIME DESTINATION bin)
install(RUNTIME_DEPENDENCY_SET bom_viewer_runtime
    DESTINATION bin
    DIRECTORIES "$<TARGET_FILE_DIR:Qt6::Core>"
    PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
    POST_EXCLUDE_REGEXES "[Ss]ystem32/")

install(DIRECTORY "${PROJECT_SOURCE_DIR}/data/" DESTINATION data)

qt_generate_deploy_app_script(
    TARGET bom_viewer
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    NO_UNSUPPORTED_PLATFORM_ERROR
    INCLUDE_PLUGIN_TYPES
        platforms
        styles
)
install(SCRIPT ${deploy_script})

if(BOM_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
