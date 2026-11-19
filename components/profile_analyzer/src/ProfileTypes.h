#pragma once

#include <string>
#include <vector>
#include <expected>

#include "profile_analyzer/profile_analyzer.h"

namespace profile_analyzer {
    struct ProfileDesignation {
        std::wstring type;
        std::vector<double> dimensions;
    };

    using ParseResult = std::expected<ProfileDesignation, int>;
    using WeightResult = std::expected<double, int>;
}  // namespace profile_analyzer
