#include <fstream>
#include <macroparser/macroparser.hpp>

// #define D(msg) std::cout << msg << "\n";
#define D(msg) //

using namespace MacroParserUtils;

int MacroParser::evaluateIf(const std::string &expr)
{
    // Statt einer eigenen inneren Parser-Klasse: der gesamte Zustand
    // (aktuelle Position 'pos' im Ausdruck 's') wird von einer Reihe
    // gegenseitig rekursiver Lambdas per Referenz geteilt. Die Lambdas
    // werden zuerst als std::function deklariert (damit sie einander
    // schon vor ihrer eigenen Definition aufrufen können) und danach
    // der Reihe nach mit ihrer eigentlichen Implementierung belegt.

    const std::string &s = expr;
    size_t pos = 0;

    auto skipSpaces = [&]()
    {
        while (pos < s.size() && std::isspace((unsigned char)s[pos]))
            pos++;
    };

    auto match = [&](const std::string &token) -> bool
    {
        skipSpaces();

        if (s.compare(pos, token.size(), token) == 0)
        {
            pos += token.size();
            return true;
        }

        return false;
    };

    // Wie match(), aber nur für Schlüsselwörter (z.B. "defined"):
    // stellt sicher, dass direkt danach kein weiteres Identifier-Zeichen
    // folgt (sonst würde z.B. "definedFoo" fälschlich als "defined" +
    // "Foo" erkannt).
    auto matchKeyword = [&](const std::string &token) -> bool
    {
        skipSpaces();

        if (s.compare(pos, token.size(), token) == 0)
        {
            size_t after = pos + token.size();

            if (after >= s.size() ||
                !(std::isalnum((unsigned char)s[after]) || s[after] == '_'))
            {
                pos = after;
                return true;
            }
        }

        return false;
    };

    auto parseIdentifier = [&]() -> std::string
    {
        skipSpaces();

        std::string id;

        while (pos < s.size() &&
               (std::isalnum((unsigned char)s[pos]) || s[pos] == '_'))
        {
            id += s[pos++];
        }

        return id;
    };

    auto parseNumber = [&]() -> long long
    {
        skipSpaces();

        size_t start = pos;

        while (pos < s.size() && std::isdigit((unsigned char)s[pos]))
        {
            pos++;
        }

        return std::stoll(s.substr(start, pos - start));
    };

    std::function<long long()> parseLogicalOr;
    std::function<long long()> parseLogicalAnd;
    std::function<long long()> parseEquality;
    std::function<long long()> parseRelational;
    std::function<long long()> parseAdditive;
    std::function<long long()> parseMultiply;
    std::function<long long()> parseUnary;
    std::function<long long()> parsePrimary;

    parsePrimary = [&]() -> long long
    {
        skipSpaces();

        if (match("("))
        {
            long long value = parseLogicalOr();

            if (!match(")"))
                throw std::runtime_error("Expected ')'");

            return value;
        }

        //----------------------------------
        // defined(NAME)
        //----------------------------------

        if (matchKeyword("defined"))
        {
            skipSpaces();

            if (match("("))
            {
                std::string name = parseIdentifier();

                if (!match(")"))
                    throw std::runtime_error("Expected ')' after defined");

                return macros.contains(name);
            }

            std::string name = parseIdentifier();

            return macros.contains(name);
        }

        //----------------------------------
        // Zahl
        //----------------------------------

        if (pos < s.size() && std::isdigit((unsigned char)s[pos]))
        {
            return parseNumber();
        }

        //----------------------------------
        // unbekannter Identifier -> 0
        //----------------------------------

        std::string id = parseIdentifier();

        if (!id.empty())
            return 0;

        throw std::runtime_error("Unexpected token");
    };

    parseUnary = [&]() -> long long
    {
        if (match("!"))
            return !parseUnary();

        if (match("-"))
            return -parseUnary();

        if (match("+"))
            return parseUnary();

        return parsePrimary();
    };

    parseMultiply = [&]() -> long long
    {
        long long lhs = parseUnary();

        while (true)
        {
            if (match("*"))
                lhs *= parseUnary();

            else if (match("/"))
                lhs /= parseUnary();

            else if (match("%"))
                lhs %= parseUnary();

            else
                break;
        }

        return lhs;
    };

    parseAdditive = [&]() -> long long
    {
        long long lhs = parseMultiply();

        while (true)
        {
            if (match("+"))
                lhs += parseMultiply();

            else if (match("-"))
                lhs -= parseMultiply();

            else
                break;
        }

        return lhs;
    };

    parseRelational = [&]() -> long long
    {
        long long lhs = parseAdditive();

        while (true)
        {
            if (match("<="))
                lhs = lhs <= parseAdditive();

            else if (match(">="))
                lhs = lhs >= parseAdditive();

            else if (match("<"))
                lhs = lhs < parseAdditive();

            else if (match(">"))
                lhs = lhs > parseAdditive();

            else
                break;
        }

        return lhs;
    };

    parseEquality = [&]() -> long long
    {
        long long lhs = parseRelational();

        while (true)
        {
            if (match("=="))
                lhs = lhs == parseRelational();

            else if (match("!="))
                lhs = lhs != parseRelational();

            else
                break;
        }

        return lhs;
    };

    parseLogicalAnd = [&]() -> long long
    {
        long long lhs = parseEquality();

        while (match("&&"))
        {
            long long rhs = parseEquality();
            lhs = lhs && rhs;
        }

        return lhs;
    };

    parseLogicalOr = [&]() -> long long
    {
        long long lhs = parseLogicalAnd();

        while (match("||"))
        {
            long long rhs = parseLogicalAnd();
            lhs = lhs || rhs;
        }

        return lhs;
    };

    try
    {
        long long value = parseLogicalOr();
        skipSpaces();

        if (pos != s.size())
            throw std::runtime_error("Unexpected token in #if expression");

        return value != 0;
    }
    catch (...)
    {
        return 0;
    }
}

// Wandelt ein rohes Makroargument in die quotierte, escapte Form um, die
// der '#'-Stringize-Operator produziert.
std::string MacroParser::stringizeArg(const std::string &raw)
{
    std::string trimmed = StringUtils::trim(raw);

    // Internes Whitespace (beliebige Länge/Art) wird zu genau einem
    // Leerzeichen zusammengefasst, wie es der C-Standard vorschreibt.
    std::string collapsed;
    bool lastWasSpace = false;

    for (char c : trimmed)
    {
        if (std::isspace((unsigned char)c))
        {
            if (!lastWasSpace)
                collapsed += ' ';

            lastWasSpace = true;
        }
        else
        {
            collapsed += c;
            lastWasSpace = false;
        }
    }

    // '"' und '\' müssen im Ergebnis escaped werden, damit z.B. ein
    // Argument wie "abc" (inkl. Anführungszeichen) sauber innerhalb der
    // äußeren Quotes landet.
    std::string escaped;

    for (char c : collapsed)
    {
        if (c == '"' || c == '\\')
            escaped += '\\';

        escaped += c;
    }

    return "\"" + escaped + "\"";
}

namespace
{
    // Kleine Token-Repräsentation nur für substituteParams: zerlegt den
    // Makro-Body in Bezeichner, Zahlen, "#", "##", String-Literale und
    // beliebigen sonstigen Text, damit '#' (stringize) und '##' (token
    // pasting) korrekt (und nur dort) angewendet werden.
    enum class SubstTokKind
    {
        Ident,
        HashHash,
        Hash,
        Text,
        Str
    };

    struct SubstTok
    {
        SubstTokKind kind;
        std::string text;
    };
} // namespace

std::string
MacroParser::substituteParams(const std::string &body,
                              const std::vector<std::string> &params,
                              const std::vector<std::string> &rawValues,
                              const std::vector<std::string> &expandedValues)
{
    //------------------------------------------------------------
    // 1) Tokenisieren
    //------------------------------------------------------------

    std::vector<SubstTok> tokens;
    size_t i = 0;

    while (i < body.size())
    {
        char c = body[i];

        if (c == '"' || c == '\'')
        {
            char quote = c;
            size_t start = i;
            i++;

            while (i < body.size() && body[i] != quote)
            {
                if (body[i] == '\\' && i + 1 < body.size())
                    i++;
                i++;
            }

            if (i < body.size())
                i++;

            tokens.push_back(
                {SubstTokKind::Str, body.substr(start, i - start)});
            continue;
        }

        if (body.compare(i, 2, "##") == 0)
        {
            tokens.push_back({SubstTokKind::HashHash, "##"});
            i += 2;
            continue;
        }

        if (c == '#')
        {
            tokens.push_back({SubstTokKind::Hash, "#"});
            i += 1;
            continue;
        }

        if (isIdentifierChar(c))
        {
            size_t start = i;

            while (i < body.size() && isIdentifierChar(body[i]))
                i++;

            tokens.push_back(
                {SubstTokKind::Ident, body.substr(start, i - start)});
            continue;
        }

        size_t start = i;

        while (i < body.size() && body[i] != '"' && body[i] != '\'' &&
               body[i] != '#' && !isIdentifierChar(body[i]) &&
               body.compare(i, 2, "##") != 0)
        {
            i++;
        }

        if (i == start)
            i++; // Sicherheitsnetz, damit die Schleife immer voranschreitet

        tokens.push_back({SubstTokKind::Text, body.substr(start, i - start)});
    }

    auto findParamIndex = [&](const std::string &name) -> int
    {
        for (size_t k = 0; k < params.size(); k++)
        {
            if (params[k] == name)
                return (int)k;
        }

        return -1;
    };

    //------------------------------------------------------------
    // 2) Stringize-Pass: "#" + Parametername -> quotiertes Roh-Literal
    //------------------------------------------------------------

    for (size_t idx = 0; idx < tokens.size();)
    {
        if (tokens[idx].kind == SubstTokKind::Hash)
        {
            size_t nxt = idx + 1;

            if (nxt < tokens.size() && tokens[nxt].kind == SubstTokKind::Text &&
                StringUtils::trim(tokens[nxt].text).empty())
                nxt++;

            if (nxt < tokens.size() && tokens[nxt].kind == SubstTokKind::Ident)
            {
                int pi = findParamIndex(tokens[nxt].text);

                if (pi >= 0)
                {
                    SubstTok merged{SubstTokKind::Str,
                                    stringizeArg(rawValues[pi])};

                    tokens.erase(tokens.begin() + idx,
                                 tokens.begin() + nxt + 1);
                    tokens.insert(tokens.begin() + idx, merged);
                    continue; // an gleicher Stelle erneut prüfen
                }
            }
        }

        idx++;
    }

    //------------------------------------------------------------
    // 3) Token-Pasting-Pass: operand1 "##" operand2 -> ein neues Token
    //    (linke/rechte Operanden werden dabei NICHT makroexpandiert -
    //    Parameter werden mit ihrem rohen, unexpandierten Text
    //    eingesetzt, wie es der C-Standard vorschreibt)
    //------------------------------------------------------------

    for (size_t idx = 0; idx < tokens.size();)
    {
        if (tokens[idx].kind == SubstTokKind::HashHash)
        {
            long li = (long)idx - 1;

            if (li >= 0 && tokens[li].kind == SubstTokKind::Text &&
                StringUtils::trim(tokens[li].text).empty())
                li--;

            long ri = (long)idx + 1;

            if (ri < (long)tokens.size() &&
                tokens[ri].kind == SubstTokKind::Text &&
                StringUtils::trim(tokens[ri].text).empty())
                ri++;

            if (li >= 0 && ri < (long)tokens.size())
            {
                auto rawTextOf = [&](const SubstTok &t) -> std::string
                {
                    if (t.kind == SubstTokKind::Ident)
                    {
                        int pi = findParamIndex(t.text);

                        if (pi >= 0)
                            return rawValues[pi];
                    }

                    return t.text;
                };

                std::string merged =
                    rawTextOf(tokens[li]) + rawTextOf(tokens[ri]);

                bool allIdentChars = !merged.empty();

                for (char c : merged)
                {
                    if (!isIdentifierChar(c))
                    {
                        allIdentChars = false;
                        break;
                    }
                }

                SubstTok mergedTok{allIdentChars ? SubstTokKind::Ident
                                                 : SubstTokKind::Text,
                                   merged};

                tokens.erase(tokens.begin() + li, tokens.begin() + ri + 1);
                tokens.insert(tokens.begin() + li, mergedTok);

                idx = (size_t)
                    li; // ggf. verkettete "##" ab hier weiterverarbeiten
                continue;
            }
        }

        idx++;
    }

    //------------------------------------------------------------
    // 4) Normale Substitution der verbleibenden Parameter-Bezeichner
    //    (mit dem bereits makroexpandierten Argumentwert) + Zusammenbau
    //------------------------------------------------------------

    std::string out;

    for (auto &t : tokens)
    {
        if (t.kind == SubstTokKind::Ident)
        {
            int pi = findParamIndex(t.text);

            if (pi >= 0)
                out += expandedValues[pi];
            else
                out += t.text;
        }
        else
        {
            out += t.text;
        }
    }

    return out;
}

// Versucht, ab der Klammer die auf einen Makronamen folgt, eine
// Argumentliste einzulesen (verschachtelte Klammern und Anführungszeichen
// werden respektiert), expandiert bei Erfolg das Makro rekursiv und hängt
// das Ergebnis an 'out' an. 'i' wird immer korrekt hinter den kompletten
// verarbeiteten Text gesetzt (egal ob expandiert wurde oder nicht), damit
// nie Text verloren geht.
bool MacroParser::tryExpandFunctionCall(
    const std::string &text, size_t identStart, size_t identEnd,
    const std::string &name, const Macro &macro,
    std::unordered_set<std::string> &expanding, std::string &out, size_t &i)
{
    size_t look = identEnd;

    while (look < text.size() && std::isspace((unsigned char)text[look]))
        look++;

    if (look >= text.size() || text[look] != '(')
    {
        // Kein Aufruf - Makroname bleibt unverändert stehen
        out += name;
        i = identEnd;
        return true;
    }

    size_t p = look + 1;
    size_t depth = 1;
    std::vector<std::string> args;
    std::string current;

    while (p < text.size() && depth > 0)
    {
        char c = text[p];

        if (c == '"' || c == '\'')
        {
            char quote = c;
            current += c;
            p++;

            while (p < text.size() && text[p] != quote)
            {
                if (text[p] == '\\' && p + 1 < text.size())
                {
                    current += text[p];
                    p++;
                }
                current += text[p];
                p++;
            }

            if (p < text.size())
            {
                current += text[p];
                p++;
            }

            continue;
        }

        if (c == '(')
        {
            depth++;
            current += c;
            p++;
            continue;
        }

        if (c == ')')
        {
            depth--;

            if (depth > 0)
                current += c;

            p++;
            continue;
        }

        if (c == ',' && depth == 1)
        {
            args.push_back(current);
            current.clear();
            p++;
            continue;
        }

        current += c;
        p++;
    }

    if (depth != 0)
    {
        // Unbalancierte Klammern -> kein gültiger Aufruf, unverändert lassen
        out += name;
        i = identEnd;
        return true;
    }

    bool callHasNoArgsAtAll =
        args.empty() && StringUtils::trim(current).empty();

    if (!(macro.parameters.empty() && callHasNoArgsAtAll))
        args.push_back(current);

    bool arityOk = macro.variadic ? args.size() >= macro.parameters.size()
                                  : args.size() == macro.parameters.size();
    bool recursive = expanding.contains(name);

    if (!arityOk || recursive)
    {
        // Nicht expandierbar (falsche Argumentanzahl oder rekursiver
        // Selbstaufruf) -> Originaltext unverändert übernehmen
        out += text.substr(identStart, p - identStart);
        i = p;
        return true;
    }

    std::vector<std::string> params = macro.parameters;
    std::vector<std::string> rawValues;
    std::vector<std::string> expandedValues;

    size_t namedCount = macro.parameters.size();

    // WICHTIG: Der Rekursions-Guard für 'name' wird erst NACH der
    // Argument-Expansion gesetzt (siehe unten). Ein Argument darf
    // denselben Makronamen erneut aufrufen (z.B. "#define ADD3(a,b,c)
    // ADD(ADD(a,b),c)" - das innere ADD(a,b) ist ein eigenständiger,
    // neuer Aufruf und kein Selbstaufruf innerhalb des ADD-Bodys).

    for (size_t k = 0; k < namedCount; k++)
    {
        std::string rawTrimmed = StringUtils::trim(args[k]);
        rawValues.push_back(rawTrimmed);
        expandedValues.push_back(expandText(rawTrimmed, expanding));
    }

    if (macro.variadic)
    {
        // Alle Argumente jenseits der benannten Parameter werden - so
        // wie sie im Aufruf durch Kommas getrennt waren - zu
        // __VA_ARGS__ zusammengefügt (einmal roh, einmal expandiert).
        std::string rawJoined;
        std::string expandedJoined;

        for (size_t k = namedCount; k < args.size(); k++)
        {
            std::string rawTrimmed = StringUtils::trim(args[k]);
            std::string expandedTrimmed = expandText(rawTrimmed, expanding);

            if (k > namedCount)
            {
                rawJoined += ",";
                expandedJoined += ",";
            }

            rawJoined += rawTrimmed;
            expandedJoined += expandedTrimmed;
        }

        params.push_back("__VA_ARGS__");
        rawValues.push_back(rawJoined);
        expandedValues.push_back(expandedJoined);
    }

    // Ab hier ist 'name' erneut gesperrt, während sein eigener Body
    // substituiert und rückgescannt wird - so bleibt echte
    // Selbstreferenz (z.B. "#define REC(x) REC(x)+1") weiterhin
    // korrekt terminierend.
    expanding.insert(name);

    std::string substituted =
        substituteParams(macro.body, params, rawValues, expandedValues);
    std::string expanded = expandText(substituted, expanding);

    expanding.erase(name);

    out += expanded;
    i = p;

    return true;
}

std::string MacroParser::expandText(const std::string &text,
                                    std::unordered_set<std::string> &expanding)
{
    std::string out;

    size_t i = 0;

    while (i < text.size())
    {
        // String-Literale unverändert kopieren, damit darin enthaltene
        // Bezeichner nicht versehentlich als Makros expandiert werden.
        if (text[i] == '"')
        {
            size_t start = i;
            i++;

            while (i < text.size() && text[i] != '"')
            {
                if (text[i] == '\\' && i + 1 < text.size())
                    i++;
                i++;
            }

            if (i < text.size())
                i++;

            out += text.substr(start, i - start);
            continue;
        }

        if (isIdentifierChar(text[i]))
        {
            size_t start = i;

            while (i < text.size() && isIdentifierChar(text[i]))
                i++;

            std::string ident = text.substr(start, i - start);

            auto it = macros.find(ident);

            if (it == macros.end())
            {
                out += ident;
                continue;
            }

            const Macro &macro = it->second;

            if (macro.functionLike)
            {
                tryExpandFunctionCall(text, start, i, ident, macro, expanding,
                                      out, i);
                continue;
            }

            // Objekt-Makro: den Body expandieren und das Ergebnis
            // zusammen mit dem REST des gerade gescannten Textes erneut
            // absuchen (Standard-Rescan-Regel). Das ist nötig, damit
            // z.B. "#define B A" gefolgt von "#define A(x) x" und dem
            // Aufruf "B(5)" korrekt zu "5" wird: B expandiert isoliert
            // erstmal nur zu "A", aber das direkt danach im Text
            // stehende "(5)" gehört noch zu diesem Aufruf und muss mit
            // "A" zusammen als Funktionsmakro-Invokation erkannt werden.
            //
            // Die Bedingung "expansion != ident" verhindert eine
            // Endlosschleife für den (durch den expanding-Guard
            // blockierten) Fall einer echten Selbstreferenz, bei der
            // expandMacro() den Namen unverändert zurückgibt - dann
            // wäre expansion + rest exakt wieder der ursprüngliche Text
            // ab dieser Stelle.
            std::string expansion = expandMacro(ident, expanding);

            if (expansion != ident)
            {
                out += expandText(expansion + text.substr(i), expanding);
                return out;
            }

            out += expansion;
        }
        else
        {
            out += text[i++];
        }
    }

    return out;
}

std::string
MacroParser::expandTextForIf(const std::string &text,
                             std::unordered_set<std::string> &expanding)
{
    std::string out;
    size_t i = 0;

    while (i < text.size())
    {
        if (isIdentifierChar(text[i]))
        {
            size_t start = i;

            while (i < text.size() && isIdentifierChar(text[i]))
                i++;

            std::string ident = text.substr(start, i - start);

            if (ident == "defined")
            {
                out += ident;

                size_t look = i;

                while (look < text.size() &&
                       std::isspace((unsigned char)text[look]))
                    look++;

                if (look < text.size() && text[look] == '(')
                {
                    // "defined" + Leerraum + '(' unverändert übernehmen
                    out += text.substr(i, look - i + 1);

                    size_t idStart = look + 1;
                    size_t p = idStart;

                    while (p < text.size() && isIdentifierChar(text[p]))
                        p++;

                    out += text.substr(idStart, p - idStart);

                    size_t look2 = p;

                    while (look2 < text.size() &&
                           std::isspace((unsigned char)text[look2]))
                        look2++;

                    if (look2 < text.size() && text[look2] == ')')
                    {
                        out += text.substr(p, look2 - p + 1);
                        i = look2 + 1;
                    }
                    else
                    {
                        i = p;
                    }
                }
                else
                {
                    // "defined" + Leerraum + NAME (ohne Klammern)
                    out += text.substr(i, look - i);

                    size_t idStart = look;
                    size_t p = idStart;

                    while (p < text.size() && isIdentifierChar(text[p]))
                        p++;

                    out += text.substr(idStart, p - idStart);
                    i = p;
                }

                continue;
            }

            auto it = macros.find(ident);

            if (it == macros.end())
            {
                out += ident;
                continue;
            }

            const Macro &macro = it->second;

            if (macro.functionLike)
            {
                tryExpandFunctionCall(text, start, i, ident, macro, expanding,
                                      out, i);
                continue;
            }

            // Gleiche Rescan-Korrektur wie in expandText() - siehe dort.
            std::string expansion = expandMacro(ident, expanding);

            if (expansion != ident)
            {
                out += expandTextForIf(expansion + text.substr(i), expanding);
                return out;
            }

            out += expansion;
        }
        else
        {
            out += text[i++];
        }
    }

    return out;
}

std::string MacroParser::expandMacro(const std::string &name,
                                     std::unordered_set<std::string> &expanding)
{
    if (expanding.contains(name))
        return name;

    expanding.insert(name);

    const Macro &macro = macros.at(name);

    std::string result = expandText(macro.body, expanding);

    expanding.erase(name);

    return result;
}

bool MacroParser::isActive()
{
    std::stack<IfState> temp = IfStack;

    while (!temp.empty())
    {
        if (!temp.top().active)
            return false;

        temp.pop();
    }

    return true;
}

bool MacroParser::isIdentifierChar(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// Parse these comments
std::string MacroParser::parseSlashComments(const std::string &input)
{
    std::string output;

    bool comment = false;
    bool string = false;

    for (size_t i = 0; i < input.size(); i++)
    {
        char c = input[i];

        if (!comment && c == '"')
        {
            string = !string;
        }

        if (!string && c == '/' && i + 1 < input.size() && input[i + 1] == '/')
        {
            comment = true;
            i++;
            continue;
        }

        if (comment)
        {
            if (c == '\n')
            {
                comment = false;
                output += c;
            }

            continue;
        }

        output += c;
    }

    return output;
}

// Parse the /* */ comments
std::string MacroParser::parseBlockComments(const std::string &input)
{
    std::string output;

    bool foundbc = false;
    bool string = false;

    // Remove block comments
    for (size_t i = 0; i < input.size(); i++)
    {
        char c = input[i];

        // Innerhalb eines String-Literals darf "/*" nicht als
        // Kommentarstart erkannt werden.
        if (!foundbc && c == '"')
        {
            string = !string;
        }

        // find /*
        if (!string && !foundbc && input.compare(i, 2, "/*") == 0)
        {
            foundbc = true;
            output += ' '; // Kommentar wird zu Leerzeichen
            i++;
            continue;
        }

        // find */
        if (foundbc && input.compare(i, 2, "*/") == 0)
        {
            foundbc = false;
            i++;
            continue;
        }

        if (foundbc)
        {
            continue;
        }

        output += c;
    }

    return parseSlashComments(output);
}

// Parse all the Macros here
std::string MacroParser::parse_macros(const std::string &input)
{
    std::stringstream stream(parseBlockComments(input));
    std::string line;
    std::string output;

    // Erst Makros sammeln und define-Zeilen entfernen
    while (std::getline(stream, line))
    {
        size_t pos = 0;

        // #define
        if (StringUtils::match_at(line, pos, "#define"))
        {
            if (isActive())
            {
                pos += 7;

                while (pos < line.size() &&
                       std::isspace(static_cast<unsigned char>(line[pos])))
                {
                    pos++;
                }

                // Name lesen
                std::string name;

                while (pos < line.size() && isIdentifierChar(line[pos]))
                {
                    name += line[pos];
                    pos++;
                }

                Macro macro;

                // Funktion?
                if (pos < line.size() && line[pos] == '(')
                {
                    macro.functionLike = true;

                    pos++; // '(' überspringen

                    std::string params;

                    while (pos < line.size() && line[pos] != ')')
                    {
                        params += line[pos];
                        pos++;
                    }

                    pos++; // ')' überspringen

                    auto list = StringUtils::split(params, ',');

                    for (auto &p : list)
                    {
                        std::string trimmed = StringUtils::trim(p);

                        if (trimmed.empty())
                            continue;

                        // "..." als (letzter) Parameter markiert ein
                        // variadisches Makro; __VA_ARGS__ steht dann im
                        // Body für alle überzähligen Aufrufargumente.
                        if (trimmed == "...")
                            macro.variadic = true;
                        else
                            macro.parameters.push_back(trimmed);
                    }
                }
                else
                {
                    macro.functionLike = false;
                }

                macro.body = StringUtils::trim(line.substr(pos));

                macros[name] = macro;
            }

            continue;
        }

        // #undef
        else if (StringUtils::match_at(line, pos, "#undef"))
        {
            if (isActive())
            {
                pos += 6;

                while (pos < line.size() &&
                       std::isspace(static_cast<unsigned char>(line[pos])))
                {
                    pos++;
                }

                std::string name = StringUtils::read_word(line, pos);

                macros.erase(name);
            }

            // undef zeile nicht in output schreiben
            continue;
        }

        // #ifdef
        else if (StringUtils::match_at(line, pos, "#ifdef"))
        {
            // WICHTIG: muss immer gepusht werden (auch wenn wir gerade in
            // einem inaktiven Block stecken), sonst gerät der IfStack aus
            // dem Takt und ein späteres #endif poppt den falschen Frame.
            pos += 6;

            while (pos < line.size() &&
                   std::isspace(static_cast<unsigned char>(line[pos])))
            {
                pos++;
            }

            std::string name = StringUtils::read_word(line, pos);

            bool parent = isActive();
            bool exists = parent && macros.contains(name);

            IfStack.push({parent, exists, parent && exists});

            continue;
        }

        // #ifndef
        else if (StringUtils::match_at(line, pos, "#ifndef"))
        {
            pos += 7;

            while (pos < line.size() &&
                   std::isspace(static_cast<unsigned char>(line[pos])))
            {
                pos++;
            }

            std::string name = StringUtils::read_word(line, pos);

            bool parent = isActive();
            bool notExists = parent && !macros.contains(name);

            IfStack.push({parent, notExists, parent && notExists});

            continue;
        }

        // #if
        else if (StringUtils::match_at(line, pos, "#if"))
        {
            pos += 3;

            std::string expr = line.substr(pos);
            expr = StringUtils::trim(expr);

            bool parent = isActive();
            bool result = false;

            if (parent)
            {
                // Macros ersetzen (defined(...) bleibt dabei geschützt)
                std::unordered_set<std::string> expanding;
                expr = expandTextForIf(expr, expanding);

                // unbekannte Tokens werden 0
                result = evaluateIf(expr) != 0;
            }

            IfStack.push({parent, result, parent && result});

            continue;
        }

        // #else
        else if (StringUtils::match_at(line, pos, "#else"))
        {
            if (!IfStack.empty())
            {
                IfState &state = IfStack.top();

                state.active = state.parentActive && !state.condition;
            }

            continue;
        }

        // #endif
        else if (StringUtils::match_at(line, pos, "#endif"))
        {
            pos += 6;

            if (!IfStack.empty())
            {
                IfStack.pop();
            }

            continue;
        }

        // #include
        else if (StringUtils::match_at(line, pos, "#include"))
        {
            if (isActive())
            {
                pos += 8; // hinter "#include"

                while (pos < line.size() &&
                       std::isspace(static_cast<unsigned char>(line[pos])))
                {
                    pos++;
                }

                std::string filename;

                if (pos < line.size() && line[pos] == '"')
                {
                    pos++; // erstes "

                    while (pos < line.size() && line[pos] != '"')
                    {
                        filename += line[pos++];
                    }

                    if (pos < line.size() && line[pos] == '"')
                    {
                        pos++; // schließendes "
                    }
                }
                else
                {
                    filename = StringUtils::read_word(line, pos);
                }

                // add file
                std::ifstream file(filename);

                if (file)
                {
                    std::stringstream buffer;
                    buffer << file.rdbuf();

                    line = buffer.str();
                }
            }
        }

        if (isActive())
        {
            if (!output.empty())
            {
                output += "\n";
            }

            output += line;
        }
    }

    // Danach Makros ersetzen (Objekt- UND funktionsähnliche Makros,
    // rekursiv, in der Reihenfolge in der sie im Text vorkommen)
    std::unordered_set<std::string> expanding;
    output = expandText(output, expanding);

    return output;
}

void MacroParser::add_macro(const std::string &name, const Macro &macro)
{
    macros[name] = macro;
}

void MacroParser::remove_macro(const std::string &name) { macros.erase(name); }

bool MacroParser::contains_macro(const std::string &name)
{
    return macros.contains(name);
}

Macro MacroParser::get_macro(const std::string &name)
{
    return macros.at(name);
}

void MacroParser::clear_macros() { macros.clear(); }
