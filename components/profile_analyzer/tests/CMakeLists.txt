# Unit tests of profile_analyzer.dll: the C ABI (parsing, weights, error codes, buffers).
bom_add_test(tst_profile_analyzer
    SOURCES tst_profile_analyzer.cpp
    LIBS    profile_analyzer)
