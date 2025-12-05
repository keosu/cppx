// cppx.string_utils - String manipulation utilities
// Provides split, join, trim, case conversion, and regex utilities

export module cppx.string_utils;

import std;
import cppx.error;

namespace cppx {

// ============================================================================
// String splitting and joining
// ============================================================================

// Split string by delimiter
export std::vector<std::string> split(std::string_view str, std::string_view delimiter) {
    std::vector<std::string> result;
    
    if (str.empty()) {
        return result;
    }
    
    if (delimiter.empty()) {
        result.emplace_back(str);
        return result;
    }
    
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string_view::npos) {
        result.emplace_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    
    result.emplace_back(str.substr(start));
    return result;
}

// Split string by any character in delimiters
export std::vector<std::string> split_any(std::string_view str, std::string_view delimiters) {
    std::vector<std::string> result;
    
    if (str.empty()) {
        return result;
    }
    
    size_t start = 0;
    size_t end = str.find_first_of(delimiters);
    
    while (end != std::string_view::npos) {
        if (end != start) { // Skip empty tokens
            result.emplace_back(str.substr(start, end - start));
        }
        start = end + 1;
        end = str.find_first_of(delimiters, start);
    }
    
    if (start < str.length()) {
        result.emplace_back(str.substr(start));
    }
    
    return result;
}

// Split string into lines
export std::vector<std::string> split_lines(std::string_view str) {
    std::vector<std::string> result;
    std::string line;
    
    for (char ch : str) {
        if (ch == '\n') {
            result.push_back(std::move(line));
            line.clear();
        } else if (ch != '\r') {
            line += ch;
        }
    }
    
    if (!line.empty() || (!str.empty() && str.back() == '\n')) {
        result.push_back(std::move(line));
    }
    
    return result;
}

// Join strings with delimiter
export template<typename Container>
std::string join(const Container& strings, std::string_view delimiter) {
    if (strings.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    auto it = strings.begin();
    oss << *it;
    ++it;
    
    for (; it != strings.end(); ++it) {
        oss << delimiter << *it;
    }
    
    return oss.str();
}

// ============================================================================
// Trimming whitespace
// ============================================================================

// Trim left (beginning) whitespace
export std::string trim_left(std::string_view str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    return std::string(start, str.end());
}

// Trim right (ending) whitespace
export std::string trim_right(std::string_view str) {
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    return std::string(str.begin(), end);
}

// Trim both sides
export std::string trim(std::string_view str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    
    if (start == str.end()) {
        return "";
    }
    
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    
    return std::string(start, end);
}

// Trim specific characters
export std::string trim_chars(std::string_view str, std::string_view chars) {
    auto start = std::find_if_not(str.begin(), str.end(), [&chars](char ch) {
        return chars.find(ch) != std::string_view::npos;
    });
    
    if (start == str.end()) {
        return "";
    }
    
    auto end = std::find_if_not(str.rbegin(), str.rend(), [&chars](char ch) {
        return chars.find(ch) != std::string_view::npos;
    }).base();
    
    return std::string(start, end);
}

// ============================================================================
// Case conversion
// ============================================================================

// Convert to uppercase
export std::string to_upper(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    
    for (char ch : str) {
        result += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    
    return result;
}

// Convert to lowercase
export std::string to_lower(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    
    for (char ch : str) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    
    return result;
}

// Capitalize first letter
export std::string capitalize(std::string_view str) {
    if (str.empty()) {
        return "";
    }
    
    std::string result(str);
    result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    
    for (size_t i = 1; i < result.size(); ++i) {
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    }
    
    return result;
}

// ============================================================================
// String predicates
// ============================================================================

// Check if string starts with prefix
export bool starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() && 
           str.substr(0, prefix.size()) == prefix;
}

// Check if string ends with suffix
export bool ends_with(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() && 
           str.substr(str.size() - suffix.size()) == suffix;
}

// Check if string contains substring
export bool contains(std::string_view str, std::string_view substr) {
    return str.find(substr) != std::string_view::npos;
}

// Case-insensitive comparison
export bool equals_ignore_case(std::string_view str1, std::string_view str2) {
    if (str1.size() != str2.size()) {
        return false;
    }
    
    return std::equal(str1.begin(), str1.end(), str2.begin(),
        [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) == 
                   std::tolower(static_cast<unsigned char>(b));
        });
}

// ============================================================================
// String replacement
// ============================================================================

// Replace first occurrence
export std::string replace_first(std::string_view str, std::string_view from, std::string_view to) {
    std::string result(str);
    
    if (from.empty()) {
        return result;
    }
    
    size_t pos = result.find(from);
    if (pos != std::string::npos) {
        result.replace(pos, from.length(), to);
    }
    
    return result;
}

// Replace all occurrences
export std::string replace_all(std::string_view str, std::string_view from, std::string_view to) {
    std::string result(str);
    
    if (from.empty()) {
        return result;
    }
    
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    
    return result;
}

// ============================================================================
// Padding
// ============================================================================

// Pad left to width
export std::string pad_left(std::string_view str, size_t width, char fill = ' ') {
    if (str.size() >= width) {
        return std::string(str);
    }
    
    std::string result(width - str.size(), fill);
    result.append(str);
    return result;
}

// Pad right to width
export std::string pad_right(std::string_view str, size_t width, char fill = ' ') {
    if (str.size() >= width) {
        return std::string(str);
    }
    
    std::string result(str);
    result.append(width - str.size(), fill);
    return result;
}

// Center string in width
export std::string center(std::string_view str, size_t width, char fill = ' ') {
    if (str.size() >= width) {
        return std::string(str);
    }
    
    size_t total_padding = width - str.size();
    size_t left_padding = total_padding / 2;
    size_t right_padding = total_padding - left_padding;
    
    std::string result(left_padding, fill);
    result.append(str);
    result.append(right_padding, fill);
    return result;
}

// ============================================================================
// Regex utilities
// ============================================================================

// Check if string matches regex
export bool regex_match(std::string_view str, const std::string& pattern) {
    try {
        std::regex re(pattern);
        return std::regex_match(str.begin(), str.end(), re);
    } catch (const std::regex_error&) {
        return false;
    }
}

// Search for regex in string
export bool regex_search(std::string_view str, const std::string& pattern) {
    try {
        std::regex re(pattern);
        return std::regex_search(str.begin(), str.end(), re);
    } catch (const std::regex_error&) {
        return false;
    }
}

// Replace using regex
export expected<std::string, error_info> regex_replace(
    std::string_view str,
    const std::string& pattern,
    std::string_view replacement
) {
    try {
        std::regex re(pattern);
        std::string result = std::regex_replace(
            std::string(str),
            re,
            std::string(replacement)
        );
        return expected<std::string, error_info>::ok(std::move(result));
    } catch (const std::regex_error& e) {
        return expected<std::string, error_info>::err(
            error_info(std::string("Regex error: ") + e.what())
        );
    }
}

// Find all regex matches
export expected<std::vector<std::string>, error_info> regex_find_all(
    std::string_view str,
    const std::string& pattern
) {
    try {
        std::regex re(pattern);
        std::vector<std::string> matches;
        
        // Convert string_view to string for regex_iterator compatibility
        std::string temp_str(str);
        auto begin = std::sregex_iterator(temp_str.begin(), temp_str.end(), re);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            matches.push_back(it->str());
        }
        
        return expected<std::vector<std::string>, error_info>::ok(std::move(matches));
    } catch (const std::regex_error& e) {
        return expected<std::vector<std::string>, error_info>::err(
            error_info(std::string("Regex error: ") + e.what())
        );
    }
}

// ============================================================================
// Numeric conversion
// ============================================================================

// Parse integer with error handling
export expected<int, error_info> parse_int(std::string_view str) {
    try {
        std::string s(str);
        size_t pos;
        int value = std::stoi(s, &pos);
        
        if (pos != s.length()) {
            return expected<int, error_info>::err(
                error_info("Invalid integer format: not all characters consumed")
            );
        }
        
        return expected<int, error_info>::ok(value);
    } catch (const std::exception& e) {
        return expected<int, error_info>::err(
            error_info(std::string("Failed to parse integer: ") + e.what())
        );
    }
}

// Parse long with error handling
export expected<long, error_info> parse_long(std::string_view str) {
    try {
        std::string s(str);
        size_t pos;
        long value = std::stol(s, &pos);
        
        if (pos != s.length()) {
            return expected<long, error_info>::err(
                error_info("Invalid long format: not all characters consumed")
            );
        }
        
        return expected<long, error_info>::ok(value);
    } catch (const std::exception& e) {
        return expected<long, error_info>::err(
            error_info(std::string("Failed to parse long: ") + e.what())
        );
    }
}

// Parse double with error handling
export expected<double, error_info> parse_double(std::string_view str) {
    try {
        std::string s(str);
        size_t pos;
        double value = std::stod(s, &pos);
        
        if (pos != s.length()) {
            return expected<double, error_info>::err(
                error_info("Invalid double format: not all characters consumed")
            );
        }
        
        return expected<double, error_info>::ok(value);
    } catch (const std::exception& e) {
        return expected<double, error_info>::err(
            error_info(std::string("Failed to parse double: ") + e.what())
        );
    }
}

} // namespace cppx

