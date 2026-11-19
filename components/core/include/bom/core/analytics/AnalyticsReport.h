#pragma once

#include <QList>
#include <QString>

namespace bom::core {
    struct BomSummary {
        QString buildingId;
        int positionCount = 0;
        int totalQuantity = 0;
        double totalLengthM = 0.0;
        double totalCost = 0.0;
        double averageCostPerMeter = 0.0;
        double shareOfTotalCostPercent = 0.0;
    };

    struct ProfileSummary {
        QString profileName;
        int positionCount = 0;
        int bomCount = 0;
        int totalQuantity = 0;
        double totalLengthM = 0.0;
        double totalCost = 0.0;
        double shareOfTotalCostPercent = 0.0;
        double averageCostPerMeter = 0.0;
        double minCostPerMeter = 0.0;
        double maxCostPerMeter = 0.0;
        double priceSpreadPercent = 0.0;
    };

    struct PriceDeviation {
        QString buildingId;
        QString profileName;
        double costPerMeter = 0.0;
        double profileAverageCostPerMeter = 0.0;
        double deviationPercent = 0.0;
        double overpayment = 0.0;
    };

    struct OverallSummary {
        int bomCount = 0;
        int positionCount = 0;
        int uniqueProfileCount = 0;
        int totalQuantity = 0;
        double totalLengthM = 0.0;
        double totalCost = 0.0;
        double averageBomCost = 0.0;
        double averageCostPerMeter = 0.0;
        QString mostExpensiveBomId;
        double topProfilesSharePercent = 0.0;
    };

    struct AnalyticsReport {
        OverallSummary summary;
        QList<BomSummary> boms;
        QList<ProfileSummary> topProfiles;
        QList<ProfileSummary> profiles;
        QList<PriceDeviation> priceDeviations;
    };
}  // namespace bom::core
