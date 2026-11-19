#pragma once

#include <QDialog>
#include <QHash>

#include "bom/core/analytics/AnalyticsReport.h"

class QComboBox;
class QLabel;

namespace bom::app {
    class ProfileAnalyzerLibrary;

    class ProfileAnalysisDialog : public QDialog {
        Q_OBJECT

    public:
        ProfileAnalysisDialog(ProfileAnalyzerLibrary& analyzer,
                              const QList<core::ProfileSummary>& profiles,
                              const QString& initialProfile, QWidget* parent = nullptr);

    private slots:
        void runAnalysis();

    private:
        void setResultText(const QString& type, const QString& height,
                           const QString& weight, const QString& totalWeight);

        ProfileAnalyzerLibrary& m_analyzer;
        QHash<QString, double> m_totalLengthMByName;

        QComboBox* m_profileCombo = nullptr;
        QLabel* m_type = nullptr;
        QLabel* m_height = nullptr;
        QLabel* m_weight = nullptr;
        QLabel* m_totalWeight = nullptr;
        QLabel* m_status = nullptr;
    };
}  // namespace bom::app
