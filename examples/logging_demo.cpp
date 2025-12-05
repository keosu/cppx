// cppx.logging module demo

import std;
import cppx.logging;

using namespace cppx;

void process_data() {
  auto logger = get_logger("processor");
  logger->info("Starting data processing...");

  // Simulate processing
  for (int i = 0; i < 3; ++i) {
    logger->debug("Processing item {}", i);
  }

  logger->info("Data processing completed");
}

void risky_operation() {
  auto logger = get_logger("risky");

  log_scope scope(logger, "risky_operation");

  logger->warn("This operation might take a while");
  // Simulate work
  logger->info("Operation in progress...");
}

int main() {
  std::cout << "=== cppx.logging Demo ===\n\n";

  // Example 1: Basic logging
  std::cout << "1. Basic logging:\n";
  auto logger = get_logger("main");
  logger->info("Application started");
  logger->debug("Debug information");
  logger->warn("This is a warning");
  logger->error("This is an error message");
  std::cout << "\n";

  // Example 2: Log levels
  std::cout << "2. Changing log levels:\n";
  auto logger2 = create_logger("level_demo");
  logger2->set_level(log_level::warn);
  std::cout << "   (Log level set to WARN)\n";
  logger2->debug("This won't appear");
  logger2->info("This won't appear either");
  logger2->warn("But this warning will");
  logger2->error("And this error too");
  std::cout << "\n";

  // Example 3: Formatted logging
  std::cout << "3. Formatted logging:\n";
  auto logger3 = create_logger("format");
  int count = 42;
  std::string name = "Alice";
  logger3->info("User {} has {} items", name, count);
  logger3->info("Calculation: {} + {} = {}", 10, 20, 30);
  std::cout << "\n";

  // Example 4: Multiple loggers
  std::cout << "4. Multiple loggers:\n";
  auto db_logger = get_logger("database");
  auto net_logger = get_logger("network");

  db_logger->info("Connected to database");
  net_logger->info("Server listening on port 8080");
  db_logger->debug("Executing query...");
  net_logger->debug("Received request");
  std::cout << "\n";

  // Example 5: Scoped logging
  std::cout << "5. Scoped logging (measures execution time):\n";
  process_data();
  std::cout << "\n";

  risky_operation();
  std::cout << "\n";

  // Example 6: Log scope with custom callback
  std::cout << "6. Log scope with timing:\n";
  {
    auto logger4 = create_logger("timer");
    log_scope scope(logger4, "expensive_calculation");

    // Simulate work
    int sum = 0;
    for (int i = 0; i < 1000000; ++i) {
      sum += i;
    }
    logger4->debug("Calculation result: {}", sum);
  }

  std::cout << "7. UTF-8 and Emoji Test:\n";
  {
    auto logger = get_logger("utf8_test");

    logger->info("=== UTF-8 and Emoji Test ===");
    logger->info("");

    // Test checkmark and other symbols
    logger->info("✓ Checkmark works!");
    logger->info("✗ Cross mark");
    logger->info("→ Arrow");
    logger->info("• Bullet");
    logger->info("★ Star");
    logger->info("");

    // Test emojis
    logger->info("😀 Grinning face");
    logger->info("🎉 Party popper");
    logger->info("🚀 Rocket");
    logger->info("💻 Laptop");
    logger->info("📊 Chart");
    logger->info("🔧 Wrench");
    logger->info("✨ Sparkles");
    logger->info("🐛 Bug");
    logger->info("🎯 Target");
    logger->info("");

    // Test Chinese
    logger->info("中文测试 - Chinese test");
    logger->info("你好世界！- Hello World!");
    logger->info("");

    // Test Japanese
    logger->info("日本語テスト - Japanese test");
    logger->info("こんにちは世界！- Hello World!");
    logger->info("");

    // Test combined
    logger->info("✓ Build successful 🎉");
    logger->info("⚠️ Warning: Test mode");
    logger->info("❌ Error detected 🐛");
    logger->info("📦 Package ready 🚀");
    logger->info("");

    logger->info("=== All UTF-8 characters displayed correctly! ===");
  }

  std::cout << "\n=== Demo completed ===\n";
  return 0;
}
