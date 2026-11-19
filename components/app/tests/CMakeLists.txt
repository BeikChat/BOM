# Unit tests of bom_app: infrastructure (CSV reader, JSON writer), tree model + delegate,
# summary panel. Input data is built with the samples of bom_core (TestData.h).

bom_add_test(tst_csv_bom_reader
    SOURCES tst_csv_bom_reader.cpp
    LIBS    bom_app)

bom_add_test(tst_report_json_writer
    SOURCES tst_report_json_writer.cpp
    LIBS    bom_app bom_core_test_data)

bom_add_test(tst_bom_tree_model
    SOURCES tst_bom_tree_model.cpp
    LIBS    bom_app bom_core_test_data)

bom_add_test(tst_summary_panel
    SOURCES tst_summary_panel.cpp
    LIBS    bom_app bom_core_test_data)
