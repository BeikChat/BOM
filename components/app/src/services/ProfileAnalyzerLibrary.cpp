#include "services/ProfileAnalyzerLibrary.h"

#include <QCoreApplication>
#include <QDir>
#include <string>
#include <utility>

#include "profile_analyzer/profile_analyzer.h"

namespace bom::app {
    namespace {
        using PFN_AnalyzeProfile = decltype(::AnalyzeProfile)*;
        using PFN_GetAnalyzeProfileErrorText = decltype(::GetAnalyzeProfileErrorText)*;

        template <typename To, typename From>
        constexpr To as(From from) noexcept {
            return reinterpret_cast<To>(from);
        }

        template <typename Pfn, typename... Args>
        constexpr std::invoke_result_t<Pfn, Args...> callAs(
            QFunctionPointer pfn,
            Args... args) noexcept(std::is_nothrow_invocable_v<Pfn, Args>) {
            return reinterpret_cast<Pfn>(pfn)(args...);
        }

        int __stdcall AnalyzeProfileNoop(const wchar_t* profileName,
                                         double* nominalHeight, double* weightPerMeter,
                                         wchar_t* profileType) {
            if (nominalHeight)
                *nominalHeight = 0.0;
            if (weightPerMeter)
                *weightPerMeter = 0.0;
            if (profileType)
                profileType[0] = L'\0';

            return PA_E_INTERNAL;
        }

        const wchar_t* GetAnalyzeProfileErrorTextNoop(int code) {
            return L"Unknown";
        }
    }  // namespace

    ProfileAnalyzerLibrary::ProfileAnalyzerLibrary(QString libraryPath)
        : m_library(std::move(libraryPath)), m_analyzeProfile(), m_errorText() {
        m_analyzeProfile = as<QFunctionPointer>(AnalyzeProfileNoop);
        m_errorText = as<QFunctionPointer>(GetAnalyzeProfileErrorTextNoop);
    }

    ProfileAnalyzerLibrary::~ProfileAnalyzerLibrary() {
        m_analyzeProfile = as<QFunctionPointer>(AnalyzeProfileNoop);
        m_errorText = as<QFunctionPointer>(GetAnalyzeProfileErrorTextNoop);

        if (m_library.isLoaded())
            m_library.unload();
    }

    QString ProfileAnalyzerLibrary::defaultLibraryPath() {
        return QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("profile_analyzer.dll"));
    }

    bool ProfileAnalyzerLibrary::isAvailable() {
        return ensureLoaded();
    }

    bool ProfileAnalyzerLibrary::ensureLoaded() {
        if (m_library.isLoaded())
            return true;

        if (m_loadAttempted)
            return false;

        m_loadAttempted = true;

        if (!m_library.load()) {
            m_loadError = QCoreApplication::translate("ProfileAnalyzerLibrary",
                                                      "Не удалось загрузить %1: %2")
                              .arg(QDir::toNativeSeparators(m_library.fileName()),
                                   m_library.errorString());
            return false;
        }

        QFunctionPointer analyzeProfile = m_library.resolve("AnalyzeProfile");
        if (!analyzeProfile) {
            m_loadError =
                QCoreApplication::translate("ProfileAnalyzerLibrary",
                                            "В библиотеке %1 нет функции AnalyzeProfile")
                    .arg(QDir::toNativeSeparators(m_library.fileName()));
            m_library.unload();
            return false;
        }

        QFunctionPointer errorText = m_library.resolve("GetAnalyzeProfileErrorText");
        if (!errorText) {
            m_loadError = QCoreApplication::translate(
                              "ProfileAnalyzerLibrary",
                              "В библиотеке %1 нет функции GetAnalyzeProfileErrorText")
                              .arg(QDir::toNativeSeparators(m_library.fileName()));
            m_library.unload();
            return false;
        }

        m_analyzeProfile = analyzeProfile;
        m_errorText = errorText;

        return true;
    }

    ProfileAnalysisResult ProfileAnalyzerLibrary::analyze(const QString& profileName) {
        ProfileAnalysis result;
        if (!ensureLoaded())
            return std::unexpected(AnalysisError{kProfileAnalyzerNotLoaded, m_loadError});

        const std::wstring name = profileName.toStdWString();
        wchar_t typeBuffer[PA_PROFILE_TYPE_BUFFER_SIZE] = {};
        double height = 0.0;
        double weight = 0.0;

        int code = callAs<PFN_AnalyzeProfile>(m_analyzeProfile, name.c_str(), &height,
                                              &weight, typeBuffer);
        typeBuffer[PA_PROFILE_TYPE_BUFFER_SIZE - 1] = L'\0';

        QString message = QString::fromWCharArray(
            callAs<PFN_GetAnalyzeProfileErrorText>(m_errorText, code));
        if (code != PA_OK)
            return std::unexpected(AnalysisError{code, message});

        result.code = code;
        result.profileType = QString::fromWCharArray(typeBuffer);
        result.nominalHeightMm = height;
        result.weightPerMeterKg = weight;
        result.message = message;

        return result;
    }

}  // namespace bom::app
