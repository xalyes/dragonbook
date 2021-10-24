#pragma once
#include <queue>
#include <stack>

#include "grammar_reader.h"

using Type = std::pair<std::string, size_t>;
using SymbolTable = std::map<std::string, Type>;

using Token = std::pair<std::string, std::string>;

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

class LrAnalyzer
{
public:
    LrAnalyzer(const LalrTable& table, const std::queue<Token>& input);
    Annotation Analyze();

private:
    LalrTable m_t;
    std::queue<Token> m_input;
    std::stack<AnnotatedState> m_states;
    SymbolTable m_symbols;
    size_t m_tempVarsCounter;
};
