#include "widgets/SummaryPanel.h"

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include "bom/core/analytics/BomAnalyzer.h"

namespace bom::app {
    namespace {
        inline QString num(double v, int decimals = 2) {
            return QLocale().toString(v, 'f', decimals);
        }

        // Formatted number that sorts by value, not by text.
        class NumericItem : public QTableWidgetItem {
        public:
            NumericItem(double value, int decimals)
                : QTableWidgetItem(num(value, decimals)), m_value(value) {
                setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }

            bool operator<(const QTableWidgetItem& other) const override {
                if (auto* n = dynamic_cast<const NumericItem*>(&other))
                    return m_value < n->m_value;
                return QTableWidgetItem::operator<(other);
            }

        private:
            double m_value;
        };

        using Row = QList<QTableWidgetItem*>;

        inline QTableWidgetItem* text(const QString& s) {
            return new QTableWidgetItem(s);
        }

        inline QTableWidgetItem* number(double value, int decimals) {
            return new NumericItem(value, decimals);
        }

        inline QTableWidget* createTable(const QStringList& headers) {
            auto* table = new QTableWidget(0, static_cast<int>(headers.size()));
            table->setHorizontalHeaderLabels(headers);
            table->setEditTriggers(QAbstractItemView::NoEditTriggers);
            table->setSelectionBehavior(QAbstractItemView::SelectRows);
            table->setAlternatingRowColors(true);
            table->verticalHeader()->setVisible(false);
            table->verticalHeader()->setDefaultSectionSize(table->fontMetrics().height() +
                                                           8);
            table->horizontalHeader()->setStretchLastSection(true);
            table->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
            table->setSortingEnabled(true);
            return table;
        }

        template <typename T, typename MakeRow>
        inline void fillTable(QTableWidget* table, const QList<T>& rows,
                              MakeRow makeRow) {
            table->setSortingEnabled(false);
            table->clearContents();
            table->setRowCount(static_cast<int>(rows.size()));
            for (int r = 0; r < rows.size(); ++r) {
                const Row cells = makeRow(rows[r]);
                for (int c = 0; c < cells.size(); ++c)
                    table->setItem(r, c, cells[c]);
            }
            table->setSortingEnabled(true);
            table->resizeColumnsToContents();
        }

        inline Row bomRow(const core::BomSummary& b) {
            return {text(b.buildingId),
                    number(b.positionCount, 0),
                    number(b.totalQuantity, 0),
                    number(b.totalLengthM, 1),
                    number(b.totalCost, 2),
                    number(b.averageCostPerMeter, 2),
                    number(b.shareOfTotalCostPercent, 1)};
        }

        inline Row profileRow(const core::ProfileSummary& p) {
            return {text(p.profileName),
                    number(p.positionCount, 0),
                    number(p.bomCount, 0),
                    number(p.totalQuantity, 0),
                    number(p.totalLengthM, 1),
                    number(p.totalCost, 2),
                    number(p.shareOfTotalCostPercent, 1),
                    number(p.averageCostPerMeter, 2),
                    number(p.minCostPerMeter, 2),
                    number(p.maxCostPerMeter, 2),
                    number(p.priceSpreadPercent, 1)};
        }

        struct Figure {
            QString caption;
            QString value;
            QString toolTip;
        };

        inline QList<Figure> overviewFigures(const core::OverallSummary& s, bool empty) {
            const auto v = [empty](const QString& text) {
                return empty || text.isEmpty() ? QStringLiteral("—") : text;
            };
            const int top = core::BomAnalyzer::kTopProfilesCount;
            return {
                {SummaryPanel::tr("Общая стоимость по всем BOM:"),
                 v(num(s.totalCost)),
                 {}},
                {SummaryPanel::tr("Количество BOM:"), v(QString::number(s.bomCount)), {}},
                {SummaryPanel::tr("Позиций:"), v(QString::number(s.positionCount)), {}},
                {SummaryPanel::tr("Уникальных профилей:"),
                 v(QString::number(s.uniqueProfileCount)),
                 {}},
                {SummaryPanel::tr("Штук (всего):"),
                 v(QString::number(s.totalQuantity)),
                 {}},
                {SummaryPanel::tr("Общая длина:"),
                 v(SummaryPanel::tr("%1 м").arg(num(s.totalLengthM, 1))),
                 {}},
                {SummaryPanel::tr("Средняя стоимость BOM:"),
                 v(num(s.averageBomCost)),
                 {}},
                {SummaryPanel::tr("Средняя цена метра:"), v(num(s.averageCostPerMeter)),
                 SummaryPanel::tr("Общая стоимость / общая длина")},
                {SummaryPanel::tr("Самый дорогой BOM:"), v(s.mostExpensiveBomId), {}},
                {SummaryPanel::tr("Доля топ-%1 профилей:").arg(top),
                 v(SummaryPanel::tr("%1 %").arg(num(s.topProfilesSharePercent, 1))),
                 SummaryPanel::tr("Какую часть бюджета занимают %1 самых дорогих профиля")
                     .arg(top)},
            };
        }

        inline void fillForm(QFormLayout* form, const QList<Figure>& figures) {
            while (form->rowCount() > 0)
                form->removeRow(0);
            for (const Figure& f : figures) {
                auto* caption = new QLabel(f.caption);
                auto* value = new QLabel(f.value);
                value->setTextInteractionFlags(Qt::TextSelectableByMouse);
                caption->setToolTip(f.toolTip);
                value->setToolTip(f.toolTip);
                form->addRow(caption, value);
            }
            if (QLayoutItem* first = form->itemAt(0, QFormLayout::FieldRole)) {
                QFont big = first->widget()->font();
                big.setPointSizeF(big.pointSizeF() * 1.4);
                big.setBold(true);
                first->widget()->setFont(big);
            }
        }

        inline Row deviationRow(const core::PriceDeviation& d) {
            QTableWidgetItem* deviation = number(d.deviationPercent, 1);
            deviation->setForeground(d.deviationPercent > 0 ? QColor(0xC0, 0x39, 0x2B)
                                                            : QColor(0x1E, 0x84, 0x49));
            return {text(d.buildingId),
                    text(d.profileName),
                    number(d.costPerMeter, 2),
                    number(d.profileAverageCostPerMeter, 2),
                    deviation,
                    number(d.overpayment, 2)};
        }
    }  // namespace

    SummaryPanel::SummaryPanel(QWidget* parent) : QWidget(parent) {
        m_tabs = new QTabWidget(this);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_tabs);

        auto* overview = new QWidget(this);
        m_overview = new QFormLayout(overview);
        m_overview->setLabelAlignment(Qt::AlignLeft);
        m_tabs->addTab(overview, tr("Сводка"));

        m_bomsTable = createTable({tr("BOM"), tr("Позиций"), tr("Штук"), tr("Длина, м"),
                                   tr("Стоимость"), tr("Цена за метр"), tr("Доля, %")});
        m_tabs->addTab(m_bomsTable, tr("По BOM"));

        const QStringList profileHeaders = {
            tr("Профиль"),  tr("Позиций"),   tr("В BOM"),     tr("Штук"),
            tr("Длина, м"), tr("Стоимость"), tr("Доля, %"),   tr("Ср. цена метра"),
            tr("Мин."),     tr("Макс."),     tr("Разброс, %")};
        const QString averageToolTip =
            tr("Средневзвешенная по длине: суммарная стоимость / суммарная длина");

        m_topTable = createTable(profileHeaders);
        m_topTable->horizontalHeaderItem(7)->setToolTip(averageToolTip);
        m_topTable->setToolTip(tr(
            "Профили с наибольшей суммарной стоимостью (Цена × Количество) по всем BOM"));
        m_tabs->addTab(m_topTable,
                       tr("Топ-%1 профиля").arg(core::BomAnalyzer::kTopProfilesCount));

        m_profilesTable = createTable(profileHeaders);
        m_profilesTable->horizontalHeaderItem(7)->setToolTip(averageToolTip);
        m_tabs->addTab(m_profilesTable, tr("Профили"));

        m_deviationsTable =
            createTable({tr("BOM"), tr("Профиль"), tr("Цена метра"),
                         tr("Средняя по профилю"), tr("Отклонение, %"), tr("Переплата")});
        m_deviationsTable->setToolTip(
            tr("Позиции, цена метра которых отличается от средней по профилю более чем "
               "на %1 %")
                .arg(core::BomAnalyzer::kPriceDeviationThresholdPercent));
        m_tabs->addTab(m_deviationsTable, tr("Отклонения цен"));

        clear();
    }

    void SummaryPanel::clear() {
        fillForm(m_overview, overviewFigures({}, true));
        for (QTableWidget* t :
             {m_bomsTable, m_topTable, m_profilesTable, m_deviationsTable}) {
            t->clearContents();
            t->setRowCount(0);
        }
    }

    void SummaryPanel::setReport(const core::AnalyticsReport& report) {
        fillForm(m_overview, overviewFigures(report.summary, false));
        fillTable(m_bomsTable, report.boms, bomRow);
        fillTable(m_topTable, report.topProfiles, profileRow);
        fillTable(m_profilesTable, report.profiles, profileRow);
        fillTable(m_deviationsTable, report.priceDeviations, deviationRow);
    }

}  // namespace bom::app
