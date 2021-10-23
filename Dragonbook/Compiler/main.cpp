#define BOOST_TEST_MODULE compiler_tests tests
#include <boost/test/included/unit_test.hpp>

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <variant>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stack>
#include <fstream>
#include <regex>
#include <set>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

using Type = std::pair<std::string, size_t>;
using SymbolTable = std::map<std::string, Type>;

using Token = std::pair<std::string, std::string>;

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

struct Array
{
    std::string name;
    std::deque<std::string> indexes;
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

    Array arr;
    Code code;
    Token token;
};

using AnnotatedState = std::pair<State, Annotation>;

static size_t GetSizeOf(const std::string& basicType)
{
    if (basicType == "int")
        return 8;
    else if (basicType == "float")
        return 4;
    else
        throw std::invalid_argument("");
}

void ReduceHandler(const Reduce& reduce, std::vector<AnnotatedState>&& oldStates, SymbolTable& symbols, AnnotatedState& newState, size_t& tempVarsCounter)
{
    const auto generateTempVar = [&tempVarsCounter]()
    {
        return "t" + std::to_string(tempVarsCounter++);
    };

    const auto binaryOp = [&](const std::string& opName)
    {
        const Code lhsCode = oldStates[0].second.code;
        const Code rhsCode = oldStates[2].second.code;

        const std::string temp = generateTempVar();
        symbols.insert({ temp, { "int", 8 } });

        newState.second.code.result = temp;
        newState.second.code.lines = lhsCode.lines + rhsCode.lines
            + temp + " = " + lhsCode.result + " " + opName + " " + rhsCode.result + "\n";
    };

    const auto parseArray = [&symbols, &generateTempVar](const Array& arr)
    {
        std::string arrTypeName = arr.name;
        std::string newCode;
        std::queue<std::string> vars;

        std::deque<std::string> indexes = arr.indexes;
        std::reverse(indexes.begin(), indexes.end());
        
        for (auto it = indexes.rbegin(); it != indexes.rend(); ++it)
        {
            arrTypeName += "[]";
            const auto varSize = symbols.at(arrTypeName).second;

            const std::string offset = generateTempVar();
            symbols.insert({ offset, { "size_t", 64 } });

            newCode += (offset + " = " + *it + " * " + std::to_string(varSize) + "\n");
            vars.push(offset);
        }

        std::string newResult;
        if (vars.size() > 1)
        {
            const std::string temp = generateTempVar();
            const auto t1 = vars.front();
            vars.pop();
            const auto t2 = vars.front();
            vars.pop();

            symbols.insert({ t1, { "size_t", 64 } });
            symbols.insert({ t2, { "size_t", 64 } });

            newCode += (temp + " = " + t1 + " + " + t2 + "\n");
            newResult = temp;

            while (!vars.empty())
            {
                const std::string nextTemp = generateTempVar();
                symbols.insert({ nextTemp, { "size_t", 64 } });

                newCode += (nextTemp + " = " + newResult + " + " + vars.front() + "\n");
                vars.pop();
                newResult = nextTemp;
            }
        }
        else
        {
            newResult = vars.front();
        }

        const std::string temp = generateTempVar();
        symbols.insert({ temp, symbols.at(arrTypeName) });

        return std::pair<std::string, std::string>{ temp, arr.lines + newCode
            + temp + " = " + arr.name + " [" + newResult + "]\n" };
    };

    // G' -> G
    if (reduce.to == std::vector<GrammarSymbol>{ { true, "G" } })
    {
        newState.second.code = oldStates[0].second.code;
    }
    // G -> G Declarations Assign
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "G" }, { true, "Declarations" }, { true, "Assign" } })
    {
        const Code lhsCode = oldStates[0].second.code;
        const Code rhsCode = oldStates[2].second.code;

        newState.second.code.lines = lhsCode.lines + rhsCode.lines;
    }
    // G -> 
    // Assign -> id = Expr ;
    else if (reduce.to == std::vector<GrammarSymbol>{ { false, "id" }, { false, "=" }, { true, "Expr" }, { false, ";" } })
    {
        const auto varName = oldStates[0].second.token.second;
        const Code oldCode = oldStates[2].second.code;

        newState.second.code.result = varName;
        newState.second.code.lines = oldCode.lines
            + varName + " = " + oldCode.result + "\n";
    }
    // Assign -> Array = Expr ;
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "Array" }, { false, "=" }, { true, "Expr" }, { false, ";" } })
    {
        const auto arr = parseArray(oldStates[0].second.arr);

        const Code oldCode = oldStates[2].second.code;

        newState.second.code.result = arr.first;
        newState.second.code.lines = oldCode.lines + arr.second
            + arr.first + " = " + oldCode.result + "\n";
    }
    // Assign ->
    // Expr -> Expr * Expr
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "Expr" }, { false, "*" }, { true, "Expr" } })
    {
        binaryOp("*");
    }
    // Expr -> Expr / Expr
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "Expr" }, { false, "/" }, { true, "Expr" } })
    {
        binaryOp("/");
    }
    // Expr -> Expr + Expr
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "Expr" }, { false, "+" }, { true, "Expr" } })
    {
        binaryOp("+");
    }
    // Expr -> Expr - Expr
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "Expr" }, { false, "-" }, { true, "Expr" } })
    {
        binaryOp("-");
    }
    // Expr -> - Expr
    else if (reduce.to == std::vector<GrammarSymbol>{ { false, "-" }, { true, "Expr" } })
    {
        const Code oldCode = oldStates[1].second.code;

        const std::string temp = generateTempVar();
        symbols.insert({ temp, { "int", 8 } });
        newState.second.code.lines = oldCode.lines + temp + " = 0 - " + oldCode.result + "\n";
        newState.second.code.result = temp;
    }
    // Expr -> ( Expr )
    else if (reduce.to == std::vector<GrammarSymbol>{ { false, "(" }, { true, "Expr" } , { false, ")" } })
    {
        newState.second.code = oldStates[1].second.code;
    }
    // Expr -> id
    else if (reduce.to == std::vector<GrammarSymbol>{ { false, "id" } })
    {
        newState.second.code.result = oldStates[0].second.token.second;
    }
    // Expr -> Array
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "Array" } })
    {
        const auto res = parseArray(oldStates[0].second.arr);
        newState.second.code.result = res.first;
        newState.second.code.lines = res.second;
    }
    // Expr -> num
    else if (reduce.to == std::vector<GrammarSymbol>{ { false, "num" } })
    {
        newState.second.code.result = oldStates[0].second.token.second;
    }
    // Array -> id [ Expr ]
    else if (reduce.to == std::vector<GrammarSymbol>{ { false, "id" }, { false, "[" }, { true, "Expr" }, { false, "]" } })
    {
        const Code oldCode = oldStates[2].second.code;
        const auto varName = oldStates[0].second.token.second;

        newState.second.arr.name = varName;
        newState.second.arr.lines = oldCode.lines;
        newState.second.arr.indexes.push_back(oldCode.result);
    }
    // Array -> Array [ Expr ]
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "Array" }, { false, "[" }, { true, "Expr" }, { false, "]" } })
    {
        const Code oldCode = oldStates[2].second.code;
        const auto arr = oldStates[0].second.arr;

        newState.second.arr.name = arr.name;
        newState.second.arr.lines = arr.lines + oldCode.lines;
        newState.second.arr.indexes = arr.indexes;
        newState.second.arr.indexes.push_back(oldCode.result);
    }
    // Declarations -> Type id ; Declarations
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "Type" }, { false, "id" }, { false, ";" }, { true, "Declarations" } })
    {
        auto varName = oldStates[1].second.token.second;
        Array type = oldStates[0].second.arr;

        for (size_t i = 0; i < type.indexes.size(); ++i)
            varName += "[]";

        auto typeName = type.name;
        auto size = GetSizeOf(type.name);

        symbols.try_emplace(varName, typeName, size);

        //type.indexes.pop_front();

        for (auto it = type.indexes.rbegin(); it != type.indexes.rend(); ++it)
        {
            size *= boost::lexical_cast<size_t>(*it);
            varName.pop_back(); // pop ]
            varName.pop_back(); // pop [
            typeName += "[]";

            symbols.try_emplace(varName, typeName, size);
        }
    }
    // Declarations ->
    // Type -> BasicType IndexesOptional
    else if (reduce.to == std::vector<GrammarSymbol>{ { true, "BasicType" }, { true, "IndexesOptional" } })
    {
        newState.second.arr.name = oldStates[0].second.arr.name;
        newState.second.arr.indexes = oldStates[1].second.arr.indexes;
    }
    // Type -> record { Declarations }
    // BasicType -> int
    else if (reduce.to == std::vector<GrammarSymbol>{ { false, "int" } })
    {
        newState.second.arr.name = "int";
    }
    // BasicType -> float
    else if (reduce.to == std::vector<GrammarSymbol>{ { false, "float" } })
    {
        newState.second.arr.name = "float";
    }
    /// IndexesOptional ->[ num ] IndexesOptional
    else if (reduce.to == std::vector<GrammarSymbol>{ { false, "[" }, { false, "num" }, { false, "]" }, { true, "IndexesOptional" } })
    {
        auto indexes = oldStates[3].second.arr.indexes;
        indexes.push_front(oldStates[1].second.token.second);
        newState.second.arr.indexes = indexes;
    }
    // IndexesOptional ->
}

class LrAnalyzer
{
public:
    LrAnalyzer(const LalrTable& table, const std::queue<Token>& input)
        : m_t(table)
        , m_input(input)
        , m_tempVarsCounter(0)
    {
        m_states.push({ 0, Annotation{} });
        m_input.push(Token{});
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
                        ReduceHandler(arg, std::move(states), m_symbols, m_states.top(), m_tempVarsCounter);
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
    size_t m_tempVarsCounter;
};

LalrTable ParseGrammarFile()
{
    std::ifstream g("grammar.csv");

    std::string header;
    std::getline(g, header);

    std::vector<GrammarSymbol> termsAndNonTerms;
    {
        boost::char_separator<char> sep{","};
        boost::tokenizer<decltype(sep)> tokenizer(header, sep);

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
            boost::char_separator<char> sep{" "};
            boost::tokenizer<decltype(sep)> tokenizer(line, sep);

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
            boost::char_separator<char> sep("", ",", boost::drop_empty_tokens);
            boost::tokenizer<boost::char_separator<char>> tokenizer(line, sep);

            bool first = true;
            size_t counter = 0;
            for (const auto& token : tokenizer)
            {
                if (first)
                {
                    first = false;
                    continue;
                }

                if (token == ",")
                {
                    counter++;
                    continue;
                }

                if (token.front() == 's')
                {
                    const auto stateInt = boost::lexical_cast<uint32_t>(token.substr(1));
                    const auto action = Shift{ stateInt };
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, action });
                }
                else if (token.front() == 'r')
                {
                    const auto reduceInt = boost::lexical_cast<uint32_t>(token.substr(1));
                    const auto action = reduces[reduceInt];
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, action });
                }
                else if (token == "acc")
                {
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, Accept{} });
                }
                else if (!token.empty())
                {
                    const auto stateInt = boost::lexical_cast<uint32_t>(token);
                    const auto action = Shift{ stateInt };
                    table.insert({ { State{num}, termsAndNonTerms[counter - 1] }, action });
                }
            }
            num++;
        }
    }

    return table;
}

std::queue<Token> Tokenize(std::string&& input)
{
    std::regex id("[a-zA-Z0-9]+");
    std::regex op(R"(\[|\]|=|;|\+|-|\*|\/|\(|\))");
    std::regex empty("[ \t]+");
    std::regex num("[0-9]+");
    std::regex keyword("record|int|float");

    std::queue<Token> tokens;
    while (!input.empty())
    {
        std::smatch match;
        if (std::regex_search(input, match, keyword, std::regex_constants::match_continuous))
        {
            tokens.push({ match.str(), "" });
        }
        else if (std::regex_search(input, match, num, std::regex_constants::match_continuous))
        {
            tokens.push({ "num", match.str() });
        }
        else if (std::regex_search(input, match, id, std::regex_constants::match_continuous))
        {
            tokens.push({ "id", match.str() });
        }
        else if (std::regex_search(input, match, op, std::regex_constants::match_continuous))
        {
            tokens.push({ match.str(), "" });
        }
        else
        {
            std::regex_search(input, match, empty, std::regex_constants::match_continuous);
        }

        if (match.empty())
            throw std::runtime_error("Lexical error: permitted characters found.");

        input = input.substr(match.str().size());
        continue;
    }

    return tokens;
}

BOOST_AUTO_TEST_CASE(ArraysTest)
{
    LalrTable table = ParseGrammarFile();

    std::string input =
        "int[5][4] a;"
        "int[5][4] b;"
        "int[5] c;"
        "x = a[b[i][j]][c[k]];"
    ;

    const std::string expectedCode =
        "t0 = i * 32\n"
        "t1 = j * 8\n"
        "t2 = t0 + t1\n"
        "t3 = b [t2]\n"
        "t4 = k * 8\n"
        "t5 = c [t4]\n"
        "t6 = t3 * 32\n"
        "t7 = t5 * 8\n"
        "t8 = t6 + t7\n"
        "t9 = a [t8]\n"
        "x = t9\n";

    std::queue<Token> tokens;
    tokens = Tokenize(std::move(input));

    LrAnalyzer l{ table, tokens };

    const std::string threeAddressCode = l.Analyze().code.lines;
    std::cout << threeAddressCode << std::endl;

    BOOST_TEST(threeAddressCode.c_str() == expectedCode.c_str());
}

BOOST_AUTO_TEST_CASE(SmokeTest)
{
    LalrTable table = ParseGrammarFile();

    std::string input =
        "int a;"
        "a = 1 + 1 * 2;"
        "int[3][2] b;"
        "b[0][1] = 3;"
        "b[1][0] = 5;"
        "b[2][1] = 7;"
        "int c;"
        "c = b[0][0] + a;"
        "int[4][3][2] d;"
        "d[3][1][0] = 17;"
    ;

    const std::string expectedCode =
        "t0 = 1 * 2\n"
        "t1 = 1 + t0\n"
        "a = t1\n"
        "t2 = 0 * 16\n"
        "t3 = 1 * 8\n"
        "t4 = t2 + t3\n"
        "t5 = b [t4]\n"
        "t5 = 3\n"
        "t6 = 1 * 16\n"
        "t7 = 0 * 8\n"
        "t8 = t6 + t7\n"
        "t9 = b [t8]\n"
        "t9 = 5\n"
        "t10 = 2 * 16\n"
        "t11 = 1 * 8\n"
        "t12 = t10 + t11\n"
        "t13 = b [t12]\n"
        "t13 = 7\n"
        "t14 = 0 * 16\n"
        "t15 = 0 * 8\n"
        "t16 = t14 + t15\n"
        "t17 = b [t16]\n"
        "t18 = t17 + a\n"
        "c = t18\n"
        "t19 = 3 * 48\n"
        "t20 = 1 * 16\n"
        "t21 = 0 * 8\n"
        "t22 = t19 + t20\n"
        "t23 = t22 + t21\n"
        "t24 = d [t23]\n"
        "t24 = 17\n";

    std::queue<Token> tokens;
    tokens = Tokenize(std::move(input));

    LrAnalyzer l{ table, tokens };

    const std::string threeAddressCode = l.Analyze().code.lines;
    std::cout << threeAddressCode << std::endl;

    BOOST_TEST(threeAddressCode.c_str() == expectedCode.c_str());
}
