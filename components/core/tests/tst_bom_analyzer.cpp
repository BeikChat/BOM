#include <QTest>

#include "TestData.h"
#include "bom/core/analytics/BomAnalyzer.h"

using namespace bom::core;

namespace {
    bool fuzzy(double a, double b) {
        return std::abs(a - b) < 1e-9 * std::max(1.0, std::abs(b));
    }
}  // namespace

#define QCOMPARE_D(actual, expected)      \
    QVERIFY2(fuzzy((actual), (expected)), \
             qPrintable(QStringLiteral("%1 != %2").arg(actual).arg(expected)))

class TestBomAnalyzer : public QObject {
    Q_OBJECT

private slots:
    void bomTotals();
    void overallSummary();
    void topProfilesAreSummedPerProfileName();
    void topProfilesTiesKeepSourceOrder();
    void topProfilesWhenFewerThanN();
    void profileAveragesAreLengthWeighted();
    void priceDeviations();
    void emptyDataset();
};

void TestBomAnalyzer::bomTotals() {
    const AnalyticsReport r = BomAnalyzer::analyze(test::specExample());
    QCOMPARE(r.boms.size(), 2);
    QCOMPARE(r.boms[0].buildingId, QStringLiteral("B0001"));
    QCOMPARE_D(r.boms[0].totalCost, 1080.0);  // 300*2 + 480*1
    QCOMPARE(r.boms[0].totalQuantity, 3);
    QCOMPARE_D(r.boms[0].totalLengthM, 20.0);
    QCOMPARE_D(r.boms[0].averageCostPerMeter, 54.0);
    QCOMPARE_D(r.boms[1].totalCost, 480.0);  // 160*3
    QCOMPARE_D(r.boms[0].shareOfTotalCostPercent + r.boms[1].shareOfTotalCostPercent,
               100.0);
}

void TestBomAnalyzer::overallSummary() {
    const OverallSummary s = BomAnalyzer::analyze(test::twoPrices()).summary;
    QCOMPARE_D(s.totalCost, 2280.0);
    QCOMPARE(s.bomCount, 2);
    QCOMPARE(s.positionCount, 4);
    QCOMPARE(s.uniqueProfileCount, 3);
    QCOMPARE(s.totalQuantity, 7);
    QCOMPARE_D(s.totalLengthM, 44.0);
    QCOMPARE_D(s.averageBomCost, 1140.0);
    QCOMPARE_D(s.averageCostPerMeter, 2280.0 / 44.0);
    QCOMPARE(s.mostExpensiveBomId, QStringLiteral("B0002"));
    QCOMPARE_D(s.topProfilesSharePercent, 100.0);
}

void TestBomAnalyzer::topProfilesAreSummedPerProfileName() {
    BomDataset ds;
    ds.boms.append(Bom{QStringLiteral("B1"),
                       {test::item("A", 1000, 1000, 1), test::item("B", 1000, 600, 1),
                        test::item("C", 1000, 500, 1)}});
    ds.boms.append(Bom{QStringLiteral("B2"),
                       {test::item("B", 1000, 300, 2), test::item("D", 1000, 100, 1)}});

    const AnalyticsReport r = BomAnalyzer::analyze(ds);
    QCOMPARE(r.topProfiles.size(), 3);
    QCOMPARE(r.topProfiles[0].profileName, QStringLiteral("B"));
    QCOMPARE_D(r.topProfiles[0].totalCost, 1200.0);
    QCOMPARE(r.topProfiles[0].positionCount, 2);
    QCOMPARE(r.topProfiles[0].bomCount, 2);
    QCOMPARE(r.topProfiles[1].profileName, QStringLiteral("A"));
    QCOMPARE(r.topProfiles[2].profileName, QStringLiteral("C"));
    QCOMPARE_D(r.topProfiles[0].shareOfTotalCostPercent, 1200.0 / 2800.0 * 100.0);
    QCOMPARE_D(r.summary.topProfilesSharePercent, 2700.0 / 2800.0 * 100.0);
}

void TestBomAnalyzer::topProfilesTiesKeepSourceOrder() {
    const AnalyticsReport r = BomAnalyzer::analyze(test::twoPrices());
    QCOMPARE(r.topProfiles.size(), 3);
    QCOMPARE(r.topProfiles[0].profileName, QStringLiteral("IPE200"));
    QCOMPARE_D(r.topProfiles[0].totalCost, 1320.0);
    QCOMPARE(r.topProfiles[1].profileName, QStringLiteral("IPE240"));
    QCOMPARE(r.topProfiles[2].profileName, QStringLiteral("HEA160"));
}

void TestBomAnalyzer::topProfilesWhenFewerThanN() {
    BomDataset ds;
    ds.boms.append(Bom{QStringLiteral("B1"), {test::item("IPE200", 6000, 300, 1)}});
    QCOMPARE(BomAnalyzer::analyze(ds).topProfiles.size(), 1);
}

void TestBomAnalyzer::profileAveragesAreLengthWeighted() {
    const AnalyticsReport r = BomAnalyzer::analyze(test::twoPrices());
    QCOMPARE(r.profiles.size(), 3);
    const ProfileSummary& ipe = r.profiles[0];
    QCOMPARE(ipe.profileName, QStringLiteral("IPE200"));
    QCOMPARE(ipe.positionCount, 2);
    QCOMPARE(ipe.bomCount, 2);
    QCOMPARE(ipe.totalQuantity, 3);
    QCOMPARE_D(ipe.totalLengthM, 24.0);
    QCOMPARE_D(ipe.totalCost, 1320.0);
    QCOMPARE_D(ipe.averageCostPerMeter, 55.0);
    QCOMPARE_D(ipe.minCostPerMeter, 50.0);
    QCOMPARE_D(ipe.maxCostPerMeter, 60.0);
    QCOMPARE_D(ipe.priceSpreadPercent, 20.0);
}

void TestBomAnalyzer::priceDeviations() {
    QCOMPARE(BomAnalyzer::kPriceDeviationThresholdPercent, 10.0);

    QVERIFY(BomAnalyzer::analyze(test::twoPrices()).priceDeviations.isEmpty());

    BomDataset ds;
    ds.boms.append(Bom{QStringLiteral("B1"), {test::item("IPE200", 6000, 300, 1)}});
    ds.boms.append(
        Bom{QStringLiteral("B2"),
            {test::item("IPE200", 6000, 420, 1), test::item("HEA160", 4000, 160, 1)}});
    const AnalyticsReport r = BomAnalyzer::analyze(ds);
    QCOMPARE(r.priceDeviations.size(), 2);
    const auto it = std::find_if(
        r.priceDeviations.cbegin(), r.priceDeviations.cend(),
        [](const PriceDeviation& d) { return d.buildingId == QStringLiteral("B2"); });
    QVERIFY(it != r.priceDeviations.cend());
    QCOMPARE(it->profileName, QStringLiteral("IPE200"));
    QCOMPARE_D(it->profileAverageCostPerMeter, 60.0);
    QCOMPARE_D(it->deviationPercent, 10.0 / 60.0 * 100.0);
    QCOMPARE_D(it->overpayment, 60.0);  // (70 - 60) * 6 m
}

void TestBomAnalyzer::emptyDataset() {
    const AnalyticsReport r = BomAnalyzer::analyze(BomDataset{});
    QCOMPARE(r.summary.totalCost, 0.0);
    QVERIFY(r.boms.isEmpty());
    QVERIFY(r.topProfiles.isEmpty());
    QVERIFY(r.profiles.isEmpty());
}

QTEST_APPLESS_MAIN(TestBomAnalyzer)
#include "tst_bom_analyzer.moc"
