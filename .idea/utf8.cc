#include <utf8.h>
#include <string>
#include <string_view>
#include <vector>
#include <cctype>

// Проверява дали code point е сепаратор
bool is_separator(uint32_t cp) {
    // ASCII whitespace / control
    if (cp <= 0x20) return true;

    // Standard ASCII punctuation
    if (cp == '.' || cp == ',' || cp == '!' || cp == '?' ||
        cp == ';' || cp == ':' || cp == '"' || cp == '\'' ||
        cp == '(' || cp == ')' || cp == '[' || cp == ']' ||
        cp == '{' || cp == '}' || cp == '-' || cp == '/') return true;

    // Unicode punctuation ranges (опростено)
    if ((cp >= 0x2000 && cp <= 0x206F) ||   // General Punctuation
        (cp >= 0x3000 && cp <= 0x303F) ||   // CJK Symbols and Punctuation
        (cp >= 0xFE30 && cp <= 0xFE4F))     // CJK Compatibility Forms
        return true;

    return false;
}

// Разбива UTF-8 string_view на "думи"
std::vector<std::string> split_words(std::string_view text) {
    std::vector<std::string> words;
    auto it = text.begin();
    auto end = text.end();

    std::string current;

    while (it != end) {
        uint32_t cp = utf8::next(it, end);

        if (is_separator(cp)) {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
        } else {
            utf8::append(cp, std::back_inserter(current));
        }
    }

    if (!current.empty())
        words.push_back(current);

    return words;
}

