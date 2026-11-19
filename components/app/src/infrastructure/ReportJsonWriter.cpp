#include "infrastructure/ReportJsonWriter.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <cmath>

namespace bom::infrastructure {
    namespace {
        using namespace bom::core;

        inline double round(double value, int decimals) {
            const double factor = std::pow(10.0, decimals);
            return std::round(value * factor) / factor;
        }

        inline double money(double v) {
            return round(v, 2);
        }
        inline double rate(double v) {
            return round(v, 4);
        }
        inline double meters(double v) {
            return round(v, 3);
        }
        inline double percent(double v) {
            return round(v, 2);
        }

        inline QJsonObject toJsonObject(const OverallSummary& s) {
            return QJsonObject{
                {"bomCount", s.bomCount},
                {"positionCount", s.positionCount},
                {"uniqueProfileCount", s.uniqueProfileCount},
                {"totalQuantity", s.totalQuantity},
                {"totalLengthM", meters(s.totalLengthM)},
                {"totalCost", money(s.totalCost)},
                {"averageBomCost", money(s.averageBomCost)},
                {"averageCostPerMeter", rate(s.averageCostPerMeter)},
                {"mostExpensiveBom", s.mostExpensiveBomId},
                {"topProfilesSharePercent", percent(s.topProfilesSharePercent)},
            };
        }

        inline QJsonObject toJsonObject(const BomSummary& b) {
            return QJsonObject{
                {"buildingId", b.buildingId},
                {"positionCount", b.positionCount},
                {"totalQuantity", b.totalQuantity},
                {"totalLengthM", meters(b.totalLengthM)},
                {"totalCost", money(b.totalCost)},
                {"averageCostPerMeter", rate(b.averageCostPerMeter)},
                {"shareOfTotalCostPercent", percent(b.shareOfTotalCostPercent)},
            };
        }

        inline QJsonObject toJsonObject(const ProfileSummary& p) {
            return QJsonObject{
                {"profileName", p.profileName},
                {"positionCount", p.positionCount},
                {"bomCount", p.bomCount},
                {"totalQuantity", p.totalQuantity},
                {"totalLengthM", meters(p.totalLengthM)},
                {"totalCost", money(p.totalCost)},
                {"shareOfTotalCostPercent", percent(p.shareOfTotalCostPercent)},
                {"averageCostPerMeter", rate(p.averageCostPerMeter)},
                {"minCostPerMeter", rate(p.minCostPerMeter)},
                {"maxCostPerMeter", rate(p.maxCostPerMeter)},
                {"priceSpreadPercent", percent(p.priceSpreadPercent)},
            };
        }

        inline QJsonObject toJsonObject(const PriceDeviation& d) {
            return QJsonObject{
                {"buildingId", d.buildingId},
                {"profileName", d.profileName},
                {"costPerMeter", rate(d.costPerMeter)},
                {"profileAverageCostPerMeter", rate(d.profileAverageCostPerMeter)},
                {"deviationPercent", percent(d.deviationPercent)},
                {"overpayment", money(d.overpayment)},
            };
        }

        template <typename T>
        inline QJsonArray toJsonArray(const QList<T>& items) {
            QJsonArray array;
            for (const T& item : items)
                array.append(toJsonObject(item));
            return array;
        }
    }  // namespace

    QJsonObject ReportJsonWriter::toJson(const core::AnalyticsReport& report,
                                         const ReportMetadata& metadata) {
        const QDateTime generatedAt = metadata.generatedAt.isValid()
                                          ? metadata.generatedAt
                                          : QDateTime::currentDateTimeUtc();

        return QJsonObject{
            {"generatedAt", generatedAt.toUTC().toString(Qt::ISODate)},
            {"generator", metadata.generator},
            {"sourceFile", metadata.sourceFile},
            {"editedAfterLoad", metadata.editedAfterLoad},
            {"summary", toJsonObject(report.summary)},
            {"boms", toJsonArray(report.boms)},
            {"topProfiles", toJsonArray(report.topProfiles)},
            {"profiles", toJsonArray(report.profiles)},
            {"priceDeviations", toJsonArray(report.priceDeviations)},
        };
    }

    bool ReportJsonWriter::writeFile(const QString& filePath,
                                     const core::AnalyticsReport& report,
                                     const ReportMetadata& metadata,
                                     QString* errorMessage) {
        auto fail = [&](const QString& message) {
            if (errorMessage)
                *errorMessage = message;
            return false;
        };

        QSaveFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
            return fail(
                QCoreApplication::translate("ReportJsonWriter",
                                            "Не удалось открыть файл «%1» для записи: %2")
                    .arg(filePath, file.errorString()));

        const QByteArray bytes =
            QJsonDocument(toJson(report, metadata)).toJson(QJsonDocument::Indented);
        if (file.write(bytes) != bytes.size() || !file.commit())
            return fail(QCoreApplication::translate("ReportJsonWriter",
                                                    "Не удалось сохранить файл «%1»: %2")
                            .arg(filePath, file.errorString()));
        return true;
    }
}  // namespace bom::infrastructure
