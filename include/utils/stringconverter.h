#pragma once

#include <string>
#include <utility>

std::pair<std::string, std::string> split_at(const std::string &input, const std::string &delimiter);
std::string to_lower(const std::string &s);
std::string get_date(const std::string &fmt);
std::string get_user();
