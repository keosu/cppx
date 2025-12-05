// Test for cppx.time_utils module

import std;
import cppx.time_utils;
import cppx.logging;

#define assert(x) if (!(x)) { std::cerr << "Assertion failed: " #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; std::abort(); }

using namespace cppx;

void test_time_formatting() {
    auto logger = get_logger("test_time_utils");
    auto now = std::chrono::system_clock::now();
    auto iso = format_iso8601(now);
    assert(!iso.empty());
    assert(iso.find('T') != std::string::npos);
    logger->info("ISO8601: {}", iso);
    logger->info("✓ time formatting test passed");
}

void test_duration_formatting() {
    auto logger = get_logger("test_time_utils");
    auto duration = std::chrono::milliseconds(1500);
    auto formatted = format_duration(duration);
    assert(!formatted.empty());
    logger->info("Duration: {}", formatted);
    logger->info("✓ duration formatting test passed");
}

void test_stopwatch() {
    auto logger = get_logger("test_time_utils");
    stopwatch sw;
    sw.start();
    sleep_ms(10);
    sw.stop();
    
    auto elapsed = sw.elapsed_ms();
    assert(elapsed.count() >= 10);
    logger->info("Elapsed: {}ms", elapsed.count());
    logger->info("✓ stopwatch test passed");
}

void test_scope_timer() {
    auto logger = get_logger("test_time_utils");
    bool callback_called = false;
    {
        auto callback = [&callback_called, logger](auto duration) {
            callback_called = true;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            logger->info("Scope took: {}ms", ms);
        };
        scope_timer timer("test", callback);
        sleep_ms(5);
    }
    assert(callback_called);
    logger->info("✓ scope_timer test passed");
}

void test_timestamp() {
    auto logger = get_logger("test_time_utils");
    auto ts = timestamp_ms();
    assert(ts > 0);
    logger->info("Timestamp: {}", ts);
    logger->info("✓ timestamp test passed");
}

int main() {
    auto logger = get_logger("test_time_utils");
    logger->info("=== Time Utils Test Suite ===\n");
    
    test_time_formatting();
    test_duration_formatting();
    test_stopwatch();
    test_scope_timer();
    test_timestamp();
    
    logger->info("\n✓ All time_utils tests passed!");
    return 0;
}

