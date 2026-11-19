#include "bom/core/domain/Bom.h"

#include <algorithm>

namespace bom::core {
    namespace {
        template <typename Range, typename Value>
        inline auto sum(const Range& range, Value value) noexcept {
            decltype(value(*range.cbegin())) total{};
            for (const auto& element : range)
                total += value(element);
            return total;
        }
    }  // namespace

    // ---- Bom ---------------------------------------------------------------

    int Bom::totalQuantity() const noexcept {
        return sum(items, [](const BomItem& i) { return i.quantity; });
    }

    double Bom::totalCost() const noexcept {
        return sum(items, [](const BomItem& i) { return i.totalCost(); });
    }

    double Bom::totalLengthMm() const noexcept {
        return sum(items, [](const BomItem& i) { return i.totalLengthMm(); });
    }

    double Bom::averageCostPerMeter() const noexcept {
        const double lengthMm = totalLengthMm();
        return lengthMm > 0.0 ? totalCost() / lengthMm * kMmPerMeter : 0.0;
    }

    // ---- BomDataset --------------------------------------------------------

    void BomDataset::addItem(const QString& buildingId, const BomItem& item) {
        auto it = std::find_if(boms.begin(), boms.end(), [&](const Bom& bom) {
            return bom.buildingId == buildingId;
        });

        if (it == boms.end())
            it = boms.insert(boms.end(), Bom{buildingId, {}});

        it->items.append(item);
    }

}  // namespace bom::core
