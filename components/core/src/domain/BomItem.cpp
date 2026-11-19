#include "bom/core/domain/BomItem.h"

#include <cmath>

namespace bom::core {
    bool BomItem::isValidLength(double lengthMm) noexcept {
        return std::isfinite(lengthMm) && lengthMm > 0.0;
    }

    bool BomItem::isValidCost(double cost) noexcept {
        return std::isfinite(cost) && cost >= 0.0;
    }

    bool BomItem::isValidQuantity(int quantity) noexcept {
        return quantity > 0;
    }

    bool BomItem::isValid() const noexcept {
        return !profileName.trimmed().isEmpty() && isValidLength(lengthMm) &&
               isValidCost(cost) && isValidQuantity(quantity);
    }

    double BomItem::totalCost() const noexcept {
        return cost * quantity;
    }

    double BomItem::costPerMeter() const noexcept {
        return lengthMm > 0.0 ? cost / lengthMm * kMmPerMeter : 0.0;
    }

    double BomItem::totalLengthMm() const noexcept {
        return lengthMm * quantity;
    }
}  // namespace bom::core
