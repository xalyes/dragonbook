#include <string>
#include <vector>
#include <map>
#include <queue>
#include <variant>
#include <iostream>
#include <sstream>
#include <stack>
#include <fstream>
#include <regex>
#include <set>

#include <Poco/StringTokenizer.h>
#include <Poco/NumberParser.h>

/*

S -> S + E
   | E.
E -> E * F
   | F.
F -> a ID | b ID
   | ( S ).
ID -> a ID | b ID | .


*/

/*
enum class Terminal : uint8_t
{
    _EOF,
    B,
    A,
    LEFT,
    RIGHT,
    MUL,
    PLUS
};

Terminal ToTerminal(const Token& token)
{
    if (token == "")
        return Terminal::_EOF;
    if (token == "b")
        return Terminal::B;
    if (token == "a")
        return Terminal::A;
    if (token == "(")
        return Terminal::LEFT;
    if (token == ")")
        return Terminal::RIGHT;
    if (token == "*")
        return Terminal::MUL;
    if (token == "+")
        return Terminal::PLUS;

    throw std::invalid_argument("ToTerminal failed. Argument: " + token);
}

enum class NonTerminal : uint8_t
{
    S = static_cast<uint8_t>(Terminal::PLUS) + 1,
    E,
    F,
    ID
};
*/

using SymbolTable = std::set<std::string>;

using Token = std::pair<std::string, SymbolTable::iterator>;

struct GrammarSymbol
{
    bool isNonTerminal{ false };
    std::string str;

    bool operator <(const GrammarSymbol& rhs) const
    {
        if (str == rhs.str)
            return (isNonTerminal < rhs.isNonTerminal);
        else
            return str < rhs.str;
    }
};

struct Terminal { std::string term; };
struct NonTerminal { std::string nonTerm; };

using State = size_t;

struct Reduce
{
    NonTerminal from;
    std::vector<GrammarSymbol> to;
};

struct Shift
{
    State st;
};

struct Accept {};

using Action = std::variant<Shift, Reduce, Accept>;
using LalrTable = std::map<std::pair<State, GrammarSymbol>, Action>;

/*
LalrTable T = {
    {{0, Terminal::B}, Shift{6}}, {{0, Terminal::A}, Shift{5}}, {{0, Terminal::LEFT}, Shift{4}}, {{0, NonTerminal::S}, Shift{3}}, {{0, NonTerminal::E}, Shift{2}}, {{0, NonTerminal::F}, Shift{1}},
    {{1, Terminal::_EOF}, Reduce{NonTerminal::E, {NonTerminal::F}}}, {{1, Terminal::RIGHT}, Reduce{NonTerminal::E, {NonTerminal::F}}}, {{1, Terminal::MUL}, Reduce{NonTerminal::E, {NonTerminal::F}}}, {{1, Terminal::PLUS}, Reduce{NonTerminal::E, {NonTerminal::F}}},
    {{2, Terminal::_EOF}, Reduce{NonTerminal::S, {NonTerminal::E}}}, {{2, Terminal::RIGHT}, Reduce{NonTerminal::S, {NonTerminal::E}}}, {{2, Terminal::MUL}, Shift{13}}, {{2, Terminal::PLUS}, Reduce{NonTerminal::S, {NonTerminal::E}}},
    {{3, Terminal::_EOF}, Accept{}}, {{3, Terminal::PLUS}, Shift{12}},
    {{4, Terminal::B}, Shift{6}}, {{4, Terminal::A}, Shift{5}}, {{4, Terminal::LEFT}, Shift{4}}, {{4, NonTerminal::S}, Shift{11}}, {{4, NonTerminal::E}, Shift{2}}, {{4, NonTerminal::F}, Shift{1}},
    {{5, Terminal::_EOF}, Reduce{NonTerminal::ID, {}}}, {{5, Terminal::B}, Shift{10}}, {{5, Terminal::A}, Shift{9}}, {{5, Terminal::RIGHT}, Reduce{NonTerminal::ID, {}}}, {{5, Terminal::MUL}, Reduce{NonTerminal::ID, {}}}, {{5, Terminal::PLUS}, Reduce{NonTerminal::ID, {}}}, {{5, NonTerminal::ID}, Shift{8}},
    {{6, Terminal::_EOF}, Reduce{NonTerminal::ID, {}}}, {{6, Terminal::B}, Shift{10}}, {{6, Terminal::A}, Shift{9}}, {{6, Terminal::RIGHT}, Reduce{NonTerminal::ID, {}}}, {{6, Terminal::MUL}, Reduce{NonTerminal::ID, {}}}, {{6, Terminal::PLUS}, Reduce{NonTerminal::ID, {}}}, {{6, NonTerminal::ID}, Shift{7}},
    {{7, Terminal::_EOF}, Reduce{NonTerminal::F, {Terminal::B, NonTerminal::ID}}}, {{7, Terminal::RIGHT}, Reduce{NonTerminal::F, {Terminal::B, NonTerminal::ID}}}, {{7, Terminal::MUL}, Reduce{NonTerminal::F, {Terminal::B, NonTerminal::ID}}}, {{7, Terminal::PLUS}, Reduce{NonTerminal::F, {Terminal::B, NonTerminal::ID}}},
    {{8, Terminal::_EOF}, Reduce{NonTerminal::F, {Terminal::A, NonTerminal::ID}}}, {{8, Terminal::RIGHT}, Reduce{NonTerminal::F, {Terminal::A, NonTerminal::ID}}}, {{8, Terminal::MUL}, Reduce{NonTerminal::F, {Terminal::A, NonTerminal::ID}}}, {{8, Terminal::PLUS}, Reduce{NonTerminal::F, {Terminal::A, NonTerminal::ID}}},
    {{9, Terminal::_EOF}, Reduce{NonTerminal::ID, {}}}, {{9, Terminal::B}, Shift{10}}, {{9, Terminal::A}, Shift{9}}, {{9, Terminal::RIGHT}, Reduce{NonTerminal::ID, {}}}, {{9, Terminal::MUL}, Reduce{NonTerminal::ID, {}}}, {{9, Terminal::PLUS}, Reduce{NonTerminal::ID, {}}}, {{9, NonTerminal::ID}, Shift{18}},
    {{10, Terminal::_EOF}, Reduce{NonTerminal::ID, {}}}, {{10, Terminal::B}, Shift{10}}, {{10, Terminal::A}, Shift{9}}, {{10, Terminal::RIGHT}, Reduce{NonTerminal::ID, {}}}, {{10, Terminal::MUL}, Reduce{NonTerminal::ID, {}}}, {{10, Terminal::PLUS}, Reduce{NonTerminal::ID, {}}}, {{10, NonTerminal::ID}, Shift{17}},
    {{11, Terminal::RIGHT}, Shift{16}}, {{11, Terminal::PLUS}, Shift{12}},
    {{12, Terminal::B}, Shift{6}}, {{12, Terminal::A}, Shift{5}}, {{12, Terminal::LEFT}, Shift{4}}, {{12, NonTerminal::E}, Shift{15}}, {{12, NonTerminal::F}, Shift{1}},
    {{13, Terminal::B}, Shift{6}}, {{13, Terminal::A}, Shift{5}}, {{13, Terminal::LEFT}, Shift{4}}, {{13, NonTerminal::F}, Shift{14}},
    {{14, Terminal::_EOF}, Reduce{NonTerminal::E, {NonTerminal::E, Terminal::MUL, NonTerminal::F}}}, {{14, Terminal::RIGHT}, Reduce{NonTerminal::E, {NonTerminal::E, Terminal::MUL, NonTerminal::F}}}, {{14, Terminal::MUL}, Reduce{NonTerminal::E, {NonTerminal::E, Terminal::MUL, NonTerminal::F}}}, {{14, Terminal::PLUS}, Reduce{NonTerminal::E, {NonTerminal::E, Terminal::MUL, NonTerminal::F}}},
    {{15, Terminal::_EOF}, Reduce{NonTerminal::S, {NonTerminal::S, Terminal::PLUS, NonTerminal::E}}}, {{15, Terminal::RIGHT}, Reduce{NonTerminal::S, {NonTerminal::S, Terminal::PLUS, NonTerminal::E}}}, {{15, Terminal::MUL}, Shift{13}}, {{15, Terminal::PLUS}, Reduce{NonTerminal::S, {NonTerminal::S, Terminal::PLUS, NonTerminal::E}}},
    {{16, Terminal::_EOF}, Reduce{NonTerminal::F, {Terminal::LEFT, NonTerminal::S, Terminal::RIGHT}}}, {{16, Terminal::RIGHT}, Reduce{NonTerminal::F, {Terminal::LEFT, NonTerminal::S, Terminal::RIGHT}}}, {{16, Terminal::MUL}, Reduce{NonTerminal::F, {Terminal::LEFT, NonTerminal::S, Terminal::RIGHT}}}, {{16, Terminal::PLUS}, Reduce{NonTerminal::F, {Terminal::LEFT, NonTerminal::S, Terminal::RIGHT}}},
    {{17, Terminal::_EOF}, Reduce{NonTerminal::ID, {Terminal::B, NonTerminal::ID}}}, {{17, Terminal::RIGHT}, Reduce{NonTerminal::ID, {Terminal::B, NonTerminal::ID}}}, {{17, Terminal::MUL}, Reduce{NonTerminal::ID, {Terminal::B, NonTerminal::ID}}}, {{17, Terminal::PLUS}, Reduce{NonTerminal::ID, {Terminal::B, NonTerminal::ID}}},
    {{18, Terminal::_EOF}, Reduce{NonTerminal::ID, {Terminal::A, NonTerminal::ID}}}, {{18, Terminal::RIGHT}, Reduce{NonTerminal::ID, {Terminal::A, NonTerminal::ID}}}, {{18, Terminal::MUL}, Reduce{NonTerminal::ID, {Terminal::A, NonTerminal::ID}}}, {{18, Terminal::PLUS}, Reduce{NonTerminal::ID, {Terminal::A, NonTerminal::ID}}}
};
*/

struct Annotation
{
    std::string str;
    bool mustEnclose = false;
};

using AnnotatedState = std::pair<State, Annotation>;

void DoTranslate(const std::vector<AnnotatedState>& oldStates, AnnotatedState& newState)
{
    if ((oldStates.size() == 3) && (oldStates[1].second.str == "+"))
    {
        newState.second.mustEnclose = true;
    }

    if ((oldStates.size() == 3) && (oldStates[0].second.str == "(") && (oldStates[2].second.str == ")") && (!oldStates[1].second.mustEnclose))
    {
        newState.second.str = oldStates[1].second.str;
        newState.second.mustEnclose = false;
        return;
    }

    std::stringstream acc;
    for (const auto& os : oldStates)
    {
        acc << os.second.str;
    }
    newState.second.str = acc.str();
}

class LrAnalyzer
{
public:
    LrAnalyzer(const LalrTable& table, const std::queue<Token>& input, const SymbolTable& symbols)
        : m_t(table)
        , m_input(input)
        , m_symbols(symbols)
    {
        m_states.push({ 0, { "", false } });
        m_input.push({ "", m_symbols.end() });
    }

    std::string Analyze()
    {
        while (true)
        {
            const GrammarSymbol elem{ false, m_input.front().first };
            const auto it = m_t.find({ m_states.top().first, elem });

            if (it == m_t.end())
                throw std::runtime_error("Syntax error. State: " + std::to_string(m_states.top().first) + ". Current token: " + m_input.front().first);

            bool accepted = false;
            std::visit(
                [this, &accepted](auto& arg)
                {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, Shift>) 
                    {
                        m_states.push({ arg.st, { m_input.front().first, false } });
                        m_input.pop();
                    }
                    else if constexpr (std::is_same_v<T, Reduce>)
                    {
                        std::vector<AnnotatedState> states;
                        for (const auto& i : arg.to)
                        {
                            states.push_back(m_states.top());
                            m_states.pop();
                        }
                        std::reverse(states.begin(), states.end());

                        const auto gotoIt = m_t.find({ m_states.top().first, GrammarSymbol{ true, arg.from.nonTerm } });
                        if (gotoIt == m_t.end())
                            throw std::runtime_error("Syntax error. State: " + std::to_string(m_states.top().first) + ". Current non terminal: " + arg.from.nonTerm);

                        const auto shift = std::get<Shift>(gotoIt->second);
                        m_states.push({ shift.st, { "", false } });
                        DoTranslate(states, m_states.top());
                    }
                    else if constexpr (std::is_same_v<T, Accept>)
                    {
                        accepted = true;
                    }
                    else
                    {
                        throw std::logic_error("visitor error");
                    }
                }, it->second);

            if (accepted)
            {
                return m_states.top().second.str;
            }
        }
    }

private:
    LalrTable m_t;
    std::queue<Token> m_input;
    std::stack<AnnotatedState> m_states;
    SymbolTable m_symbols;
};

LalrTable ParseGrammarFile()
{
    std::ifstream g("grammar.csv");

    std::string header;
    std::getline(g, header);

    std::vector<GrammarSymbol> termsAndNonTerms;
    {
        Poco::StringTokenizer tokenizer(header, ",", 1);

        bool terms = true;
        for (const auto& token : tokenizer)
        {
            if (terms)
            {
                termsAndNonTerms.push_back(GrammarSymbol{ false, token });
            }
            else
            {
                termsAndNonTerms.push_back(GrammarSymbol{ true, token } );
            }

            if (token == "$")
            {
                terms = false;
            }
        }
    }

    std::vector<Reduce> reduces;
    {
        std::string line;
        std::getline(g, line);
        while (std::getline(g, line) && (!line.empty()))
        {
            Poco::StringTokenizer tokenizer(line, " ", 1);

            Reduce r;
            for (const auto& token : tokenizer)
            {
                if (r.from.nonTerm.empty())
                {
                    r.from.nonTerm = token;
                    continue;
                }

                if (token == "->")
                    continue;

                const auto it = std::find_if(termsAndNonTerms.begin(), termsAndNonTerms.end(),
                    [&token](const GrammarSymbol& e) {
                        return e.str == token;
                    }
                );

                if (it != termsAndNonTerms.end())
                {
                    r.to.push_back(*it);
                }
                else
                {
                    throw std::runtime_error("Unknown token in grammar: '" + token + "'");
                }
            }
            reduces.push_back(r);
        }
    }

    LalrTable table;
    {
        size_t num = 0;
        std::string line;
        while (std::getline(g, line))
        {
            Poco::StringTokenizer tokenizer(line, ",", 0);

            bool first = true;
            size_t counter = 0;
            for (const auto& token : tokenizer)
            {
                if (first)
                {
                    first = false;
                    continue;
                }

                counter++;
                if (token == ",")
                    continue;

                if (token.front() == 's')
                {
                    const auto stateInt = Poco::NumberParser::parseUnsigned(token.substr(1));
                    const auto action = Shift{ stateInt };
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, action });
                }

                if (token.front() == 'r')
                {
                    const auto reduceInt = Poco::NumberParser::parseUnsigned(token.substr(1));
                    const auto action = reduces[reduceInt];
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, action });
                }

                if (token == "acc")
                {
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, Accept{} });
                }
            }
            num++;
        }
    }

    return table;
}

std::queue<Token> Tokenize(SymbolTable& symbolTable, std::string&& input)
{
    std::regex id("[a-zA-Z0-9]+");
    std::regex op(R"(=|;|\+|-|\*|\/|\(|\))");
    std::regex empty("[ \t]+");

    std::queue<Token> tokens;
    while (!input.empty())
    {
        std::smatch match;
        if (std::regex_search(input, match, id, std::regex_constants::match_continuous))
        {
            auto it = symbolTable.insert(match.str()).first;
            tokens.push({ "id", it });
        }
        else if (std::regex_search(input, match, op, std::regex_constants::match_continuous))
        {
            tokens.push({ match.str(), symbolTable.end() });
        }
        else
        {
            std::regex_search(input, match, empty, std::regex_constants::match_continuous);
        }

        if (!match.ready())
            throw std::runtime_error("Lexical error: permitted characters found.");

        input = input.substr(match.str().size());
        continue;
    }

    return tokens;
}

int main()
{
    LalrTable table = ParseGrammarFile();

    std::string input;

    std::getline(std::cin, input);

    SymbolTable symbolTable;
    std::queue<Token> tokens = Tokenize(symbolTable, std::move(input));

    LrAnalyzer l{ table, tokens, symbolTable };

    try
    {
        std::cout << l.Analyze();
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}