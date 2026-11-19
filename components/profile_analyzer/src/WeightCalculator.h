#pragma once

#include "ProfileTypes.h"

namespace profile_analyzer {

    WeightResult calculateWeightPerMeter(const ProfileDesignation& designation);
    double nominalHeightOf(const ProfileDesignation& designation) noexcept;

}  // namespace profile_analyzer
