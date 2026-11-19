#pragma once

#include <QList>
#include <QString>

#include "bom/core/domain/Bom.h"

namespace bom::infrastructure {
    enum class IssueSeverity {
        Warning,
        Error,
    };

    struct ParseIssue {
        int line = 0;
        IssueSeverity severity = IssueSeverity::Error;
        QString message;
    };

    struct CsvLoadResult {
        core::BomDataset dataset;
        QList<ParseIssue> issues;
        int dataRowCount = 0;
        int acceptedRowCount = 0;
        QString fatalError;

        bool succeeded() const noexcept { return fatalError.isEmpty(); }
        int rejectedRowCount() const noexcept { return dataRowCount - acceptedRowCount; }
        int errorCount() const noexcept;
        int warningCount() const noexcept;
    };

    class CsvBomReader {
    public:
        CsvLoadResult readFile(const QString& filePath) const;
        CsvLoadResult readText(const QString& text) const;
    };
}  // namespace bom::infrastructure
