// Test for cppx.string_utils module

import std;
import cppx.string_utils;
import cppx.logging;

#define assert(x)                                                                               \
  if (!(x)) {                                                                                   \
    std::cerr << "Assertion failed: " #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
    std::abort();                                                                               \
  }

using namespace cppx;

void test_split() {
  auto parts = split("hello,world,test", ",");
  assert(parts.size() == 3);
  assert(parts[0] == "hello");
  assert(parts[1] == "world");
  assert(parts[2] == "test");
  auto logger = get_logger("test_string_utils");
  logger->info("✓ split test passed");
}

void test_join() {
  std::vector<std::string> parts = {"hello", "world", "test"};
  auto joined = join(parts, "-");
  assert(joined == "hello-world-test");
  auto logger = get_logger("test_string_utils");
  logger->info("✓ join test passed");
}

void test_trim() {
  assert(trim("  hello  ") == "hello");
  assert(trim_left("  hello") == "hello");
  assert(trim_right("hello  ") == "hello");
  auto logger = get_logger("test_string_utils");
  logger->info("✓ trim test passed");
}

void test_case_conversion() {
  assert(to_upper("hello") == "HELLO");
  assert(to_lower("WORLD") == "world");
  assert(capitalize("hello world") == "Hello world");
  auto logger = get_logger("test_string_utils");
  logger->info("✓ case conversion test passed");
}

void test_predicates() {
  assert(starts_with("hello world", "hello"));
  assert(ends_with("hello world", "world"));
  assert(contains("hello world", "lo wo"));
  assert(!starts_with("hello", "world"));
  auto logger = get_logger("test_string_utils");
  logger->info("✓ predicates test passed");
}

void test_replace() {
  auto result = replace_all("hello hello hello", "hello", "hi");
  assert(result == "hi hi hi");
  auto logger = get_logger("test_string_utils");
  logger->info("✓ replace test passed");
}

void test_padding() {
  assert(pad_left("5", 3, '0') == "005");
  assert(pad_right("5", 3, '0') == "500");
  assert(center("hi", 6, '-') == "--hi--");
  auto logger = get_logger("test_string_utils");
  logger->info("✓ padding test passed");
}

void test_parse() {
  auto int_result = parse_int("42");
  assert(int_result.is_ok());
  assert(int_result.value() == 42);

  auto double_result = parse_double("3.14");
  assert(double_result.is_ok());
  assert(std::abs(double_result.value() - 3.14) < 0.001);

  auto logger = get_logger("test_string_utils");
  logger->info("✓ parse test passed");
}

int main() {
  auto logger = get_logger("test_string_utils");
  logger->info("=== String Utils Test Suite ===\n");

  test_split();
  test_join();
  test_trim();
  test_case_conversion();
  test_predicates();
  test_replace();
  test_padding();
  test_parse();

  logger->info("\n✓ All string_utils tests passed!");
  return 0;
}
