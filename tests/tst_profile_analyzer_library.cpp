#include "infrastructure/CsvBomReader.h"
#include "services/ProfileAnalyzerLibrary.h"
#include "profile_analyzer/profile_analyzer.h"

#include <QTest>
#include <cmath>

using namespace bom::app;

class TestProfileAnalyzerLibrary : public QObject
{
    Q_OBJECT

private slots:
    void loadsDllAndAnalyzes();
    void reportsDllErrorCodeAndText();
    void missingDllIsReported();
    void everyProfileOfSampleFileIsKnown();
};

void TestProfileAnalyzerLibrary::loadsDllAndAnalyzes()
{
    ProfileAnalyzerLibrary library;
    QVERIFY2(library.isAvailable(), qPrintable(library.unavailableReason()));

    const ProfileAnalysisResult r1 = library.analyze(QStringLiteral("IPE200"));
    QVERIFY(r1.has_value());
    QCOMPARE(r1.value().code, PA_OK);
    QCOMPARE(r1.value().profileType, QStringLiteral("IPE"));
    QCOMPARE(r1.value().nominalHeightMm, 200.0);
    QCOMPARE(r1.value().weightPerMeterKg, 22.4);

    const ProfileAnalysisResult r2 = library.analyze(QStringLiteral("L150*5"));
    QVERIFY(r2.has_value());
    QCOMPARE(r2.value().code, PA_OK);
    QCOMPARE(r2.value().profileType, QStringLiteral("L"));
}

void TestProfileAnalyzerLibrary::reportsDllErrorCodeAndText()
{
    ProfileAnalyzerLibrary library;
    const ProfileAnalysisResult r = library.analyze(QStringLiteral("XYZ100"));
    QVERIFY(!r.has_value());
    QCOMPARE(r.error().code, PA_E_UNKNOWN_TYPE);
    QCOMPARE(r.error().message, QStringLiteral("Profile type is not supported"));
}

void TestProfileAnalyzerLibrary::missingDllIsReported()
{
    ProfileAnalyzerLibrary library(QStringLiteral("no_such_profile_analyzer.dll"));
    QVERIFY(!library.isAvailable());
    QVERIFY(!library.unavailableReason().isEmpty());

    const ProfileAnalysisResult r = library.analyze(QStringLiteral("IPE200"));
    QVERIFY(!r.has_value());
    QCOMPARE(r.error().code, kProfileAnalyzerNotLoaded);
}

void TestProfileAnalyzerLibrary::everyProfileOfSampleFileIsKnown()
{
    // Profile names of the shipped sample, as loaded by the application, are understood by the DLL.
    const auto loaded =
        bom::infrastructure::CsvBomReader().readFile(QStringLiteral(BOM_TEST_DATA_DIR "/sample_bom.csv"));
    QVERIFY(loaded.succeeded());

    ProfileAnalyzerLibrary library;
    for (const bom::core::Bom& bom : loaded.dataset.boms) {
        for (const bom::core::BomItem& item : bom.items) {
            const ProfileAnalysisResult r = library.analyze(item.profileName);
            QVERIFY2(r.has_value(), qPrintable(item.profileName + QStringLiteral(": ") +
                                               r.value().message));
            QVERIFY(r.value().weightPerMeterKg > 0.0);
        }
    }
}

QTEST_GUILESS_MAIN(TestProfileAnalyzerLibrary)
#include "tst_profile_analyzer_library.moc"
