// Test for cppx.cli module

import std;
import cppx.cli;
import cppx.logging;

#define assert(x)                                                                               \
  if (!(x)) {                                                                                   \
    std::cerr << "Assertion failed: " #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
    std::abort();                                                                               \
  }

using namespace cppx;

void test_positional_args() {
  argument_parser parser("test", "Test program");
  parser.add_positional("input", "Input file");

  std::vector<std::string> args = {"file.txt"};
  auto result = parser.parse(args);

  assert(result.is_ok());
  assert(result.value().get_string("input").value() == "file.txt");
  auto logger = get_logger("test_cli");
  logger->info("✓ positional args test passed");
}

void test_options() {
  argument_parser parser("test", "Test program");
  parser.add_option("output", "o", "Output file", false, "default.txt");

  std::vector<std::string> args = {"--output", "result.txt"};
  auto result = parser.parse(args);

  assert(result.is_ok());
  assert(result.value().get_string("output").value() == "result.txt");
  auto logger = get_logger("test_cli");
  logger->info("✓ options test passed");
}

void test_short_options() {
  argument_parser parser("test", "Test program");
  parser.add_option("output", "o", "Output file");

  std::vector<std::string> args = {"-o", "file.txt"};
  auto result = parser.parse(args);

  assert(result.is_ok());
  assert(result.value().get_string("output").value() == "file.txt");
  auto logger = get_logger("test_cli");
  logger->info("✓ short options test passed");
}

void test_flags() {
  argument_parser parser("test", "Test program");
  parser.add_flag("verbose", "v", "Verbose output");

  std::vector<std::string> args = {"--verbose"};
  auto result = parser.parse(args);

  assert(result.is_ok());
  assert(result.value().get_flag("verbose") == true);

  std::vector<std::string> args2 = {};
  auto result2 = parser.parse(args2);
  assert(result2.is_ok());
  assert(result2.value().get_flag("verbose") == false);

  auto logger = get_logger("test_cli");
  logger->info("✓ flags test passed");
}

void test_default_values() {
  argument_parser parser("test", "Test program");
  parser.add_option("count", "c", "Count", false, "10");

  std::vector<std::string> args = {};
  auto result = parser.parse(args);

  assert(result.is_ok());
  assert(result.value().get_string("count").value() == "10");
  auto logger = get_logger("test_cli");
  logger->info("✓ default values test passed");
}

void test_required_args() {
  argument_parser parser("test", "Test program");
  parser.add_option("input", "i", "Input file", true);

  std::vector<std::string> args = {};
  auto result = parser.parse(args);

  assert(result.is_err());
  auto logger = get_logger("test_cli");
  logger->info("✓ required args test passed");
}

void test_type_conversion() {
  argument_parser parser("test", "Test program");
  parser.add_option("count", "c", "Count");

  std::vector<std::string> args = {"--count", "42"};
  auto result = parser.parse(args);

  assert(result.is_ok());
  auto count = result.value().get_int("count");
  assert(count.is_ok());
  assert(count.value() == 42);
  auto logger = get_logger("test_cli");
  logger->info("✓ type conversion test passed");
}

int main() {
  auto logger = get_logger("test_cli");
  logger->info("=== CLI Test Suite ===\n");

  test_positional_args();
  test_options();
  test_short_options();
  test_flags();
  test_default_values();
  test_required_args();
  test_type_conversion();

  logger->info("\n✓ All cli tests passed!");
  return 0;
}
