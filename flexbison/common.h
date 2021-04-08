#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>

using SemanticValue = std::vector<std::pair<std::string, size_t>>;

extern std::map<size_t, std::pair<std::string, std::optional<SemanticValue>>> symbols;
