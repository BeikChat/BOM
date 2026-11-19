#include "bom/core/analytics/BomAnalyzer.h"
#include "infrastructure/CsvBomReader.h"
#include "infrastructure/ReportJsonWriter.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

using namespace bom::core;
using namespace bom::infrastructure;

class TestReportPipeline : public QObject
{
    Q_OBJECT

private slots:
    void sampleFileToJsonReport();
    void fileWithErrorsStillGivesReport();

private:
    static QJsonObject runPipeline(const QString& csvPath);
};

QJsonObject TestReportPipeline::runPipeline(const QString& csvPath)
{
    const CsvLoadResult loaded = CsvBomReader().readFile(csvPath);
    if (!loaded.succeeded()) {
        QTest::qFail(qPrintable(loaded.fatalError), __FILE__, __LINE__);
        return {};
    }

    QTemporaryDir dir;
    const QString jsonPath = dir.filePath(QStringLiteral("report.json"));
    ReportMetadata meta;
    meta.sourceFile = QFileInfo(csvPath).fileName();
    QString error;
    if (!ReportJsonWriter::writeFile(jsonPath, BomAnalyzer::analyze(loaded.dataset), meta, &error)) {
        QTest::qFail(qPrintable(error), __FILE__, __LINE__);
        return {};
    }

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

void TestReportPipeline::sampleFileToJsonReport()
{
    const QJsonObject json = runPipeline(QStringLiteral(BOM_TEST_DATA_DIR "/sample_bom.csv"));
    QCOMPARE(json["sourceFile"].toString(), QStringLiteral("sample_bom.csv"));

    // Expected values are computed by hand from data/sample_bom.csv.
    const QJsonObject summary = json["summary"].toObject();
    QCOMPARE(summary["totalCost"].toDouble(), 29466.0);
    QCOMPARE(summary["bomCount"].toInt(), 4);
    QCOMPARE(summary["positionCount"].toInt(), 17);
    QCOMPARE(summary["uniqueProfileCount"].toInt(), 11);
    QCOMPARE(summary["mostExpensiveBom"].toString(), QStringLiteral("B0004"));

    // BOMs with their totals, in the order of the file.
    const QJsonArray boms = json["boms"].toArray();
    const QList<QPair<QString, double>> expectedBoms = {
        {QStringLiteral("B0001"), 2670.0}, {QStringLiteral("B0002"), 5700.0},
        {QStringLiteral("B0003"), 7904.0}, {QStringLiteral("B0004"), 13192.0}};
    QCOMPARE(boms.size(), expectedBoms.size());
    for (qsizetype i = 0; i < boms.size(); ++i) {
        QCOMPARE(boms[i].toObject()["buildingId"].toString(), expectedBoms[i].first);
        QCOMPARE(boms[i].toObject()["totalCost"].toDouble(), expectedBoms[i].second);
    }

    // Top-3 profiles by Σ Cost × Quantity.
    const QJsonArray top = json["topProfiles"].toArray();
    QCOMPARE(top.size(), 3);
    QCOMPARE(top[0].toObject()["profileName"].toString(), QStringLiteral("IPE200")); // 6360
    QCOMPARE(top[0].toObject()["totalCost"].toDouble(), 6360.0);
    QCOMPARE(top[1].toObject()["profileName"].toString(), QStringLiteral("HEB300")); // 5800
    QCOMPARE(top[2].toObject()["profileName"].toString(), QStringLiteral("IPE300")); // 3180

    // Every unique profile with average price per metre, total length and total cost;
    // the profile totals add up to the grand total.
    const QJsonArray profiles = json["profiles"].toArray();
    QCOMPARE(profiles.size(), 11);
    double sum = 0.0;
    for (const QJsonValue& v : profiles) {
        const QJsonObject p = v.toObject();
        QVERIFY(p["averageCostPerMeter"].toDouble() > 0.0);
        QVERIFY(p["totalLengthM"].toDouble() > 0.0);
        sum += p["totalCost"].toDouble();
    }
    QCOMPARE(sum, 29466.0);
}

void TestReportPipeline::fileWithErrorsStillGivesReport()
{
    // Invalid rows are skipped, the report is built from the valid ones.
    const QJsonObject json = runPipeline(QStringLiteral(BOM_TEST_DATA_DIR "/sample_bom_with_errors.csv"));
    const QJsonObject summary = json["summary"].toObject();
    QVERIFY(summary["bomCount"].toInt() > 0);
    QVERIFY(summary["totalCost"].toDouble() > 0.0);
    QCOMPARE(json["boms"].toArray().size(), summary["bomCount"].toInt());
}

QTEST_GUILESS_MAIN(TestReportPipeline)
#include "tst_report_pipeline.moc"
