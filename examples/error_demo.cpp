// cppx.error module demo

import std;
import cppx.error;
import cppx.logging;

using namespace cppx;

// Example function that returns expected
expected<double, error_info> divide(double a, double b) {
  if (b == 0.0) {
    return expected<double, error_info>::err(error_info("Division by zero"));
  }
  return expected<double, error_info>::ok(a / b);
}

// Example function chaining
expected<int, error_info> parse_and_double(const std::string& str) {
  return try_catch([&str]() -> int {
    int value = std::stoi(str);
    return value * 2;
  });
}

int main() {
  auto logger = get_logger("error_demo");

  logger->info("=== cppx.error Demo ===\n");

  // Example 1: Basic expected usage
  logger->info("1. Basic expected usage:");
  auto result1 = divide(10.0, 2.0);
  if (result1.is_ok()) {
    logger->info("   10 / 2 = {}", result1.value());
  }

  auto result2 = divide(10.0, 0.0);
  if (result2.is_err()) {
    logger->info("   10 / 0 = Error: {}", result2.error().message());
  }
  logger->info("");

  // Example 2: Mapping and chaining
  logger->info("2. Mapping and chaining:");
  auto result3 = divide(20.0, 4.0).map([](double x) { return x * 2; });

  if (result3.is_ok()) {
    logger->info("   (20 / 4) * 2 = {}", result3.value());
  }
  logger->info("");

  // Example 3: Error context
  logger->info("3. Error with context:");
  error_info err("File not found");
  err.add_context("read_config");
  err.add_context("init_system");
  logger->info("   Error: {}", err.to_string());
  logger->info("");

  // Example 4: try_catch helper
  logger->info("4. try_catch helper:");
  auto result4 = parse_and_double("42");
  if (result4.is_ok()) {
    logger->info("   parse_and_double(\"42\") = {}", result4.value());
  }

  auto result5 = parse_and_double("invalid");
  if (result5.is_err()) {
    logger->info("   parse_and_double(\"invalid\") = Error: {}", result5.error().message());
  }
  logger->info("");

  // Example 5: value_or
  logger->info("5. Using value_or for defaults:");
  auto result6 = divide(10.0, 0.0);
  double safe_value = result6.value_or(-1.0);
  logger->info("   Result with fallback: {}", safe_value);

  logger->info("\n=== Demo completed ===");
  return 0;
}
