#pragma once

#include "bom/core/analytics/AnalyticsReport.h"
#include "bom/core/domain/Bom.h"

namespace bom::core {
    class BomAnalyzer {
    public:
        static constexpr int kTopProfilesCount = 3;
        static constexpr double kPriceDeviationThresholdPercent = 10.0;

        static AnalyticsReport analyze(const BomDataset& dataset);
    };
}  // namespace bom::core
