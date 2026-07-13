#pragma once

#include <string>
#include <utility>

std::pair<std::string, std::string> split_at(const std::string &input, const std::string &delimiter);
void replace_all(std::string &str, std::string_view from, std::string_view to);
bool replace_first(std::string &str, std::string_view from, std::string_view to);
std::string to_lower(const std::string &s);
std::string get_date(const std::string &fmt);
std::string get_timestamp();
std::string to_german_time(const std::string &time);
std::string get_user();
std::string to_utf8(const std::string &content);
std::string from_utf8(const std::string &content);
bool is_integer(const std::string &str);
