// cppx.concurrent module demo

import std;
import cppx.concurrent;

import cppx.logging;

using namespace cppx;

int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    auto logger = get_logger("concurrent_demo");
    logger->info("=== cppx.concurrent Demo ===\n");
    
    // Example 1: Thread pool
    logger->info("1. Thread pool with tasks:");
    {
        thread_pool pool(4);
        logger->info("   Created thread pool with 4 threads");
        
        std::vector<std::future<int>> futures;
        
        // Submit multiple tasks
        for (int i = 20; i <= 25; ++i) {
            futures.push_back(pool.submit(fibonacci, i));
        }
        
        // Collect results
        for (size_t i = 0; i < futures.size(); ++i) {
            std::cout << "   fib(" << (20 + i) << ") = " 
                      << futures[i].get() << "\n";
        }
    }
    logger->info("\n");
    
    // Example 2: Parallel for
    logger->info("2. Parallel for loop:");
    std::vector<int> data(100);
    
    parallel_for(0, 100, [&data](int i) {
        data[i] = i * i;
    });
    
    logger->info("   Computed squares for 0-99 in parallel");
    logger->info("   data[10] = {}", data[10]);
    logger->info("   data[50] = {}", data[50]);
    
    // Example 3: Channel communication
    logger->info("3. Channel for thread communication:");
    {
        channel<int> ch(5);
        
        // Producer thread
        std::thread producer([&ch, logger]() {
            for (int i = 1; i <= 10; ++i) {
                ch.send(i);
                logger->info("   Sent: {}", i);
            }
            ch.close();
        });
        
        // Consumer thread
        std::thread consumer([&ch, logger]() {
            while (true) {
                auto value = ch.receive();
                if (!value) break;
                logger->info("   Received: {}", *value);
            }
        });
        
        producer.join();
        consumer.join();
    }
    logger->info("\n");
    
    // Example 4: Blocking queue
    logger->info("4. Blocking queue:");
    {
        blocking_queue<std::string> queue;
        
        std::thread producer([&queue, logger]() {
            std::vector<std::string> items = {
                "task1", "task2", "task3", "task4", "task5"
            };
            for (const auto& item : items) {
                queue.push(item);
                logger->info("   Produced: {}", item);
            }
            queue.push("DONE");  // Sentinel value
        });
        
        std::thread consumer([&queue, logger]() {
            while (true) {
                auto item = queue.pop();
                if (item == "DONE") break;
                logger->info("   Consumed: {}", item);
            }
        });
        
        producer.join();
        consumer.join();
    }
    logger->info("\n");
    
    // Example 5: Atomic counter
    logger->info("5. Atomic counter:");
    {
        atomic_counter counter(0);
        std::vector<std::thread> threads;
        
        // Multiple threads incrementing
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&counter, logger]() {
                for (int j = 0; j < 1000; ++j) {
                    counter.increment();
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        logger->info("   Final counter value: {}", counter.get());
        logger->info("   (Expected: 10000)");
    }
    logger->info("\n");
    
    // Example 6: Parallel invoke
    logger->info("6. Parallel invoke (run functions simultaneously):");
    parallel_invoke(
        [logger]() { logger->info("   Task 1 running"); },
        [logger]() { logger->info("   Task 2 running"); },
        [logger]() { logger->info("   Task 3 running"); }
    );
    logger->info("   All tasks completed\n");
    
    // Example 7: Parallel for each
    logger->info("7. Parallel for each:");
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> squares(numbers.size());
    
    parallel_for_each(numbers, [&squares, &numbers](int value) {
        // Find index (in real code, use indexed parallel_for)
        auto it = std::find(numbers.begin(), numbers.end(), value);
        auto index = std::distance(numbers.begin(), it);
        squares[index] = value * value;
    });
    
    logger->info("   Computed squares: ");
    for (auto s : squares) {
        std::cout << s << " ";
    }
    logger->info("\n");
    
    logger->info("\n=== Demo completed ===");
    return 0;
}

