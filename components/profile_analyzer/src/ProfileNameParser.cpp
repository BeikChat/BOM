#include "ProfileNameParser.h"

#include <charconv>
#include <optional>
#include <string>

#include "profile_analyzer/profile_analyzer.h"

namespace profile_analyzer {
    namespace {
        bool isAsciiLetter(wchar_t c) noexcept {
            return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z');
        }
        bool isDigit(wchar_t c) noexcept {
            return c >= L'0' && c <= L'9';
        }
        bool isSpace(wchar_t c) noexcept {
            return c == L' ' || c == L'\t';
        }
        bool isDecimalPoint(wchar_t c) noexcept {
            return c == L'.' || c == L',';
        }
        bool isDimensionSeparator(wchar_t c) noexcept {
            return c == L'*' || c == L'x' || c == L'X' || c == L'/';
        }
        bool isSpaceSeparator(wchar_t c) noexcept {
            return c == L'-' || c == L'_';
        }

        wchar_t toUpperAscii(wchar_t c) noexcept {
            return (c >= L'a' && c <= L'z') ? wchar_t(c - L'a' + L'A') : c;
        }

        std::wstring_view trim(std::wstring_view s) noexcept {
            while (!s.empty() && isSpace(s.front()))
                s.remove_prefix(1);
            while (!s.empty() && isSpace(s.back()))
                s.remove_suffix(1);
            return s;
        }

        std::optional<double> readNumber(std::wstring_view s, std::size_t& pos) {
            std::string ascii;
            bool seenDecimalPoint = false;
            while (pos < s.size()) {
                const wchar_t c = s[pos];
                if (isDigit(c)) {
                    ascii.push_back(static_cast<char>(c));
                } else if (isDecimalPoint(c) && !seenDecimalPoint) {
                    seenDecimalPoint = true;
                    ascii.push_back('.');
                } else {
                    break;
                }
                ++pos;
            }

            if (ascii.empty())
                return std::nullopt;

            double result = 0.0;
            const char* first = ascii.data();
            const char* last = ascii.data() + ascii.size();
            const auto [ptr, ec] = std::from_chars(first, last, result);

            if (ec != std::errc() || ptr != last)
                return std::nullopt;

            return result;
        }

        void skipSpaces(std::wstring_view s, std::size_t& pos) noexcept {
            while (pos < s.size() && isSpace(s[pos]))
                ++pos;
        }
    }  // namespace

    ParseResult parseProfileName(std::wstring_view name) {
        ProfileDesignation result;
        const std::wstring_view s = trim(name);
        if (s.empty())
            return std::unexpected(PA_E_EMPTY_NAME);

        std::size_t pos = 0;
        while (pos < s.size() && isAsciiLetter(s[pos])) {
            result.type.push_back(toUpperAscii(s[pos]));
            ++pos;
        }

        const std::size_t MAX_SIZE =
            static_cast<std::size_t>(PA_PROFILE_TYPE_BUFFER_SIZE);
        if (result.type.empty() || result.type.size() >= MAX_SIZE)
            return std::unexpected(PA_E_INVALID_FORMAT);

        while (pos < s.size() && (isSpace(s[pos]) || isSpaceSeparator(s[pos])))
            ++pos;

        while (true) {
            auto parsed = readNumber(s, pos);
            if (!parsed.has_value())
                return std::unexpected(PA_E_INVALID_FORMAT);

            result.dimensions.push_back(parsed.value());

            skipSpaces(s, pos);

            if (pos == s.size())
                break;

            if (!isDimensionSeparator(s[pos]))
                return std::unexpected(PA_E_INVALID_FORMAT);

            ++pos;

            skipSpaces(s, pos);
        }

        return result;
    }

}  // namespace profile_analyzer
