# Integration tests: several components together, with the real DLL and real files.

# ---- Delphi wrapper <-> profile_analyzer.dll ------------------------------------
if(TARGET profile_analyzer_delphi_demo)
    get_target_property(delphi_demo profile_analyzer_delphi_demo BOM_OUTPUT_FILE)

    add_test(NAME delphi_demo_valid_profiles
             COMMAND "${delphi_demo}" IPE200 HEA160 L150*5 RHS100x50x4 CHS114.3x4)

    add_test(NAME delphi_demo_converts_result COMMAND "${delphi_demo}" IPE200)
    set_tests_properties(delphi_demo_converts_result PROPERTIES
        # The decimal separator follows the system locale.
        PASS_REGULAR_EXPRESSION "IPE200 +type=IPE +height= +200[.,]0 mm +weight= +22[.,]400")

    add_test(NAME delphi_demo_reports_error_code COMMAND "${delphi_demo}" XYZ100)
    set_tests_properties(delphi_demo_reports_error_code PROPERTIES
        PASS_REGULAR_EXPRESSION "XYZ100 +FAILED, code -4")
endif()

if(NOT BOM_BUILD_APP)
    return() 
endif()

# ---- Qt application <-> profile_analyzer.dll ------------------------------------
bom_add_test(tst_profile_analyzer_library
    SOURCES tst_profile_analyzer_library.cpp
    LIBS    bom_app)
add_dependencies(tst_profile_analyzer_library profile_analyzer)

# ---- CSV file -> report -> JSON file --------------------------------------------
bom_add_test(tst_report_pipeline
    SOURCES tst_report_pipeline.cpp
    LIBS    bom_app)
