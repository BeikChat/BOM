#include "bom/core/analytics/BomAnalyzer.h"

#include <QHash>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <limits>

namespace bom::core {
    namespace {
        inline double partOf(double part, double total) {
            return total > 0.0 ? part / total : 0.0;
        }

        inline double percentOf(double part, double total) {
            return total > 0.0 ? part / total * 100.0 : 0.0;
        }

        inline QList<ProfileSummary> summarizeProfiles(const BomDataset& dataset,
                                                double grandTotal) {
            QList<ProfileSummary> profiles{};
            QList<QSet<QString>> buildings{};
            QHash<QString, qsizetype> indexByName{};

            for (const Bom& bom : dataset.boms) {
                for (const BomItem& item : bom.items) {
                    qsizetype index = indexByName.value(item.profileName, -1);
                    if (index < 0) {
                        index = profiles.size();
                        indexByName.insert(item.profileName, index);

                        ProfileSummary s;
                        s.profileName = item.profileName;
                        s.minCostPerMeter = std::numeric_limits<double>::max();

                        profiles.append(s);
                        buildings.append(QSet<QString>{});
                    }

                    const double cpm = item.costPerMeter();
                    ProfileSummary& s = profiles[index];
                    s.positionCount += 1;
                    s.totalQuantity += item.quantity;
                    s.totalLengthM += item.totalLengthM();
                    s.totalCost += item.totalCost();
                    s.minCostPerMeter = std::min(s.minCostPerMeter, cpm);
                    s.maxCostPerMeter = std::max(s.maxCostPerMeter, cpm);

                    buildings[index].insert(bom.buildingId);
                }
            }

            for (qsizetype i = 0; i < profiles.size(); ++i) {
                ProfileSummary& s = profiles[i];
                s.bomCount = static_cast<int>(buildings[i].size());
                s.shareOfTotalCostPercent = percentOf(s.totalCost, grandTotal);
                s.averageCostPerMeter = partOf(s.totalCost, s.totalLengthM);
                s.priceSpreadPercent =
                    percentOf(s.maxCostPerMeter - s.minCostPerMeter, s.minCostPerMeter);
            }

            std::stable_sort(profiles.begin(), profiles.end(),
                             [](const ProfileSummary& a, const ProfileSummary& b) {
                                 return a.totalCost > b.totalCost;
                             });
            return profiles;
        }

        inline QList<PriceDeviation> findPriceDeviations(const BomDataset& dataset,
                                                  const QList<ProfileSummary>& profiles) {
            QHash<QString, double> averageByName;
            for (const ProfileSummary& p : profiles) {
                if (p.positionCount > 1 && p.averageCostPerMeter > 0.0)
                    averageByName.insert(p.profileName, p.averageCostPerMeter);
            }

            QList<PriceDeviation> result;
            for (const Bom& bom : dataset.boms) {
                for (const BomItem& item : bom.items) {
                    const auto average = averageByName.constFind(item.profileName);
                    if (average == averageByName.cend())
                        continue;

                    const double cpm = item.costPerMeter();
                    const double deviation = (cpm - *average) / *average * 100.0;
                    if (std::abs(deviation) <=
                        BomAnalyzer::kPriceDeviationThresholdPercent)
                        continue;

                    PriceDeviation d;
                    d.buildingId = bom.buildingId;
                    d.profileName = item.profileName;
                    d.costPerMeter = cpm;
                    d.profileAverageCostPerMeter = *average;
                    d.deviationPercent = deviation;
                    d.overpayment = (cpm - *average) * item.totalLengthM();
                    result.append(d);
                }
            }

            std::stable_sort(result.begin(), result.end(),
                             [](const PriceDeviation& a, const PriceDeviation& b) {
                                 return std::abs(a.deviationPercent) >
                                        std::abs(b.deviationPercent);
                             });
            return result;
        }

    }  // namespace

    AnalyticsReport BomAnalyzer::analyze(const BomDataset& dataset) {
        AnalyticsReport report;
        OverallSummary& s = report.summary;

        report.boms.reserve(dataset.boms.size());
        for (const Bom& bom : dataset.boms) {
            BomSummary b;
            b.buildingId = bom.buildingId;
            b.positionCount = bom.positionCount();
            b.totalQuantity = bom.totalQuantity();
            b.totalLengthM = bom.totalLengthM();
            b.totalCost = bom.totalCost();
            b.averageCostPerMeter = bom.averageCostPerMeter();
            report.boms.append(b);

            s.positionCount += b.positionCount;
            s.totalQuantity += b.totalQuantity;
            s.totalLengthM += b.totalLengthM;
            s.totalCost += b.totalCost;
        }
        for (BomSummary& b : report.boms)
            b.shareOfTotalCostPercent = percentOf(b.totalCost, s.totalCost);

        s.bomCount = static_cast<int>(report.boms.size());
        s.averageBomCost = s.bomCount > 0 ? s.totalCost / s.bomCount : 0.0;
        s.averageCostPerMeter = s.totalLengthM > 0.0 ? s.totalCost / s.totalLengthM : 0.0;

        const auto mostExpensive =
            std::max_element(report.boms.cbegin(), report.boms.cend(),
                             [](const BomSummary& a, const BomSummary& b) {
                                 return a.totalCost < b.totalCost;
                             });
        if (mostExpensive != report.boms.cend())
            s.mostExpensiveBomId = mostExpensive->buildingId;

        report.profiles = summarizeProfiles(dataset, s.totalCost);
        report.topProfiles = report.profiles.mid(0, kTopProfilesCount);
        s.uniqueProfileCount = static_cast<int>(report.profiles.size());

        double topSum = 0.0;
        for (const ProfileSummary& p : std::as_const(report.topProfiles))
            topSum += p.totalCost;

        s.topProfilesSharePercent = percentOf(topSum, s.totalCost);

        report.priceDeviations = findPriceDeviations(dataset, report.profiles);
        return report;
    }

}  // namespace bom::core
