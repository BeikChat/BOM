#pragma once

#include <QLibrary>
#include <QString>

#include <expected>

namespace bom::app {
    constexpr int kProfileAnalyzerNotLoaded = -1000;

    struct ProfileAnalysis {
        int code = 0;
        QString profileType;
        double nominalHeightMm = 0.0;
        double weightPerMeterKg = 0.0;
        QString message;
    };

    struct AnalysisError {
        int code = 0;
        QString message;
    };

    using ProfileAnalysisResult = std::expected<ProfileAnalysis, AnalysisError>;

    class ProfileAnalyzerLibrary final {
    public:
        explicit ProfileAnalyzerLibrary(QString libraryPath = defaultLibraryPath());
        ~ProfileAnalyzerLibrary();

        ProfileAnalyzerLibrary(const ProfileAnalyzerLibrary&) = delete;
        ProfileAnalyzerLibrary& operator=(const ProfileAnalyzerLibrary&) = delete;

        static QString defaultLibraryPath();

        bool isAvailable();
        QString unavailableReason() const { return m_loadError; }
        ProfileAnalysisResult analyze(const QString& profileName);

    private:
        bool ensureLoaded();

        QLibrary m_library;
        QFunctionPointer m_analyzeProfile = nullptr;
        QFunctionPointer m_errorText = nullptr;
        bool m_loadAttempted = false;
        QString m_loadError;
    };

}  // namespace bom::app
