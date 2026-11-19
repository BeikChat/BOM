#pragma once

#include <QTableWidget>

#include "infrastructure/CsvBomReader.h"

namespace bom::app {
    class LoadIssuesView : public QTableWidget {
        Q_OBJECT

    public:
        explicit LoadIssuesView(QWidget* parent = nullptr);

        void setIssues(const QList<infrastructure::ParseIssue>& issues);
    };
}  // namespace bom::app
