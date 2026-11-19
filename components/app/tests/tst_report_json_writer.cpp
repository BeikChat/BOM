#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include "TestData.h"
#include "bom/core/analytics/BomAnalyzer.h"
#include "infrastructure/ReportJsonWriter.h"

using namespace bom::core;
using namespace bom::infrastructure;

class TestReportJsonWriter : public QObject {
    Q_OBJECT

private slots:
    void structure();
    void writeAndReadBack();
    void writeToInvalidPathFails();
};

void TestReportJsonWriter::structure() {
    const AnalyticsReport report = BomAnalyzer::analyze(test::specExample());
    ReportMetadata meta;
    meta.sourceFile = QStringLiteral("bom.csv");
    meta.generator = QStringLiteral("test");
    meta.generatedAt = QDateTime(QDate(2026, 1, 2), QTime(3, 4, 5), QTimeZone::UTC);

    const QJsonObject json = ReportJsonWriter::toJson(report, meta);
    QVERIFY(!json.contains(QStringLiteral("schemaVersion")));
    QCOMPARE(json["generatedAt"].toString(), QStringLiteral("2026-01-02T03:04:05Z"));
    QCOMPARE(json["sourceFile"].toString(), QStringLiteral("bom.csv"));
    QCOMPARE(json["editedAfterLoad"].toBool(), false);
    QCOMPARE(json["summary"].toObject()["totalCost"].toDouble(), 1560.0);

    const QJsonArray boms = json["boms"].toArray();
    QCOMPARE(boms.size(), 2);
    QCOMPARE(boms[0].toObject()["buildingId"].toString(), QStringLiteral("B0001"));
    QCOMPARE(boms[0].toObject()["totalCost"].toDouble(), 1080.0);

    const QJsonArray profiles = json["profiles"].toArray();
    QCOMPARE(profiles.size(), 3);
    for (const QJsonValue& v : profiles) {
        const QJsonObject p = v.toObject();
        for (const char* key : {"profileName", "averageCostPerMeter", "totalLengthM",
                                "totalCost", "shareOfTotalCostPercent"})
            QVERIFY2(p.contains(QLatin1String(key)), key);
    }

    const QJsonArray top = json["topProfiles"].toArray();
    QCOMPARE(top.size(), 3);
    QCOMPARE(top[0].toObject()["profileName"].toString(),
             QStringLiteral("IPE200"));  // 600
    QCOMPARE(top[0].toObject()["totalCost"].toDouble(), 600.0);
    QVERIFY(!json.contains(QStringLiteral("topPositions")));
    QVERIFY(
        json["summary"].toObject().contains(QStringLiteral("topProfilesSharePercent")));
    QVERIFY(json["priceDeviations"].isArray());
}

void TestReportJsonWriter::writeAndReadBack() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("report.json"));

    QString error;
    QVERIFY2(ReportJsonWriter::writeFile(path, BomAnalyzer::analyze(test::twoPrices()),
                                         {}, &error),
             qPrintable(error));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    QCOMPARE(parseError.error, QJsonParseError::NoError);
    QCOMPARE(doc.object()["summary"].toObject()["totalCost"].toDouble(), 2280.0);
    QCOMPARE(doc.object()["summary"].toObject()["averageCostPerMeter"].toDouble(),
             51.8182);  // rounded to 4
}

void TestReportJsonWriter::writeToInvalidPathFails() {
    QString error;
    QVERIFY(!ReportJsonWriter::writeFile(
        QStringLiteral("Z:/definitely/missing/dir/report.json"), AnalyticsReport{}, {},
        &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestReportJsonWriter)
#include "tst_report_json_writer.moc"
