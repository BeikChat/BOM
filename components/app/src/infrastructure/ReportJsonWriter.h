#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

#include "bom/core/analytics/AnalyticsReport.h"

namespace bom::infrastructure {
    struct ReportMetadata {
        QString sourceFile;
        QDateTime generatedAt;
        QString generator;
        bool editedAfterLoad = false;
    };

    class ReportJsonWriter {
    public:
        static QJsonObject toJson(const core::AnalyticsReport& report,
                                  const ReportMetadata& metadata);

        static bool writeFile(const QString& filePath, const core::AnalyticsReport& report,
                              const ReportMetadata& metadata,
                              QString* errorMessage = nullptr);
    };
}  // namespace bom::infrastructure
