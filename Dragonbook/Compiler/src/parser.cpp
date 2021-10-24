#include <map>
#include <deque>
#include <queue>
#include <stack>
#include <variant>
#include <boost/lexical_cast.hpp>

#include "grammar_reader.h"
#include "parser.h"

using Type = std::pair<std::string, size_t>;
using SymbolTable = std::map<std::string, Type>;

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
            const auto symbol = symbols.find(arrTypeName);
            if (symbol == symbols.end())
                throw std::runtime_error("Undefined symbol '" + arrTypeName + "'");

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
        const auto varName = oldStates[0].second.token.second;
        if (symbols.find(varName) == symbols.end())
            throw std::runtime_error("Undefined symbol '" + varName + "'");

        newState.second.code.result = varName;
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

LrAnalyzer::LrAnalyzer(const LalrTable& table, const std::queue<Token>& input)
    : m_t(table)
    , m_input(input)
    , m_tempVarsCounter(0)
{
    m_states.push({ 0, Annotation{} });
    m_input.push(Token{});
}

Annotation LrAnalyzer::Analyze()
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
