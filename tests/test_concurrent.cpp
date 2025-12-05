// Test for cppx.concurrent module

import std;
import cppx.concurrent;
import cppx.logging;

#define assert(x) if (!(x)) { std::cerr << "Assertion failed: " #x << " at " << __FILE__ << ":" << __LINE__ << std::endl; std::abort(); }

using namespace cppx;

void test_thread_pool() {
    thread_pool pool(4);
    
    auto future = pool.submit([]() {
        return 42;
    });
    
    int result = future.get();
    assert(result == 42);
    auto logger = get_logger("test_concurrent");
    logger->info("✓ thread_pool test passed");
}

void test_parallel_for() {
    std::vector<int> values(100, 0);
    
    parallel_for(0, 100, [&values](int i) {
        values[i] = i * 2;
    });
    
    assert(values[0] == 0);
    assert(values[50] == 100);
    assert(values[99] == 198);
    auto logger = get_logger("test_concurrent");
    logger->info("✓ parallel_for test passed");
}

void test_channel() {
    channel<int> ch(10);
    
    // Send in one thread
    std::thread sender([&ch]() {
        for (int i = 0; i < 5; ++i) {
            ch.send(i);
        }
        ch.close();
    });
    
    // Receive in main thread
    int count = 0;
    while (true) {
        auto val = ch.receive();
        if (!val) break;
        assert(*val == count);
        ++count;
    }
    
    assert(count == 5);
    sender.join();
    auto logger = get_logger("test_concurrent");
    logger->info("✓ channel test passed");
}

void test_blocking_queue() {
    blocking_queue<int> queue;
    
    queue.push(1);
    queue.push(2);
    queue.push(3);
    
    assert(queue.size() == 3);
    assert(queue.pop() == 1);
    assert(queue.pop() == 2);
    assert(queue.pop() == 3);
auto logger = get_logger("test_concurrent");
    logger->info("✓ blocking_queue test passed");
}

void test_atomic_counter() {
    atomic_counter counter(0);
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 100; ++j) {
                counter.increment();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    assert(counter.get() == 1000);
    auto logger = get_logger("test_concurrent");
    logger->info("✓ atomic_counter test passed");
}

int main() {
    auto logger = get_logger("test_concurrent");
    logger->info("=== Concurrent Test Suite ===\n");
    
    test_thread_pool();
    test_parallel_for();
    test_channel();
    test_blocking_queue();
    test_atomic_counter();
    
    logger->info("\n✓ All concurrent tests passed!");
    return 0;
}

