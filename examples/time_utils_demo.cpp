// cppx.time_utils module demo

import std;
import cppx.time_utils;

import cppx.logging;

using namespace cppx;

void expensive_operation() {
  int sum = 0;
  for (int i = 0; i < 10000000; ++i) {
    sum += i;
  }
}

int main() {
  auto logger = get_logger("time_utils_demo");
  logger->info("=== cppx.time_utils Demo ===\n");

  // Example 1: Current time formatting
  logger->info("1. Current time formatting:");
  logger->info("   ISO8601: {}", now_iso8601());
  logger->info("   RFC3339: {}", now_rfc3339());
  logger->info("   Custom:  {}", now_formatted("%Y/%m/%d %H:%M:%S"));

  // Example 2: Duration formatting
  logger->info("2. Duration formatting:");
  logger->info("   500ms: {}", format_duration(std::chrono::milliseconds(500)));
  logger->info("   1500ms: {}", format_duration(std::chrono::milliseconds(1500)));
  logger->info("   65000ms: {}", format_duration(std::chrono::milliseconds(65000)));
  logger->info("   3661000ms: {}", format_duration(std::chrono::milliseconds(3661000)));

  // Example 3: Stopwatch
  logger->info("3. Using stopwatch:");
  stopwatch sw;
  sw.start();

  logger->info("   Performing operation...");
  sleep_ms(100);

  sw.stop();
  logger->info("   Elapsed time: {}ms", sw.elapsed_ms().count());

  // Example 4: Scope timer
  logger->info("4. Scope timer (auto-timing):");
  {
    scope_timer timer("calculation", [](std::chrono::milliseconds duration) {
      std::cout << "   Operation completed in " << duration.count() << "ms\n";
    });

    expensive_operation();
  }
  logger->info("\n");

  // Example 5: Multiple measurements
  logger->info("5. Measuring with stopwatch:");
  stopwatch watch;

  watch.start();
  sleep_ms(50);
  logger->info("   After 50ms: {}ms", watch.elapsed_ms().count());

  sleep_ms(50);
  logger->info("   After 100ms: {}ms", watch.elapsed_ms().count());

  watch.stop();
  logger->info("   Final: {}ms", watch.elapsed_ms().count());

  // Example 6: Timestamps
  logger->info("6. Unix timestamps:");
  auto ts_sec = timestamp_sec();
  auto ts_ms = timestamp_ms();
  auto ts_us = timestamp_us();

  logger->info("   Seconds: {}", ts_sec);
  logger->info("   Milliseconds: {}", ts_ms);
  logger->info("   Microseconds: {}", ts_us);

  // Example 7: Timer (periodic execution)
  logger->info("7. Periodic timer (runs 3 times):");
  timer periodic;
  int count = 0;

  periodic.start_periodic(std::chrono::milliseconds(100), [&count, logger]() {
    logger->info("   Tick {}", ++count);
    if (count >= 3) {
      // Timer will be stopped when it goes out of scope
    }
  });

  sleep_ms(350);  // Let it run for ~3 ticks
  periodic.stop();
  logger->info("\n");

  // Example 8: Restart stopwatch
  logger->info("8. Restart stopwatch:");
  stopwatch sw2;
  sw2.start();
  sleep_ms(50);
  logger->info("   First run: {}ms", sw2.elapsed_ms().count());

  sw2.restart();
  sleep_ms(30);
  logger->info("   After restart: {}ms", sw2.elapsed_ms().count());

  logger->info("\n=== Demo completed ===");
  return 0;
}
