#include "WeightCalculator.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string_view>
#include <utility>

namespace profile_analyzer {
    namespace {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kMaxDimensionMm = 5000.0;
        constexpr double kEpsilon = 1e-9;
        constexpr double kSteelDensity = 7850.0;

        struct TableRow {
            double heightMm;
            double kgPerMeter;
        };

        constexpr TableRow kIpe[] = {
            {80, 6.0},   {100, 8.1},   {120, 10.4},  {140, 12.9}, {160, 15.8},
            {180, 18.8}, {200, 22.4},  {220, 26.2},  {240, 30.7}, {270, 36.1},
            {300, 42.2}, {330, 49.1},  {360, 57.1},  {400, 66.3}, {450, 77.6},
            {500, 90.7}, {550, 106.0}, {600, 122.0},
        };
        constexpr TableRow kHea[] = {
            {100, 16.7},  {120, 19.9},  {140, 24.7},  {160, 30.4},  {180, 35.5},
            {200, 42.3},  {220, 50.5},  {240, 60.3},  {260, 68.2},  {280, 76.4},
            {300, 88.3},  {320, 97.6},  {340, 104.8}, {360, 112.1}, {400, 124.8},
            {450, 139.8}, {500, 155.1}, {550, 166.2}, {600, 177.8},
        };
        constexpr TableRow kHeb[] = {
            {100, 20.4},  {120, 26.7},  {140, 33.7},  {160, 42.6},  {180, 51.2},
            {200, 61.3},  {220, 71.5},  {240, 83.2},  {260, 93.0},  {280, 103.1},
            {300, 117.0}, {320, 126.7}, {340, 134.2}, {360, 141.8}, {400, 155.3},
            {450, 171.1}, {500, 187.3}, {550, 199.4}, {600, 211.9},
        };
        constexpr TableRow kUpn[] = {
            {80, 8.6},   {100, 10.6}, {120, 13.4}, {140, 16.0}, {160, 18.8}, {180, 22.0},
            {200, 25.3}, {220, 29.4}, {240, 33.2}, {260, 37.9}, {280, 41.8}, {300, 46.2},
            {320, 59.5}, {350, 60.6}, {380, 63.1}, {400, 71.8},
        };

        struct TableFamily {
            std::wstring_view type;
            const TableRow* begin;
            const TableRow* end;
        };

        constexpr TableFamily kTableFamilies[] = {
            {L"IPE", std::begin(kIpe), std::end(kIpe)},
            {L"HEA", std::begin(kHea), std::end(kHea)},
            {L"HEB", std::begin(kHeb), std::end(kHeb)},
            {L"UPN", std::begin(kUpn), std::end(kUpn)},
        };

        double areaToKgPerMeter(double areaMm2) noexcept {
            return areaMm2 * 1e-6 * kSteelDensity;
        }

        WeightResult fromTable(const TableFamily& family,
                               const std::vector<double>& dims) {
            if (dims.size() != 1)
                return std::unexpected(PA_E_INVALID_FORMAT);

            const double h = dims.front();
            if (h < family.begin->heightMm - kEpsilon ||
                h > (family.end - 1)->heightMm + kEpsilon)
                return std::unexpected(PA_E_OUT_OF_RANGE);

            const auto upper = std::lower_bound(
                family.begin, family.end, h - kEpsilon,
                [](const TableRow& row, double v) { return row.heightMm < v; });

            if (std::abs(upper->heightMm - h) < kEpsilon)
                return upper->kgPerMeter;

            const TableRow& lo = *(upper - 1);
            const TableRow& hi = *upper;
            const double t = (h - lo.heightMm) / (hi.heightMm - lo.heightMm);
            return lo.kgPerMeter + t * (hi.kgPerMeter - lo.kgPerMeter);
        }

        WeightResult calculated(double areaMm2) {
            return areaToKgPerMeter(areaMm2);
        }

        WeightResult angle(const std::vector<double>& d) {
            if (d.size() != 2 && d.size() != 3)
                return PA_E_INVALID_FORMAT;

            const double a = d[0];
            const double b = d.size() == 3 ? d[1] : d[0];
            const double t = d.back();
            if (t >= std::min(a, b))
                return std::unexpected(PA_E_OUT_OF_RANGE);

            return calculated(t * (a + b - t));
        }

        WeightResult hollowRectangular(const std::vector<double>& d, bool square) {
            double h = 0, b = 0, t = 0;
            if (square && d.size() == 2) {
                h = b = d[0];
                t = d[1];
            } else if (d.size() == 3) {
                h = d[0];
                b = d[1];
                t = d[2];
                if (square && std::abs(h - b) > kEpsilon)
                    return std::unexpected(PA_E_INVALID_FORMAT);
            } else {
                return std::unexpected(PA_E_INVALID_FORMAT);
            }
            if (2 * t >= std::min(h, b))
                return std::unexpected(PA_E_OUT_OF_RANGE);

            return calculated(2 * t * (h + b - 2 * t));
        }

        WeightResult hollowCircular(const std::vector<double>& d) {
            if (d.size() != 2)
                return PA_E_INVALID_FORMAT;

            const double diameter = d[0];
            const double t = d[1];
            if (2 * t > diameter)
                return std::unexpected(PA_E_OUT_OF_RANGE);

            return calculated(kPi * t * (diameter - t));
        }

        WeightResult flat(const std::vector<double>& d) {
            if (d.size() != 2)
                return std::unexpected(PA_E_INVALID_FORMAT);

            return calculated(d[0] * d[1]);
        }

        WeightResult roundBar(const std::vector<double>& d) {
            if (d.size() != 1)
                return std::unexpected(PA_E_INVALID_FORMAT);

            return calculated(kPi * d[0] * d[0] / 4.0);
        }

    }  // namespace

    double nominalHeightOf(const ProfileDesignation& designation) noexcept {
        return designation.dimensions.empty() ? 0.0 : designation.dimensions.front();
    }

    WeightResult calculateWeightPerMeter(const ProfileDesignation& designation) {
        const auto& dims = designation.dimensions;
        for (double v : dims) {
            if (v <= 0.0 || v > kMaxDimensionMm)
                return std::unexpected(PA_E_OUT_OF_RANGE);
        }

        const std::wstring_view type = designation.type;
        for (const auto& family : kTableFamilies) {
            if (family.type == type)
                return fromTable(family, dims);
        }

        if (type == L"L")
            return angle(dims);
        if (type == L"RHS")
            return hollowRectangular(dims, false);
        if (type == L"SHS")
            return hollowRectangular(dims, true);
        if (type == L"CHS")
            return hollowCircular(dims);
        if (type == L"FL" || type == L"PL")
            return flat(dims);
        if (type == L"RD")
            return roundBar(dims);

        return std::unexpected(PA_E_UNKNOWN_TYPE);
    }

}  // namespace profile_analyzer
