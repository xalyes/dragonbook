#pragma once
#include <queue>
#include <string>
#include <utility>

using Token = std::pair<std::string, std::string>;

std::queue<Token> Tokenize(std::string&& input);
