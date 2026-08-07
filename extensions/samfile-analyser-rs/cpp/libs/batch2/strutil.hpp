// string_utils.hpp
// Header-only C++ Bibliothek mit String-Manipulationsfunktionen,
// wie man sie aus Python, JavaScript, Java etc. kennt.
//
// Einfach includieren:
//   #include "string_utils.hpp"
//   using namespace strutil;
//
// Alle Funktionen sind als `inline` markiert -> keine ODR-Probleme
// beim Einbinden in mehrere Übersetzungseinheiten.

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <numeric>

namespace strutil {

    // ---------------------------------------------------------------------
    // Trimming / Whitespace  (Python: strip, lstrip, rstrip)
    // ---------------------------------------------------------------------

    inline std::string lstrip(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
            ++start;
        return s.substr(start);
    }

    inline std::string rstrip(const std::string& s) {
        size_t end = s.size();
        while (end > 0 && std::isspace(static_cast<unsigned char>(s[end - 1])))
            --end;
        return s.substr(0, end);
    }

    inline std::string strip(const std::string& s) {
        return lstrip(rstrip(s));
    }

    // Variante mit eigenem Zeichensatz, der entfernt werden soll (wie Python's strip(chars))
    inline std::string strip(const std::string& s, const std::string& chars) {
        size_t start = s.find_first_not_of(chars);
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(chars);
        return s.substr(start, end - start + 1);
    }

    // ---------------------------------------------------------------------
    // Groß-/Kleinschreibung (Python: upper, lower, title, capitalize, swapcase)
    // ---------------------------------------------------------------------

    inline std::string to_upper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::toupper(c); });
        return s;
    }

    inline std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    inline std::string capitalize(std::string s) {
        if (!s.empty()) {
            s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
            for (size_t i = 1; i < s.size(); ++i)
                s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
        }
        return s;
    }

    inline std::string title(const std::string& s) {
        std::string result = s;
        bool new_word = true;
        for (auto& c : result) {
            if (std::isalpha(static_cast<unsigned char>(c))) {
                c = new_word ? std::toupper(static_cast<unsigned char>(c))
                    : std::tolower(static_cast<unsigned char>(c));
                new_word = false;
            }
            else {
                new_word = true;
            }
        }
        return result;
    }

    inline std::string swapcase(std::string s) {
        for (auto& c : s) {
            if (std::isupper(static_cast<unsigned char>(c)))
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            else if (std::islower(static_cast<unsigned char>(c)))
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return s;
    }

    // ---------------------------------------------------------------------
    // Prüfen / Suchen (Python: startswith, endswith, contains/in, count, find)
    // ---------------------------------------------------------------------

    inline bool starts_with(const std::string& s, const std::string& prefix) {
        return s.size() >= prefix.size() &&
            s.compare(0, prefix.size(), prefix) == 0;
    }

    inline bool ends_with(const std::string& s, const std::string& suffix) {
        return s.size() >= suffix.size() &&
            s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    inline bool contains(const std::string& s, const std::string& needle) {
        return s.find(needle) != std::string::npos;
    }

    inline size_t count(const std::string& s, const std::string& sub) {
        if (sub.empty()) return 0;
        size_t cnt = 0, pos = 0;
        while ((pos = s.find(sub, pos)) != std::string::npos) {
            ++cnt;
            pos += sub.size();
        }
        return cnt;
    }

    // ---------------------------------------------------------------------
    // is*-Prüfungen (Python: isdigit, isalpha, isalnum, isspace, isupper, islower)
    // ---------------------------------------------------------------------

    inline bool is_digit(const std::string& s) {
        if (s.empty()) return false;
        return std::all_of(s.begin(), s.end(),
            [](unsigned char c) { return std::isdigit(c); });
    }

    inline bool is_alpha(const std::string& s) {
        if (s.empty()) return false;
        return std::all_of(s.begin(), s.end(),
            [](unsigned char c) { return std::isalpha(c); });
    }

    inline bool is_alnum(const std::string& s) {
        if (s.empty()) return false;
        return std::all_of(s.begin(), s.end(),
            [](unsigned char c) { return std::isalnum(c); });
    }

    inline bool is_space(const std::string& s) {
        if (s.empty()) return false;
        return std::all_of(s.begin(), s.end(),
            [](unsigned char c) { return std::isspace(c); });
    }

    inline bool is_upper(const std::string& s) {
        bool has_alpha = false;
        for (unsigned char c : s) {
            if (std::isalpha(c)) {
                has_alpha = true;
                if (!std::isupper(c)) return false;
            }
        }
        return has_alpha;
    }

    inline bool is_lower(const std::string& s) {
        bool has_alpha = false;
        for (unsigned char c : s) {
            if (std::isalpha(c)) {
                has_alpha = true;
                if (!std::islower(c)) return false;
            }
        }
        return has_alpha;
    }

    // ---------------------------------------------------------------------
    // Split / Join (Python: split, rsplit, splitlines, join)
    // ---------------------------------------------------------------------

    // Split an jedem beliebigen Zeichen aus `delims` (Standard: Whitespace),
    // analog zu Python's str.split() ohne Argument (mehrfache Trenner werden zusammengefasst)
    inline std::vector<std::string> split(const std::string& s, const std::string& delims = " \t\n\r\f\v") {
        std::vector<std::string> tokens;
        size_t start = s.find_first_not_of(delims);
        while (start != std::string::npos) {
            size_t end = s.find_first_of(delims, start);
            tokens.push_back(s.substr(start, end - start));
            start = (end == std::string::npos) ? std::string::npos : s.find_first_not_of(delims, end);
        }
        return tokens;
    }

    // Split an einem festen String-Trenner (wie Python's str.split(sep))
    inline std::vector<std::string> split_by(const std::string& s, const std::string& sep) {
        if (sep.empty()) throw std::invalid_argument("split_by: separator darf nicht leer sein");
        std::vector<std::string> tokens;
        size_t start = 0, pos;
        while ((pos = s.find(sep, start)) != std::string::npos) {
            tokens.push_back(s.substr(start, pos - start));
            start = pos + sep.size();
        }
        tokens.push_back(s.substr(start));
        return tokens;
    }

    inline std::vector<std::string> splitlines(const std::string& s) {
        std::vector<std::string> lines;
        std::istringstream iss(s);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
        return lines;
    }

    inline std::string join(const std::vector<std::string>& parts, const std::string& sep) {
        std::string result;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i) result += sep;
            result += parts[i];
        }
        return result;
    }

    // ---------------------------------------------------------------------
    // Ersetzen (Python: replace)
    // ---------------------------------------------------------------------

    inline std::string replace(const std::string& s, const std::string& from,
        const std::string& to, int max_count = -1) {
        if (from.empty()) return s;
        std::string result;
        size_t start = 0, pos;
        int replaced = 0;
        while ((pos = s.find(from, start)) != std::string::npos) {
            if (max_count >= 0 && replaced >= max_count) break;
            result.append(s, start, pos - start);
            result += to;
            start = pos + from.size();
            ++replaced;
        }
        result.append(s, start, std::string::npos);
        return result;
    }

    // ---------------------------------------------------------------------
    // Padding / Ausrichtung (Python: ljust, rjust, center, zfill)
    // ---------------------------------------------------------------------

    inline std::string ljust(const std::string& s, size_t width, char fill = ' ') {
        if (s.size() >= width) return s;
        return s + std::string(width - s.size(), fill);
    }

    inline std::string rjust(const std::string& s, size_t width, char fill = ' ') {
        if (s.size() >= width) return s;
        return std::string(width - s.size(), fill) + s;
    }

    inline std::string center(const std::string& s, size_t width, char fill = ' ') {
        if (s.size() >= width) return s;
        size_t total_pad = width - s.size();
        size_t left = total_pad / 2;
        size_t right = total_pad - left;
        return std::string(left, fill) + s + std::string(right, fill);
    }

    inline std::string zfill(const std::string& s, size_t width) {
        if (s.size() >= width) return s;
        bool has_sign = !s.empty() && (s[0] == '+' || s[0] == '-');
        std::string sign = has_sign ? std::string(1, s[0]) : "";
        std::string rest = has_sign ? s.substr(1) : s;
        size_t pad = width - s.size();
        return sign + std::string(pad, '0') + rest;
    }

    // ---------------------------------------------------------------------
    // Sonstiges (Python: str * n, reversed(str), removeprefix, removesuffix)
    // ---------------------------------------------------------------------

    inline std::string repeat(const std::string& s, size_t n) {
        std::string result;
        result.reserve(s.size() * n);
        for (size_t i = 0; i < n; ++i) result += s;
        return result;
    }

    inline std::string reverse(std::string s) {
        std::reverse(s.begin(), s.end());
        return s;
    }

    // Python 3.9+: removeprefix / removesuffix
    inline std::string remove_prefix(const std::string& s, const std::string& prefix) {
        return starts_with(s, prefix) ? s.substr(prefix.size()) : s;
    }

    inline std::string remove_suffix(const std::string& s, const std::string& suffix) {
        return ends_with(s, suffix) ? s.substr(0, s.size() - suffix.size()) : s;
    }

    // Wie Java/JS Array.prototype.slice bzw. Python's s[start:end]
    inline std::string slice(const std::string& s, int start, int end) {
        int len = static_cast<int>(s.size());
        if (start < 0) start += len;
        if (end < 0) end += len;
        start = std::max(0, std::min(start, static_cast<int>(len)));
        end = std::max(0, std::min(end, static_cast<int>(len)));
        if (start >= end) return "";
        return s.substr(start, end - start);
    }

} // namespace strutil
