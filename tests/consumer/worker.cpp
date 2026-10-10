import std;
import cppx.error;
import cppx.string_utils;

cppx::expected<int, cppx::error_info> consumer_parse(std::string_view input) {
  return cppx::parse_int(cppx::trim(input));
}
