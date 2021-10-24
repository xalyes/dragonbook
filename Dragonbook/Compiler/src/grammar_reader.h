#pragma once

#include <map>
#include <variant>
#include <filesystem>

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

using State = size_t;

struct NonTerminal { std::string nonTerm; };

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

LalrTable ParseGrammarFile(const std::filesystem::path& grammarFile);
