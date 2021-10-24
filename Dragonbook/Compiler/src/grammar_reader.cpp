#include <vector>
#include <string>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include "grammar_reader.h"

LalrTable ParseGrammarFile(const std::filesystem::path& grammarFile)
{
    std::ifstream g(grammarFile);

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