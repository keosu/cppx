import std;
import cppx.error;
import cppx.string_utils;
import cppx.file_system;
import cppx.logging;
import cppx.time_utils;
import cppx.concurrent;
import cppx.math_utils;
import cppx.cli;
import cppx.serialization;
import cppx.test;

#include "serialization_macros.h"
#include "test_macros.h"

cppx::expected<int, cppx::error_info> consumer_parse(std::string_view input);

struct consumer_record {
  int value;
};
CPPX_SERIALIZABLE(consumer_record, value)

TEST_CASE("External consumer", "[integration]") {
  auto value = consumer_parse(" 42 ");
  REQUIRE(value.is_ok());
  REQUIRE(value.value() == 42);
  REQUIRE(consumer_parse("invalid").is_err());
  REQUIRE(cppx::join(cppx::split("one,two", ","), "|") == "one|two");

  auto log = cppx::get_logger("consumer");
  log->remove_console_sink();
  log->info("External consumer: {}", value.value());

  cppx::thread_pool pool(1);
  REQUIRE(pool.submit([] { return 42; }).get() == 42);
  cppx::channel<int> channel;
  REQUIRE(channel.send(7));
  channel.close();
  REQUIRE(channel.receive() == 7);
  REQUIRE(!channel.receive());

  REQUIRE(cppx::mean(std::vector<int>{2, 4, 6}) == 4.0);
  REQUIRE(!cppx::now_iso8601().empty());
  REQUIRE(cppx::path_join("a", "b") == std::filesystem::path("a") / "b");
  cppx::argument_parser parser("consumer");
  parser.add_flag("verbose", "v", "Verbose output");
  auto arguments = parser.parse(std::vector<std::string>{"--verbose"});
  REQUIRE(arguments.is_ok());
  REQUIRE(arguments.value().get_flag("verbose"));

  auto encoded = cppx::to_json_string(consumer_record{42});
  REQUIRE(cppx::from_json_string<consumer_record>(encoded).value == 42);
}

int main() {
  cppx::test_runner runner;
  runner.run_all();
  return runner.failed_count() == 0 ? 0 : 1;
}
