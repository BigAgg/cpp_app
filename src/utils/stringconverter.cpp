#include "utils/stringconverter.h"

#include <string>
#include <utility>
#include <chrono>
#include <algorithm>

using namespace std::chrono;

std::pair<std::string, std::string> split_at(const std::string &input, const std::string &delimiter) {
  std::size_t pos = input.find(delimiter);
  if (pos == std::string::npos)
    return {input, ""};
  return {
      input.substr(0, pos),
      input.substr(pos + delimiter.length())
  };
}

std::string to_lower (const std::string& s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
  return out;
}

std::string get_date (const std::string& fmt) {
  auto now = system_clock::now();
  auto today = floor<days>(now);
  year_month_day ymd{today};
  return std::vformat(fmt, std::make_format_args(ymd));
}

std::string get_user () {
  return std::getenv("USERNAME");
}

