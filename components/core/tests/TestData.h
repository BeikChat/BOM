#pragma once

#include "bom/core/domain/Bom.h"

namespace bom::core::test {

    inline BomItem item(const char* profile, double lengthMm, double cost, int quantity) {
        return BomItem{QString::fromLatin1(profile), lengthMm, cost, quantity};
    }

    /// The example from the task statement.
    inline BomDataset specExample() {
        BomDataset ds;
        ds.boms.append(Bom{QStringLiteral("B0001"),
                           {item("IPE200", 6000, 300, 2), item("IPE240", 8000, 480, 1)}});
        ds.boms.append(Bom{QStringLiteral("B0002"), {item("HEA160", 4000, 160, 3)}});
        return ds;
    }

    /// IPE200 is bought at two different prices per metre (50 and 60).
    inline BomDataset twoPrices() {
        BomDataset ds;
        ds.boms.append(Bom{QStringLiteral("B0001"),
                           {item("IPE200", 6000, 300, 2), item("IPE240", 8000, 480, 1)}});
        ds.boms.append(
            Bom{QStringLiteral("B0002"),
                {item("HEA160", 4000, 160, 3), item("IPE200", 12000, 720, 1)}});
        return ds;
    }

}  // namespace bom::core::test
