// cppx.string_utils module demo

import std;
import cppx.string_utils;
import cppx.logging;

using namespace cppx;

int main() {
  auto logger = get_logger("string_utils_demo");
  logger->info("=== cppx.string_utils Demo ===\n");

  // Example 1: Split and join
  logger->info("1. Split and join:");
  std::string csv = "apple,banana,cherry,date";
  auto fruits = split(csv, ",");
  logger->info("   Original: {}", csv);
  logger->info("   Split into {} parts", fruits.size());
  auto rejoined = join(fruits, " | ");
  logger->info("   Rejoined: {}\n", rejoined);

  // Example 2: Trimming
  logger->info("2. Trimming whitespace:");
  std::string messy = "   hello world   ";
  logger->info("   Original: '{}'", messy);
  logger->info("   Trimmed: '{}'", trim(messy));
  logger->info("   Trim left: '{}'", trim_left(messy));
  logger->info("   Trim right: '{}'", trim_right(messy));
  logger->info("");

  // Example 3: Case conversion
  logger->info("3. Case conversion:");
  std::string text = "Hello World";
  logger->info("   Original: {}", text);
  logger->info("   Upper: {}", to_upper(text));
  logger->info("   Lower: {}", to_lower(text));
  logger->info("   Capitalize: {}\n", capitalize(text));

  // Example 4: String predicates
  logger->info("4. String predicates:");
  std::string url = "https://example.com/page.html";
  logger->info("   URL: {}", url);
  logger->info("   Starts with 'https': {}", (starts_with(url, "https") ? "yes" : "no"));
  logger->info("   Ends with '.html': {}", (ends_with(url, ".html") ? "yes" : "no"));
  logger->info("   Contains 'example': {}\n", (contains(url, "example") ? "yes" : "no"));

  // Example 5: Replacement
  logger->info("5. String replacement:");
  std::string template_str = "Hello {name}, welcome to {place}!";
  auto result = replace_all(template_str, "{name}", "Alice");
  result = replace_all(result, "{place}", "Wonderland");
  logger->info("   Template: {}", template_str);
  logger->info("   Result: {}\n", result);

  // Example 6: Padding
  logger->info("6. Padding:");
  logger->info("   pad_left('5', 5, '0'): '{}'", pad_left("5", 5, '0'));
  logger->info("   pad_right('5', 5, '0'): '{}'", pad_right("5", 5, '0'));
  logger->info("   center('Hi', 10, '-'): '{}'\n", center("Hi", 10, '-'));

  // Example 7: Parsing numbers
  logger->info("7. Parsing numbers:");
  auto int_result = parse_int("42");
  if (int_result.is_ok()) {
    logger->info("   Parsed integer: {}", int_result.value());
  }

  auto double_result = parse_double("3.14159");
  if (double_result.is_ok()) {
    logger->info("   Parsed double: {}", double_result.value());
  }

  auto invalid = parse_int("not a number");
  if (invalid.is_err()) {
    logger->info("   Invalid parse: {}", invalid.error().message());
  }

  logger->info("\n=== Demo completed ===");
  return 0;
}
