#include <regex>
#include <queue>
#include <string>
#include <utility>

#include "tokenizer.h"

using Token = std::pair<std::string, std::string>;

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