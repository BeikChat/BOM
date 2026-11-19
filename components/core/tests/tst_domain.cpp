#include <QTest>
#include <algorithm>
#include <cmath>
#include <limits>

#include "TestData.h"

using namespace bom::core;

namespace {
    bool fuzzy(double a, double b) {
        return std::abs(a - b) < 1e-9 * std::max(1.0, std::abs(b));
    }
}  // namespace

#define QCOMPARE_D(actual, expected)      \
    QVERIFY2(fuzzy((actual), (expected)), \
             qPrintable(QStringLiteral("%1 != %2").arg(actual).arg(expected)))

class TestDomain : public QObject {
    Q_OBJECT

private slots:
    void itemFormulas();
    void itemValidation();
    void bomAggregates();
    void emptyBom();
    void addItemGroupsByBuildingId();
};

void TestDomain::itemFormulas() {
    const BomItem it = test::item("IPE200", 6000, 300, 2);
    QCOMPARE_D(it.totalCost(), 600.0);
    QCOMPARE_D(it.costPerMeter(), 50.0);  // 300 / 6000 * 1000
    QCOMPARE_D(it.totalLengthMm(), 12000.0);
    QCOMPARE_D(it.totalLengthM(), 12.0);
    QCOMPARE_D(test::item("X", 0, 10, 1).costPerMeter(), 0.0);
}

void TestDomain::itemValidation() {
    constexpr double inf = std::numeric_limits<double>::infinity();
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();

    QVERIFY(BomItem::isValidLength(0.01));
    QVERIFY(!BomItem::isValidLength(0.0));
    QVERIFY(!BomItem::isValidLength(-1.0));
    QVERIFY(!BomItem::isValidLength(inf));
    QVERIFY(!BomItem::isValidLength(nan));

    QVERIFY(BomItem::isValidCost(0.0));
    QVERIFY(!BomItem::isValidCost(-0.01));
    QVERIFY(!BomItem::isValidCost(nan));

    QVERIFY(BomItem::isValidQuantity(1));
    QVERIFY(!BomItem::isValidQuantity(0));

    QVERIFY(test::item("IPE200", 6000, 300, 2).isValid());
    QVERIFY(!test::item("  ", 6000, 300, 2).isValid());
    QVERIFY(!test::item("IPE200", 0, 300, 2).isValid());
    QVERIFY(!test::item("IPE200", 6000, -1, 2).isValid());
    QVERIFY(!test::item("IPE200", 6000, 300, 0).isValid());
}

void TestDomain::bomAggregates() {
    const Bom bom =
        test::specExample().boms[0];  // IPE200 6000 300 ×2, IPE240 8000 480 ×1
    QCOMPARE(bom.positionCount(), 2);
    QCOMPARE(bom.totalQuantity(), 3);
    QCOMPARE_D(bom.totalCost(), 1080.0);
    QCOMPARE_D(bom.totalLengthMm(), 20000.0);
    QCOMPARE_D(bom.totalLengthM(), 20.0);
    QCOMPARE_D(bom.averageCostPerMeter(), 54.0);  // 1080 / 20 m, length-weighted
}

void TestDomain::emptyBom() {
    const Bom bom{QStringLiteral("B1"), {}};
    QCOMPARE(bom.positionCount(), 0);
    QCOMPARE(bom.totalQuantity(), 0);
    QCOMPARE_D(bom.totalCost(), 0.0);
    QCOMPARE_D(bom.averageCostPerMeter(), 0.0);
}

void TestDomain::addItemGroupsByBuildingId() {
    BomDataset ds;
    QVERIFY(ds.isEmpty());
    ds.addItem(QStringLiteral("B2"), test::item("IPE200", 6000, 300, 1));
    ds.addItem(QStringLiteral("B1"), test::item("HEA160", 4000, 160, 1));
    ds.addItem(QStringLiteral("B2"), test::item("IPE240", 8000, 480, 1));
    ds.addItem(QStringLiteral("B2"), test::item("IPE240", 8000, 480, 2));

    QVERIFY(!ds.isEmpty());
    QCOMPARE(ds.boms.size(), 2);
    QCOMPARE(ds.boms[0].buildingId, QStringLiteral("B2"));
    QCOMPARE(ds.boms[1].buildingId, QStringLiteral("B1"));
    QCOMPARE(ds.boms[0].positionCount(), 3);
    QCOMPARE(ds.boms[0].items[1].profileName, QStringLiteral("IPE240"));
    QCOMPARE(ds.boms[1].positionCount(), 1);
}

QTEST_APPLESS_MAIN(TestDomain)
#include "tst_domain.moc"
