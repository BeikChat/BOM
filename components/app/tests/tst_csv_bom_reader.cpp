#include <QTest>

#include "infrastructure/CsvBomReader.h"

using namespace bom::core;
using namespace bom::infrastructure;

class TestCsvBomReader : public QObject {
    Q_OBJECT

private slots:
    void specExampleWithoutHeader();
    void headerIsDetectedAndSkipped();
    void headerWithMissingColumnIsFatal();
    void headerInOtherOrderIsFatal();
    void utf8BomAndCrLf();
    void invalidRowsAreSkippedAndReported();
    void exampleFileWithErrors();
    void emptyInputIsFatal();
    void headerOnlyIsFatal();
    void allRowsInvalidIsFatal();
    void missingFileIsFatal();
};

static const char* const kSpecExample =
    "B0001,IPE200,6000,300,2\n"
    "B0001,IPE240,8000,480,1\n"
    "B0002,HEA160,4000,160,3\n";

void TestCsvBomReader::specExampleWithoutHeader() {
    const CsvLoadResult r = CsvBomReader().readText(QString::fromLatin1(kSpecExample));
    QVERIFY(r.succeeded());
    QVERIFY(r.issues.isEmpty());
    QCOMPARE(r.dataRowCount, 3);
    QCOMPARE(r.acceptedRowCount, 3);
    QCOMPARE(r.dataset.boms.size(), 2);

    const Bom& b1 = r.dataset.boms[0];
    QCOMPARE(b1.buildingId, QStringLiteral("B0001"));
    QCOMPARE(b1.items.size(), 2);
    QCOMPARE(b1.items[0].profileName, QStringLiteral("IPE200"));
    QCOMPARE(b1.items[0].lengthMm, 6000.0);
    QCOMPARE(b1.items[0].cost, 300.0);
    QCOMPARE(b1.items[0].quantity, 2);
    QCOMPARE(r.dataset.boms[1].items[0].profileName, QStringLiteral("HEA160"));
}

void TestCsvBomReader::headerIsDetectedAndSkipped() {
    const CsvLoadResult r = CsvBomReader().readText(
        QStringLiteral("BuildingID,ProfileName,Length_mm,Cost,Quantity\n") +
        QString::fromLatin1(kSpecExample));
    QVERIFY(r.succeeded());
    QCOMPARE(r.dataRowCount, 3);
    QCOMPARE(r.acceptedRowCount, 3);
    QCOMPARE(r.dataset.boms[0].items[0].profileName, QStringLiteral("IPE200"));
}

void TestCsvBomReader::headerWithMissingColumnIsFatal() {
    const CsvLoadResult r = CsvBomReader().readText(
        QStringLiteral("BuildingID,ProfileName,Length_mm,Cost\nB1,IPE200,6000,300\n"));
    QVERIFY(!r.succeeded());
    QVERIFY(r.fatalError.contains(QStringLiteral("Quantity")));
    QVERIFY(r.dataset.boms.isEmpty());
}

void TestCsvBomReader::headerInOtherOrderIsFatal() {
    const CsvLoadResult r = CsvBomReader().readText(QStringLiteral(
        "BuildingID,ProfileName,Cost,Length_mm,Quantity\nB1,IPE200,300,6000,2\n"));
    QVERIFY(!r.succeeded());
    QCOMPARE(r.dataRowCount, 0);  // nothing is parsed after a wrong header
}

void TestCsvBomReader::utf8BomAndCrLf() {
    const CsvLoadResult r = CsvBomReader().readText(
        QString(QChar(0xFEFF)) + QStringLiteral("BuildingID,ProfileName,Length_mm,Cost,"
                                                "Quantity\r\nB1,IPE200,6000,300,2\r\n"));
    QVERIFY(r.succeeded());
    QCOMPARE(r.dataRowCount, 1);
    QCOMPARE(r.dataset.boms[0].items[0].quantity, 2);
}

void TestCsvBomReader::invalidRowsAreSkippedAndReported() {
    const CsvLoadResult r =
        CsvBomReader().readText(QStringLiteral("B1,IPE200,6000,300,2\n"
                                               "B1,IPE200,abc,300,2\n"
                                               "B1,IPE200,6000,-1,0\n"
                                               "B1,IPE200,6000\n"));
    QVERIFY(r.succeeded());
    QCOMPARE(r.acceptedRowCount, 1);
    QCOMPARE(r.rejectedRowCount(), 3);
    QCOMPARE(r.errorCount(), 3);
    QCOMPARE(r.issues[0].line, 2);
    QVERIFY(r.issues[0].message.contains(QStringLiteral("abc")));
    QVERIFY(r.issues[1].message.contains(QStringLiteral("Cost")));
    QVERIFY(r.issues[1].message.contains(QStringLiteral("Quantity")));
    QCOMPARE(r.issues[2].line, 4);
}

void TestCsvBomReader::exampleFileWithErrors() {
    const CsvLoadResult r = CsvBomReader().readFile(
        QStringLiteral(BOM_TEST_DATA_DIR "/sample_bom_with_errors.csv"));
    QVERIFY2(r.succeeded(), qPrintable(r.fatalError));
    QCOMPARE(r.dataRowCount, 15);
    QCOMPARE(r.acceptedRowCount, 6);
    QCOMPARE(r.errorCount(), 9);
    QCOMPARE(r.warningCount(), 1);
    QCOMPARE(r.dataset.boms.size(), 4);

    QList<int> errorLines;
    for (const ParseIssue& issue : r.issues) {
        if (issue.severity == IssueSeverity::Error)
            errorLines << issue.line;
    }
    QCOMPARE(errorLines, (QList<int>{6, 7, 8, 9, 10, 11, 12, 13, 15}));
}

void TestCsvBomReader::emptyInputIsFatal() {
    QVERIFY(!CsvBomReader().readText(QString()).succeeded());
    QVERIFY(!CsvBomReader().readText(QStringLiteral("\n\n  \n")).succeeded());
}

void TestCsvBomReader::headerOnlyIsFatal() {
    QVERIFY(
        !CsvBomReader()
             .readText(QStringLiteral("BuildingID,ProfileName,Length_mm,Cost,Quantity\n"))
             .succeeded());
}

void TestCsvBomReader::allRowsInvalidIsFatal() {
    const CsvLoadResult r = CsvBomReader().readText(
        QStringLiteral("B1,IPE200,x,300,2\nB1,IPE200,6000,300,y\n"));
    QVERIFY(!r.succeeded());
    QCOMPARE(r.errorCount(), 2);
    QVERIFY(r.dataset.boms.isEmpty());
}

void TestCsvBomReader::missingFileIsFatal() {
    const CsvLoadResult r = CsvBomReader().readFile(QStringLiteral("does/not/exist.csv"));
    QVERIFY(!r.succeeded());
    QVERIFY(!r.fatalError.isEmpty());
}

QTEST_APPLESS_MAIN(TestCsvBomReader)
#include "tst_csv_bom_reader.moc"
