#pragma once

#include <string_view>

#include "ProfileTypes.h"

namespace profile_analyzer {

    ParseResult parseProfileName(std::wstring_view name);

}  // namespace profile_analyzer
