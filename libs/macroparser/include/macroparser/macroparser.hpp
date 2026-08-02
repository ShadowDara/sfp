#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>


namespace StringUtils
{
    // Entfernt Leerzeichen am Anfang und Ende
    inline std::string trim(const std::string &str)
    {
        size_t start = 0;
        size_t end = str.size();

        while (start < end &&
               std::isspace(static_cast<unsigned char>(str[start])))
            start++;

        while (end > start &&
               std::isspace(static_cast<unsigned char>(str[end - 1])))
            end--;

        return str.substr(start, end - start);
    }

    // Prüft, ob String mit anderem String beginnt
    inline bool starts_with(const std::string &str, const std::string &prefix)
    {
        return str.size() >= prefix.size() &&
               str.compare(0, prefix.size(), prefix) == 0;
    }

    // Prüft, ob String an Position pos einen String enthält
    inline bool match_at(const std::string &str, size_t pos,
                         const std::string &value)
    {
        return pos + value.size() <= str.size() &&
               str.compare(pos, value.size(), value) == 0;
    }

    // Split an einem Zeichen
    inline std::vector<std::string> split(const std::string &str,
                                          char delimiter)
    {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter))
        {
            result.push_back(token);
        }

        return result;
    }

    // Ersetzt alle Vorkommen
    inline void replace_all(std::string &str, const std::string &from,
                            const std::string &to)
    {
        if (from.empty())
            return;

        size_t pos = 0;

        while ((pos = str.find(from, pos)) != std::string::npos)
        {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    // Liest ein Wort ab Position
    inline std::string read_word(const std::string &str, size_t &pos)
    {
        while (pos < str.size() &&
               std::isspace(static_cast<unsigned char>(str[pos])))
        {
            pos++;
        }

        size_t start = pos;

        while (pos < str.size() &&
               !std::isspace(static_cast<unsigned char>(str[pos])))
        {
            pos++;
        }

        return str.substr(start, pos - start);
    }

    // Convert to int
    inline int toInt(const std::string &s)
    {
        try
        {
            return std::stoi(s);
        }
        catch (...)
        {
            return 0;
        }
    }

    // Check if Number
    inline int isNumber(const std::string &s)
    {
        try
        {
            std::stoi(s);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
} // namespace StringUtils

namespace MacroParserUtils
{

    enum class TokenType
    {
        Identifier,
        Number,
        String,
        Operator,
        Punctuation,
        Whitespace
    };

    struct Token
    {
        TokenType type;
        std::string value;
    };

    struct IfState
    {
        bool parentActive;
        bool condition;
        bool active;
    };

    struct Macro
    {
        bool functionLike = false;

        // true, wenn die Parameterliste mit "..." endet (variadisches Makro,
        // z.B. "#define LOG(fmt, ...) ..."). Zusätzliche Aufrufargumente
        // jenseits der benannten Parameter werden dann über den speziellen
        // Bezeichner __VA_ARGS__ im Body verfügbar gemacht.
        bool variadic = false;

        std::vector<std::string> parameters;

        std::string body;
    };

} // namespace MacroParserUtils

class MacroParser
{
  private:
    std::unordered_map<std::string, MacroParserUtils::Macro> macros;
    std::stack<MacroParserUtils::IfState> IfStack;
    // std::unordered_set<std::string> expanding;

    int evaluateIf(const std::string &expr);

    std::string expandText(const std::string &text,
                           std::unordered_set<std::string> &expanding);

    // Wie expandText, aber speziell für #if-Ausdrücke: das Argument
    // von "defined(NAME)" / "defined NAME" wird NICHT expandiert,
    // sonst würde z.B. "#if defined(FOO)" kaputtgehen sobald FOO
    // ein Objekt-Makro ist (FOO würde textuell durch seinen Wert
    // ersetzt, bevor "defined" überhaupt ausgewertet wird).
    std::string expandTextForIf(const std::string &text,
                                std::unordered_set<std::string> &expanding);

    std::string expandMacro(const std::string &name,
                            std::unordered_set<std::string> &expanding);

    // Versucht ab Position 'i' im Text 'text' einen Aufruf des
    // funktionsähnlichen Makros 'name' (inkl. Klammer-Argumentliste,
    // auch verschachtelt) zu parsen und zu expandieren.
    bool tryExpandFunctionCall(const std::string &text, size_t identStart,
                               size_t identEnd, const std::string &name,
                               const MacroParserUtils::Macro &macro,
                               std::unordered_set<std::string> &expanding,
                               std::string &out, size_t &i);

    // Setzt in 'body' Parameter ein und wertet dabei die Operatoren
    // '#' (stringize) und '##' (token pasting) korrekt aus:
    //  - "#param"   -> quotierte, whitespace-normalisierte, escapte
    //                  Darstellung des UNEXPANDIERTEN Roharguments
    //  - "a##b"     -> textuelle Verklebung der UNEXPANDIERTEN
    //                  Rohtexte von a und b zu einem neuen Token
    //  - sonst      -> normale Ersetzung durch den bereits
    //                  makro-expandierten Argumentwert
    // 'params' enthält die Parameternamen (inkl. ggf. "__VA_ARGS__"
    // als letztem Eintrag bei variadischen Makros), 'rawValues' die
    // unexpandierten (nur getrimmten) Argumenttexte und
    // 'expandedValues' die bereits rekursiv makroexpandierten
    // Argumentwerte - beide parallel zu 'params' indiziert.
    std::string
    substituteParams(const std::string &body,
                     const std::vector<std::string> &params,
                     const std::vector<std::string> &rawValues,
                     const std::vector<std::string> &expandedValues);

    // Wandelt ein rohes Makroargument in die quotierte, escapte
    // String-Literal-Form um, die der '#'-Stringize-Operator
    // produziert (internes Whitespace wird auf je ein Leerzeichen
    // reduziert, führendes/folgendes Whitespace entfernt, '"' und
    // '\' werden escaped).
    std::string stringizeArg(const std::string &raw);

    bool isActive();

    bool isIdentifierChar(char c);

    // Parse these comments
    std::string parseSlashComments(const std::string &input);

    // Parse the /**/ comments
    std::string parseBlockComments(const std::string &input);

  public:
    std::vector<std::string> importPaths;

    std::string parse_macros(const std::string &input);

    void add_macro(const std::string &name,
                   const MacroParserUtils::Macro &macro);

    void remove_macro(const std::string &name);

    bool contains_macro(const std::string &name);

    MacroParserUtils::Macro get_macro(const std::string &name);
};
