// C ABI boundary of profile_analyzer.dll.
// Responsibilities: argument validation, conversion between C buffers and C++
// types, and making sure no C++ exception ever escapes into the caller
// (Delphi cannot handle MSVC C++ exceptions).

#include <cwchar>
#include <string_view>

#include "ProfileNameParser.h"
#include "WeightCalculator.h"
#include "profile_analyzer/profile_analyzer.h"

namespace {
    std::size_t boundedLength(const wchar_t* s, std::size_t maxLength) noexcept {
        std::size_t n = 0;
        while (n <= maxLength && s[n] != L'\0')
            ++n;
        return n;
    }
}  // namespace

extern "C" int PA_CALL AnalyzeProfile(const wchar_t* profileName, double* nominalHeight,
                                      double* weightPerMeter, wchar_t* profileType) {
    // Reset every output we are allowed to touch, so callers never see stale data.
    if (nominalHeight)
        *nominalHeight = 0.0;
    if (weightPerMeter)
        *weightPerMeter = 0.0;
    if (profileType)
        profileType[0] = L'\0';

    if (!profileName || !nominalHeight || !weightPerMeter || !profileType)
        return PA_E_INVALID_ARGUMENT;

    try {
        const std::size_t length = boundedLength(profileName, PA_PROFILE_NAME_MAX_LENGTH);
        if (length > PA_PROFILE_NAME_MAX_LENGTH)
            return PA_E_INVALID_FORMAT;

        using namespace profile_analyzer;

        const ParseResult parsed = parseProfileName(std::wstring_view(profileName, length));
        if (!parsed)
            return parsed.error();

        // Type and height are returned even if the weight cannot be determined.
        const ProfileDesignation& designation = parsed.value();
        if (designation.type.size() < PA_PROFILE_TYPE_BUFFER_SIZE)
            std::wmemcpy(profileType, designation.type.c_str(), designation.type.size() + 1);
        *nominalHeight = nominalHeightOf(designation);

        const WeightResult weight = calculateWeightPerMeter(designation);
        if (!weight)
            return weight.error();

        *weightPerMeter = weight.value();
        return PA_OK;
    } catch (...) {
        *nominalHeight = 0.0;
        *weightPerMeter = 0.0;
        profileType[0] = L'\0';
        return PA_E_INTERNAL;
    }
}

extern "C" const wchar_t* PA_CALL GetAnalyzeProfileErrorText(int code) {
    switch (code) {
        case PA_OK:
            return L"OK";
        case PA_E_INVALID_ARGUMENT:
            return L"Invalid argument: a required pointer is NULL";
        case PA_E_EMPTY_NAME:
            return L"Profile name is empty";
        case PA_E_INVALID_FORMAT:
            return L"Profile name has an invalid format or a wrong number of dimensions";
        case PA_E_UNKNOWN_TYPE:
            return L"Profile type is not supported";
        case PA_E_OUT_OF_RANGE:
            return L"Profile dimensions are out of the supported range";
        case PA_E_INTERNAL:
            return L"Internal error in profile_analyzer.dll";
        default:
            return L"Unknown result code";
    }
}
