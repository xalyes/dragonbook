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

    bool operator ==(const GrammarSymbol& rhs) const
    {
        return (str == rhs.str) && (isNonTerminal == rhs.isNonTerminal);
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

struct Code
{
    std::string result;
    std::string lines;
};

struct Annotation
{
    Annotation() {}
    Annotation(const Token& t)
    {
        token = t;
        isToken = true;
    }

    bool isToken{ false };

    Code code;
    Token token;
};

using AnnotatedState = std::pair<State, Annotation>;

static std::string GenerateTempVar()
{
    static size_t num = 0;
    return "t" + std::to_string(num++);
}

void ReduceHandler(const Reduce& reduce, std::vector<AnnotatedState>&& oldStates, SymbolTable& symbols, AnnotatedState& newState)
{
    // 0. S -> id = E ;
    if (reduce.to == std::vector<GrammarSymbol>{ { false, "id" }, { false, "=" }, { true, "E" }, { false, ";" } })
    {
        newState.second.code.lines = oldStates[2].second.code.lines
            + *(oldStates[0].second.token.second) + " = " + oldStates[2].second.code.result + "\n";
        newState.second.code.result = *(oldStates[0].second.token.second);
    }
    // 1. E -> E * E
    if (reduce.to == std::vector<GrammarSymbol>{ { true, "E" }, { false, "*" }, { true, "E" } })
    {
        const std::string temp = GenerateTempVar();
        newState.second.code.lines = oldStates[0].second.code.lines + oldStates[2].second.code.lines
            + temp + " = " + oldStates[0].second.code.result + " * " + oldStates[2].second.code.result + "\n";
        newState.second.code.result = temp;
        symbols.insert(temp);
    }
    // 2. E -> E / E
    if (reduce.to == std::vector<GrammarSymbol>{ { true, "E" }, { false, "/" }, { true, "E" } })
    {
        const std::string temp = GenerateTempVar();
        newState.second.code.lines = oldStates[0].second.code.lines + oldStates[2].second.code.lines
            + temp + " = " + oldStates[0].second.code.result + " / " + oldStates[2].second.code.result + "\n";
        newState.second.code.result = temp;
        symbols.insert(temp);
    }
    // 3. E -> E + E
    if (reduce.to == std::vector<GrammarSymbol>{ { true, "E" }, { false, "+" }, { true, "E" } })
    {
        const std::string temp = GenerateTempVar();
        newState.second.code.lines = oldStates[0].second.code.lines + oldStates[2].second.code.lines
            + temp + " = " + oldStates[0].second.code.result + " + " + oldStates[2].second.code.result + "\n";
        newState.second.code.result = temp;
        symbols.insert(temp);
    }
    // 4. E -> E - E
    if (reduce.to == std::vector<GrammarSymbol>{ { true, "E" }, { false, "-" }, { true, "E" } })
    {
        const std::string temp = GenerateTempVar();
        newState.second.code.lines = oldStates[0].second.code.lines + oldStates[2].second.code.lines
            + temp + " = " + oldStates[0].second.code.result + " - " + oldStates[2].second.code.result + "\n";
        newState.second.code.result = temp;
        symbols.insert(temp);
    }
    // 5. E -> - E
    if (reduce.to == std::vector<GrammarSymbol>{ { false, "-" }, { true, "E" } })
    {
        const std::string temp = GenerateTempVar();
        newState.second.code.lines = oldStates[1].second.code.lines + temp + " = 0 - " + oldStates[1].second.code.result + "\n";
        newState.second.code.result = temp;
        symbols.insert(temp);
    }
    // 6. E -> ( E )
    if (reduce.to == std::vector<GrammarSymbol>{ { false, "(" }, { true, "E" } , { false, ")" } })
    {
        newState.second.code = oldStates[1].second.code;
    }
    // 7. E -> id
    if (reduce.to == std::vector<GrammarSymbol>{ { false, "id" } })
    {
        newState.second.code.result = *(oldStates[0].second.token.second);
    }
}

class LrAnalyzer
{
public:
    LrAnalyzer(const LalrTable& table, const std::queue<Token>& input, const SymbolTable& symbols)
        : m_t(table)
        , m_input(input)
        , m_symbols(symbols)
    {
        m_states.push({ 0, Annotation{} });
        m_input.push({ "", m_symbols.end() });
    }

    Annotation Analyze()
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
                        m_states.push({ arg.st, Annotation{ m_input.front() } });
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
                        m_states.push({ shift.st, Annotation{} });
                        ReduceHandler(arg, std::move(states), m_symbols, m_states.top());
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
                return m_states.top().second;
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
                if (token != "$")
                    termsAndNonTerms.push_back(GrammarSymbol{ false, token });
                else
                    termsAndNonTerms.push_back(GrammarSymbol{ false, "" });
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
                else if (token.front() == 'r')
                {
                    const auto reduceInt = Poco::NumberParser::parseUnsigned(token.substr(1));
                    const auto action = reduces[reduceInt];
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, action });
                }
                else if (token == "acc")
                {
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, Accept{} });
                }
                else if (!token.empty())
                {
                    const auto stateInt = Poco::NumberParser::parseUnsigned(token);
                    const auto action = Shift{ stateInt };
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, action });
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
            tokens.push({ match.str(), SymbolTable::iterator() });
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
        std::cout << l.Analyze().code.lines;
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}