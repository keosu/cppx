// Verify the test runner using ordinary process-level checks, independent of its assertions.
import std;
import cppx.test;
import cppx.logging;

#include "../src/test_macros.h"

using namespace cppx;

int main() {
  get_logger("test_runner")->remove_console_sink();
  auto& registry = test_registry::instance();
  test_runner runner;
  int failures = 0;
  auto verify = [&](bool condition, std::string_view description) {
    if (!condition) {
      std::cerr << "Runner contract failed: " << description << '\n';
      ++failures;
    }
  };
  auto add_case = [&](std::string name, std::function<void()> body) {
    registry.register_test({std::move(name), "[contract]", std::move(body), __FILE__, __LINE__});
  };

  registry.clear();
  auto empty = runner.run_all();
  verify(runner.failed_count() == 1 && empty.size() == 1, "empty registry must fail");

  bool check_continued = false;
  bool require_continued = false;
  add_case("check failure", [&] {
    CHECK_MSG(false, "check detail");
    check_continued = true;
    CHECK(true);
  });
  add_case("require failure", [&] {
    CHECK(true);
    REQUIRE_MSG(false, "require detail");
    require_continued = true;
  });
  add_case("standard exception", [] {
    CHECK(true);
    throw std::runtime_error("runtime detail");
  });
  add_case("unknown exception", [] { throw 42; });
  add_case("passing case", [] { REQUIRE(true); });

  auto results = runner.run_all();
  verify(results.size() == 5 && runner.failed_count() == 4 && runner.passed_count() == 1,
         "mixed pass/failure totals");
  verify(check_continued && !require_continued, "CHECK continues and REQUIRE stops the case");
  if (results.size() == 5) {
    verify(results[0].assertions.size() == 2, "CHECK records both assertions");
    verify(results[1].assertions.size() == 2, "REQUIRE preserves assertions when it throws");
    if (results[1].assertions.size() == 2) {
      const auto& failure = results[1].assertions[1];
      verify(!failure.passed && failure.message == "require detail" && failure.line > 0 &&
                 !failure.file.empty(),
             "REQUIRE retains the failure location and message");
    }
    verify(results[2].assertions.size() == 1 && results[2].error_message == "runtime detail",
           "unexpected exceptions preserve earlier assertions");
    verify(results[3].error_message == "Unknown exception", "non-standard exception diagnosis");
    verify(results[4].assertions.size() == 1, "assertions do not leak between cases");
  }

  auto filtered = runner.run_filtered("passing case");
  verify(filtered.size() == 1 && runner.failed_count() == 0 && runner.passed_count() == 1,
         "filtered runs reset counters");
  auto missing = runner.run_filtered("no matching case");
  verify(missing.size() == 1 && runner.failed_count() == 1, "unmatched filter must fail");

  registry.clear();
  std::vector<int> visits;
  int invocations = 0;
  add_case("section isolation", [&] {
    ++invocations;
    int value = 0;
    SECTION("parent") {
      REQUIRE(value == 0);
      value = 10;
      SECTION("same name") {
        REQUIRE(value == 10);
        value = 11;
        visits.push_back(value);
      }
      SECTION("same name") {
        REQUIRE(value == 10);
        visits.push_back(12);
      }
    }
    SECTION("sibling") {
      REQUIRE(value == 0);
      visits.push_back(20);
    }
  });
  results = runner.run_all();
  verify(runner.failed_count() == 0, "sibling and nested sections start with fresh state");
  verify(visits == std::vector<int>({11, 12, 20}) && invocations == 3,
         "each leaf executes once, including sections with identical names");
  if (results.size() == 1) {
    verify(results[0].assertions.size() == 5, "assertions accumulate across all section paths");
  }

  registry.clear();
  bool second_section_ran = false;
  add_case("failure in an earlier section", [&] {
    SECTION("failure") { CHECK(false); }
    SECTION("success") {
      second_section_ran = true;
      CHECK(true);
    }
  });
  results = runner.run_all();
  verify(second_section_ran && runner.failed_count() == 1,
         "later success cannot erase an earlier section failure");
  if (results.size() == 1) {
    verify(results[0].assertions.size() == 2, "retain all section assertions");
  }

  registry.clear();
  int passes = 0;
  add_case("failure before section replay", [&] {
    CHECK(++passes > 1);
    SECTION("only section") { CHECK(true); }
  });
  results = runner.run_all();
  verify(passes == 1 && runner.failed_count() == 1,
         "single section runs once and a failed CHECK cannot become a passing test");

  registry.clear();
  bool after_fatal_section = false;
  add_case("fatal section", [&] {
    SECTION("fatal") { REQUIRE(false); }
    SECTION("unreached") { after_fatal_section = true; }
  });
  add_case("following case", [] { CHECK(true); });
  results = runner.run_all();
  verify(!after_fatal_section && runner.failed_count() == 1 && runner.passed_count() == 1,
         "fatal assertion stops its case while following cases still run");
  if (results.size() == 2) {
    verify(results[0].assertions.size() == 1 && results[1].assertions.size() == 1,
           "fatal section unwinding does not corrupt the next case");
  }

  registry.clear();
  if (failures != 0) return 1;
  std::cout << "Test runner contracts passed\n";
  return 0;
}
