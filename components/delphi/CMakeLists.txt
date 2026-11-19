set(DELPHI_USE_COMPILER "AUTO" CACHE STRING "Compiler for the Delphi demo: AUTO, DCC or FPC")
set_property(CACHE DELPHI_USE_COMPILER PROPERTY STRINGS AUTO DCC FPC)

string(TOUPPER "${DELPHI_USE_COMPILER}" _delphi_mode)

if(NOT _delphi_mode MATCHES "^(AUTO|DCC|FPC)$")
    message(FATAL_ERROR
        "Invalid DELPHI_USE_COMPILER: ${DELPHI_USE_COMPILER}. Expected AUTO, DCC or FPC.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_delphi_platform Win64)
    set(_delphi_dcc_var DELPHI_DCC64_EXECUTABLE)
    set(_delphi_dcc_name dcc64)
    set(_delphi_fpc_target -Px86_64 -Twin64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(_delphi_platform Win32)
    set(_delphi_dcc_var DELPHI_DCC32_EXECUTABLE)
    set(_delphi_dcc_name dcc32)
    set(_delphi_fpc_target -Pi386 -Twin32)
else()
    message(FATAL_ERROR "Unsupported target architecture")
endif()

# Find compiler: DCC first, then FPC.
set(_delphi_compiler "")
set(_delphi_kind "")

if(NOT _delphi_mode STREQUAL "FPC")
    find_program(${_delphi_dcc_var}
        NAMES "${_delphi_dcc_name}"
        DOC "Delphi command-line compiler (${_delphi_platform})")

    if("${${_delphi_dcc_var}}")
        set(_delphi_compiler "${${_delphi_dcc_var}}")
        set(_delphi_kind DCC)
    endif()
endif()

if(NOT _delphi_compiler AND NOT _delphi_mode STREQUAL "DCC")
    find_program(DELPHI_FPC_EXECUTABLE
        NAMES fpc
        DOC "Free Pascal compiler driver")

    if(DELPHI_FPC_EXECUTABLE)
        set(_delphi_compiler "${DELPHI_FPC_EXECUTABLE}")
        set(_delphi_kind FPC)
    endif()
endif()

if(NOT _delphi_compiler)
    message(STATUS
        "Delphi demo: no suitable compiler for ${_delphi_platform}; skipped")
    return()
endif()

# Determine the output directory of the C++ DLL.
get_target_property(_delphi_out_dir profile_analyzer RUNTIME_OUTPUT_DIRECTORY)

if(NOT _delphi_out_dir)
    get_target_property(_delphi_out_dir profile_analyzer BINARY_DIR)
endif()

get_property(_delphi_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(_delphi_multi_config AND NOT _delphi_out_dir MATCHES "\\$<")
    string(APPEND _delphi_out_dir "/$<CONFIG>")
endif()

# Delphi/FPC compilation arguments.
set(_delphi_unit_dir "${CMAKE_CURRENT_BINARY_DIR}/units/$<CONFIG>")
set(_delphi_dpr "${CMAKE_CURRENT_SOURCE_DIR}/src/ProfileAnalyzerDemo.dpr")

if(_delphi_kind STREQUAL "DCC")
    set(_delphi_args
        -B
        -Q
        "-NSSystem$<SEMICOLON>Winapi$<SEMICOLON>System.Win"
        "-E${_delphi_out_dir}"
        "-NU${_delphi_unit_dir}"
        "$<IF:$<CONFIG:Debug>,-$D+,-$O+>"
    )
else()
    set(_delphi_args
        -B
        -l-
        -Mdelphiunicode
        ${_delphi_fpc_target}
        "-FE${_delphi_out_dir}"
        "-FU${_delphi_unit_dir}"
        "$<IF:$<CONFIG:Debug>,-g,-O2>"
    )
endif()

# Source files.
set(_delphi_sources
    "${_delphi_dpr}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ProfileAnalyzerWrapper.pas"
)

set(_delphi_demo_exe "${_delphi_out_dir}/ProfileAnalyzerDemo.exe")

# Build rule.
add_custom_command(
    OUTPUT "${_delphi_demo_exe}"

    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${_delphi_out_dir}"
        "${_delphi_unit_dir}"

    COMMAND "${_delphi_compiler}"
        ${_delphi_args}
        "${_delphi_dpr}"

    DEPENDS ${_delphi_sources}

    COMMENT "Compiling Delphi demo (${_delphi_platform})"
    VERBATIM
)

add_custom_target(profile_analyzer_delphi_demo ALL
    DEPENDS "${_delphi_demo_exe}"
    SOURCES ${_delphi_sources}
)

set_target_properties(profile_analyzer_delphi_demo PROPERTIES
    BOM_OUTPUT_FILE "${_delphi_demo_exe}"
)

add_dependencies(profile_analyzer_delphi_demo profile_analyzer)

install(PROGRAMS "${_delphi_demo_exe}" DESTINATION bin)
