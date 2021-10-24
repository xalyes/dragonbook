#include <compiler/compiler.h>
#include "grammar_reader.h"
#include "parser.h"
#include "tokenizer.h"

std::string Compile(const std::filesystem::path& grammar, std::string&& input)
{
    LalrTable table = ParseGrammarFile(grammar);

    std::queue<Token> tokens;
    tokens = Tokenize(std::move(input));

    LrAnalyzer l{ table, tokens };

    return l.Analyze().code.lines;
}
