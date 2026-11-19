#pragma once

#include <QString>

namespace bom::core {
    inline constexpr double kMmPerMeter = 1000.0;

    struct BomItem {
        QString profileName;
        double lengthMm = 0.0;
        double cost = 0.0;
        int quantity = 0;

        static bool isValidLength(double lengthMm) noexcept;
        static bool isValidCost(double cost) noexcept;
        static bool isValidQuantity(int quantity) noexcept;

        bool isValid() const noexcept;

        double totalCost() const noexcept;
        double costPerMeter() const noexcept;
        double totalLengthMm() const noexcept;
        double totalLengthM() const noexcept { return totalLengthMm() / kMmPerMeter; }
    };
}  // namespace bom::core
