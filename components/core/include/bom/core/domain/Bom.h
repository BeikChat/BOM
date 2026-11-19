#pragma once

#include <QList>
#include <QString>

#include "bom/core/domain/BomItem.h"

namespace bom::core {
    struct Bom {
        QString buildingId;
        QList<BomItem> items;

        int positionCount() const noexcept { return static_cast<int>(items.size()); }
        int totalQuantity() const noexcept;
        double totalCost() const noexcept;
        double totalLengthMm() const noexcept;
        double totalLengthM() const noexcept { return totalLengthMm() / kMmPerMeter; }
        double averageCostPerMeter() const noexcept;
    };

    struct BomDataset {
        QList<Bom> boms;

        void addItem(const QString& buildingId, const BomItem& item);
        bool isEmpty() const noexcept { return boms.isEmpty(); }
    };
}  // namespace bom::core
