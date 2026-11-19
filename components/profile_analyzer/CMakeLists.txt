configure_file(src/profile_analyzer.rc.in
    "${CMAKE_CURRENT_BINARY_DIR}/profile_analyzer.rc" @ONLY)

add_library(profile_analyzer SHARED
    include/profile_analyzer/profile_analyzer.h
    src/ProfileTypes.h
    src/ProfileNameParser.h
    src/ProfileNameParser.cpp
    src/WeightCalculator.h
    src/WeightCalculator.cpp
    src/profile_analyzer_api.cpp
    src/profile_analyzer.def
    "${CMAKE_CURRENT_BINARY_DIR}/profile_analyzer.rc"
)

target_include_directories(profile_analyzer
    PUBLIC  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PRIVATE src)
target_compile_definitions(profile_analyzer PRIVATE PROFILE_ANALYZER_BUILD)

set_target_properties(profile_analyzer PROPERTIES
    PREFIX ""
    OUTPUT_NAME "profile_analyzer"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(profile_analyzer_headers INTERFACE)
target_include_directories(profile_analyzer_headers INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")

install(TARGETS profile_analyzer
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib)
install(FILES include/profile_analyzer/profile_analyzer.h
    DESTINATION include/profile_analyzer)
