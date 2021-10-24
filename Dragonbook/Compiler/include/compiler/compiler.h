#pragma once

#include <string>
#include <filesystem>

std::string Compile(const std::filesystem::path& grammar, std::string&& input);
