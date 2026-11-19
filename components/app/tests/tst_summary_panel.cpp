#include <QFormLayout>
#include <QLabel>
#include <QLocale>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>

#include "TestData.h"
#include "bom/core/analytics/BomAnalyzer.h"
#include "widgets/SummaryPanel.h"

using namespace bom::app;
using namespace bom::core;

class TestSummaryPanel : public QObject {
    Q_OBJECT

private slots:
    void emptyPanelShowsCaptionsAndDashes();
    void reportFillsFiguresAndTables();
    void clearRestoresDashes();

private:
    static QFormLayout* overview(SummaryPanel& panel) {
        auto* tabs = panel.findChild<QTabWidget*>();
        return qobject_cast<QFormLayout*>(tabs->widget(0)->layout());
    }
    static QString value(QFormLayout* form, int row) {
        return qobject_cast<QLabel*>(form->itemAt(row, QFormLayout::FieldRole)->widget())
            ->text();
    }
    static QString caption(QFormLayout* form, int row) {
        return qobject_cast<QLabel*>(form->itemAt(row, QFormLayout::LabelRole)->widget())
            ->text();
    }
};

void TestSummaryPanel::emptyPanelShowsCaptionsAndDashes() {
    SummaryPanel panel;
    QFormLayout* form = overview(panel);
    QCOMPARE(form->rowCount(), 10);
    for (int row = 0; row < form->rowCount(); ++row)
        QCOMPARE(value(form, row), QStringLiteral("—"));
    QCOMPARE(caption(form, 9),
             QStringLiteral("Доля топ-%1 профилей:").arg(BomAnalyzer::kTopProfilesCount));
    QCOMPARE(panel.findChild<QTabWidget*>()->tabText(2),
             QStringLiteral("Топ-%1 профиля").arg(BomAnalyzer::kTopProfilesCount));
}

void TestSummaryPanel::reportFillsFiguresAndTables() {
    SummaryPanel panel;
    panel.setReport(BomAnalyzer::analyze(test::specExample()));
    QFormLayout* form = overview(panel);
    QCOMPARE(form->rowCount(), 10);
    QCOMPARE(caption(form, 0), QStringLiteral("Общая стоимость по всем BOM:"));
    QCOMPARE(value(form, 0), QLocale().toString(1560.0, 'f', 2));
    QCOMPARE(value(form, 1), QStringLiteral("2"));      // BOMs
    QCOMPARE(value(form, 8), QStringLiteral("B0001"));  // most expensive BOM
    QCOMPARE(value(form, 9), QLocale().toString(100.0, 'f', 1) + QStringLiteral(" %"));

    auto* tabs = panel.findChild<QTabWidget*>();
    QCOMPARE(qobject_cast<QTableWidget*>(tabs->widget(1))->rowCount(), 2);  // per BOM
    QCOMPARE(qobject_cast<QTableWidget*>(tabs->widget(2))->rowCount(),
             3);  // top profiles
}

void TestSummaryPanel::clearRestoresDashes() {
    SummaryPanel panel;
    panel.setReport(BomAnalyzer::analyze(test::specExample()));
    panel.clear();
    QFormLayout* form = overview(panel);
    QCOMPARE(form->rowCount(), 10);
    QCOMPARE(value(form, 0), QStringLiteral("—"));
    QCOMPARE(qobject_cast<QTableWidget*>(panel.findChild<QTabWidget*>()->widget(1))
                 ->rowCount(),
             0);
}

QTEST_MAIN(TestSummaryPanel)
#include "tst_summary_panel.moc"
