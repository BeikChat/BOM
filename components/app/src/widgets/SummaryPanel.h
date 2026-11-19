#pragma once

#include <QWidget>

#include "bom/core/analytics/AnalyticsReport.h"

class QFormLayout;
class QTableWidget;
class QTabWidget;

namespace bom::app {
    class SummaryPanel : public QWidget {
        Q_OBJECT

    public:
        explicit SummaryPanel(QWidget* parent = nullptr);

        void setReport(const core::AnalyticsReport& report);
        void clear();

    private:
        QTabWidget* m_tabs = nullptr;
        QFormLayout* m_overview = nullptr;  // the "Сводка" tab
        QTableWidget* m_bomsTable = nullptr;
        QTableWidget* m_topTable = nullptr;
        QTableWidget* m_profilesTable = nullptr;
        QTableWidget* m_deviationsTable = nullptr;
    };
}  // namespace bom::app
