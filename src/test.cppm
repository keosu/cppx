// cppx.test - A modern C++20 module-based testing framework
// Inspired by Catch2 with simplified syntax

export module cppx.test;

import std;
import cppx.string_utils;
import cppx.logging;

namespace cppx {

// Test result types
export enum class test_status { passed, failed, skipped };

// Test assertion result
export struct assertion_result {
  bool passed;
  std::string expression;
  std::string file;
  int line;
  std::string message;
};

// Section information
export struct section_info {
  std::string name;
  std::function<void()> body;
};

// Test case information
export struct test_case_info {
  std::string name;
  std::string tags;
  std::function<void()> body;
  std::string file;
  int line;
};

// Test result
export struct test_result {
  std::string name;
  test_status status;
  std::vector<assertion_result> assertions;
  std::chrono::milliseconds duration{0};
  std::string error_message;
};

// Test context for managing sections
export class test_context {
 private:
  std::vector<section_info> sections_;
  std::vector<assertion_result> assertions_;
  bool current_section_entered_ = false;
  size_t current_section_index_ = 0;

 public:
  void add_section(const std::string& name, std::function<void()> body) {
    sections_.push_back({name, std::move(body)});
  }

  bool should_enter_section() {
    if (current_section_index_ < sections_.size()) {
      current_section_entered_ = true;
      return true;
    }
    return false;
  }

  void run_next_section() {
    if (current_section_index_ < sections_.size()) {
      sections_[current_section_index_++].body();
    }
  }

  void add_assertion(assertion_result result) { assertions_.push_back(std::move(result)); }

  const std::vector<assertion_result>& get_assertions() const { return assertions_; }

  void reset() {
    sections_.clear();
    assertions_.clear();
    current_section_entered_ = false;
    current_section_index_ = 0;
  }

  bool has_sections() const { return !sections_.empty(); }

  size_t section_count() const { return sections_.size(); }
};

// Global test context (thread_local for thread safety)
thread_local test_context g_test_context;

// Test registry
export class test_registry {
 private:
  std::vector<test_case_info> test_cases_;

  test_registry() = default;

 public:
  static test_registry& instance() {
    static test_registry registry;
    return registry;
  }

  void register_test(test_case_info info) { test_cases_.push_back(std::move(info)); }

  const std::vector<test_case_info>& get_tests() const { return test_cases_; }

  void clear() { test_cases_.clear(); }
};

// Test registrar (for automatic registration)
export class test_registrar {
 public:
  test_registrar(const std::string& name, const std::string& tags, std::function<void()> body,
                 const char* file, int line) {
    test_registry::instance().register_test({name, tags, std::move(body), file, line});
  }
};

// Test runner
export class test_runner {
 private:
  std::shared_ptr<logger> logger_;
  size_t passed_count_ = 0;
  size_t failed_count_ = 0;
  size_t skipped_count_ = 0;
  std::vector<test_result> results_;

 public:
  test_runner() : logger_(get_logger("test_runner")) {}

  // Run all registered tests
  std::vector<test_result> run_all() {
    results_.clear();
    passed_count_ = 0;
    failed_count_ = 0;
    skipped_count_ = 0;

    const auto& tests = test_registry::instance().get_tests();

    logger_->info(
        "===============================================================================");
    logger_->info("Running {} test case(s)", tests.size());
    logger_->info(
        "===============================================================================");
    logger_->info("");

    for (const auto& test : tests) {
      auto result = run_test(test);
      results_.push_back(result);

      if (result.status == test_status::passed) {
        ++passed_count_;
      } else if (result.status == test_status::failed) {
        ++failed_count_;
      } else {
        ++skipped_count_;
      }
    }

    print_summary();
    return results_;
  }

  // Run tests matching a filter
  std::vector<test_result> run_filtered(const std::string& filter) {
    results_.clear();
    passed_count_ = 0;
    failed_count_ = 0;
    skipped_count_ = 0;

    const auto& tests = test_registry::instance().get_tests();
    std::vector<test_case_info> filtered_tests;

    for (const auto& test : tests) {
      if (test.name.find(filter) != std::string::npos ||
          test.tags.find(filter) != std::string::npos) {
        filtered_tests.push_back(test);
      }
    }

    logger_->info(
        "===============================================================================");
    logger_->info("Running {} test case(s) matching '{}'", filtered_tests.size(), filter);
    logger_->info(
        "===============================================================================");
    logger_->info("");

    for (const auto& test : filtered_tests) {
      auto result = run_test(test);
      results_.push_back(result);

      if (result.status == test_status::passed) {
        ++passed_count_;
      } else if (result.status == test_status::failed) {
        ++failed_count_;
      } else {
        ++skipped_count_;
      }
    }

    print_summary();
    return results_;
  }

  size_t passed_count() const { return passed_count_; }
  size_t failed_count() const { return failed_count_; }
  size_t skipped_count() const { return skipped_count_; }

 private:
  test_result run_test(const test_case_info& test) {
    test_result result;
    result.name = test.name;
    result.status = test_status::passed;

    logger_->info(
        "-------------------------------------------------------------------------------");
    logger_->info("{}", test.name);
    logger_->info(
        "-------------------------------------------------------------------------------");
    logger_->info("{}:{}", test.file, test.line);
    logger_->info(
        "...............................................................................");
    logger_->info("");

    auto start = std::chrono::steady_clock::now();

    try {
      // Reset test context
      g_test_context.reset();

      // Run the test body
      test.body();

      // If test has sections, run them
      if (g_test_context.has_sections()) {
        for (size_t i = 0; i < g_test_context.section_count(); ++i) {
          g_test_context.reset();
          test.body();
        }
      }

      // Collect assertions
      result.assertions = g_test_context.get_assertions();

      // Check if any assertion failed
      for (const auto& assertion : result.assertions) {
        if (!assertion.passed) {
          result.status = test_status::failed;
          logger_->error("{}:{}: FAILED:", assertion.file, assertion.line);
          logger_->error("  {}", assertion.expression);
          if (!assertion.message.empty()) {
            logger_->error("  {}", assertion.message);
          }
        }
      }

      if (result.status == test_status::passed) {
        logger_->info("✓ Test passed");
      }

    } catch (const std::exception& e) {
      result.status = test_status::failed;
      result.error_message = e.what();
      logger_->error("✗ Test failed with exception: {}", e.what());
    } catch (...) {
      result.status = test_status::failed;
      result.error_message = "Unknown exception";
      logger_->error("✗ Test failed with unknown exception");
    }

    auto end = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    logger_->info("");
    return result;
  }

  void print_summary() {
    logger_->info(
        "===============================================================================");
    logger_->info("Test Summary");
    logger_->info(
        "===============================================================================");

    size_t total = passed_count_ + failed_count_ + skipped_count_;
    logger_->info("Total: {}", total);
    logger_->info("Passed: {} ({}%)", passed_count_, total > 0 ? (passed_count_ * 100 / total) : 0);

    if (failed_count_ > 0) {
      logger_->error("Failed: {}", failed_count_);
    }

    if (skipped_count_ > 0) {
      logger_->warn("Skipped: {}", skipped_count_);
    }

    logger_->info(
        "===============================================================================");

    if (failed_count_ == 0) {
      logger_->info("✓ All tests passed!");
    } else {
      logger_->error("✗ {} test(s) failed", failed_count_);
    }
  }
};

// Assertion helpers
export inline void record_assertion(bool condition, const char* expr, const char* file, int line,
                                    const std::string& message = "") {
  assertion_result result;
  result.passed = condition;
  result.expression = expr;
  result.file = file;
  result.line = line;
  result.message = message;

  g_test_context.add_assertion(result);

  if (!condition) {
    throw std::runtime_error("Assertion failed: " + std::string(expr));
  }
}

export inline void record_check(bool condition, const char* expr, const char* file, int line,
                                const std::string& message = "") {
  assertion_result result;
  result.passed = condition;
  result.expression = expr;
  result.file = file;
  result.line = line;
  result.message = message;

  g_test_context.add_assertion(result);
  // Don't throw - CHECK doesn't stop test execution
}

// Section helper
export class section_guard {
 private:
  bool entered_ = false;

 public:
  section_guard(const std::string& name, std::function<void()> body) {
    g_test_context.add_section(name, std::move(body));
    entered_ = g_test_context.should_enter_section();
  }

  operator bool() const { return entered_; }
};

// Helper for test registration
export template <typename Func>
inline void register_test_case(const char* name, const char* tags, Func&& func, const char* file,
                               int line) {
  test_registry::instance().register_test({name, tags, std::forward<Func>(func), file, line});
}

}  // namespace cppx
