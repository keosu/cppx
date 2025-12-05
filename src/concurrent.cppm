// cppx.concurrent - Concurrency utilities
// Provides thread pool, parallel execution, and channels

export module cppx.concurrent;

import std;
import cppx.error;

namespace cppx {

// ============================================================================
// Thread pool
// ============================================================================

export class thread_pool {
public:
    explicit thread_pool(size_t num_threads = std::thread::hardware_concurrency())
        : stop_(false)
    {
        if (num_threads == 0) {
            num_threads = 1;
        }
        
        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_thread(); });
        }
    }
    
    ~thread_pool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    // Non-copyable, non-movable
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;
    
    // Submit a task and get a future
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using return_type = std::invoke_result_t<F, Args...>;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if (stop_) {
                throw std::runtime_error("Cannot submit task to stopped thread pool");
            }
            
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    // Get number of threads
    [[nodiscard]] size_t size() const {
        return workers_.size();
    }
    
    // Get number of pending tasks
    [[nodiscard]] size_t pending_tasks() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

private:
    void worker_thread() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this] {
                    return stop_ || !tasks_.empty();
                });
                
                if (stop_ && tasks_.empty()) {
                    return;
                }
                
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            
            task();
        }
    }
    
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};

// ============================================================================
// Parallel for
// ============================================================================

// Execute function for each element in range in parallel
export template<typename IndexType, typename Function>
void parallel_for(IndexType start, IndexType end, Function&& func, 
                 size_t num_threads = std::thread::hardware_concurrency()) {
    if (start >= end) {
        return;
    }
    
    if (num_threads == 0) {
        num_threads = 1;
    }
    
    IndexType total = end - start;
    IndexType chunk_size = (total + num_threads - 1) / num_threads;
    
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    
    for (size_t t = 0; t < num_threads; ++t) {
        IndexType chunk_start = start + t * chunk_size;
        IndexType chunk_end = std::min(chunk_start + chunk_size, end);
        
        if (chunk_start >= chunk_end) {
            break;
        }
        
        threads.emplace_back([chunk_start, chunk_end, &func]() {
            for (IndexType i = chunk_start; i < chunk_end; ++i) {
                func(i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Execute function for each element in container in parallel
export template<typename Container, typename Function>
void parallel_for_each(Container& container, Function&& func,
                      size_t num_threads = std::thread::hardware_concurrency()) {
    if (container.empty()) {
        return;
    }
    
    if (num_threads == 0) {
        num_threads = 1;
    }
    
    size_t total = container.size();
    size_t chunk_size = (total + num_threads - 1) / num_threads;
    
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    
    auto it = container.begin();
    for (size_t t = 0; t < num_threads; ++t) {
        auto chunk_start = it;
        auto chunk_end = it;
        
        size_t count = 0;
        while (count < chunk_size && chunk_end != container.end()) {
            ++chunk_end;
            ++count;
        }
        
        if (chunk_start == chunk_end) {
            break;
        }
        
        threads.emplace_back([chunk_start, chunk_end, &func]() {
            for (auto iter = chunk_start; iter != chunk_end; ++iter) {
                func(*iter);
            }
        });
        
        it = chunk_end;
        if (it == container.end()) {
            break;
        }
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// ============================================================================
// Channel (thread-safe queue with blocking)
// ============================================================================

export template<typename T>
class channel {
public:
    explicit channel(size_t capacity = 0)
        : capacity_(capacity), closed_(false)
    {}
    
    // Send value to channel (blocks if full)
    bool send(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (closed_) {
            return false;
        }
        
        if (capacity_ > 0) {
            send_cv_.wait(lock, [this] {
                return queue_.size() < capacity_ || closed_;
            });
            
            if (closed_) {
                return false;
            }
        }
        
        queue_.push(value);
        recv_cv_.notify_one();
        return true;
    }
    
    bool send(T&& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (closed_) {
            return false;
        }
        
        if (capacity_ > 0) {
            send_cv_.wait(lock, [this] {
                return queue_.size() < capacity_ || closed_;
            });
            
            if (closed_) {
                return false;
            }
        }
        
        queue_.push(std::move(value));
        recv_cv_.notify_one();
        return true;
    }
    
    // Try to send without blocking
    bool try_send(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (closed_) {
            return false;
        }
        
        if (capacity_ > 0 && queue_.size() >= capacity_) {
            return false;
        }
        
        queue_.push(value);
        recv_cv_.notify_one();
        return true;
    }
    
    bool try_send(T&& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (closed_) {
            return false;
        }
        
        if (capacity_ > 0 && queue_.size() >= capacity_) {
            return false;
        }
        
        queue_.push(std::move(value));
        recv_cv_.notify_one();
        return true;
    }
    
    // Receive value from channel (blocks if empty)
    std::optional<T> receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        recv_cv_.wait(lock, [this] {
            return !queue_.empty() || closed_;
        });
        
        if (queue_.empty()) {
            return std::nullopt;
        }
        
        T value = std::move(queue_.front());
        queue_.pop();
        
        if (capacity_ > 0) {
            send_cv_.notify_one();
        }
        
        return value;
    }
    
    // Try to receive without blocking
    std::optional<T> try_receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (queue_.empty()) {
            return std::nullopt;
        }
        
        T value = std::move(queue_.front());
        queue_.pop();
        
        if (capacity_ > 0) {
            send_cv_.notify_one();
        }
        
        return value;
    }
    
    // Close the channel
    void close() {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_ = true;
        recv_cv_.notify_all();
        send_cv_.notify_all();
    }
    
    [[nodiscard]] bool is_closed() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return closed_;
    }
    
    [[nodiscard]] size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    [[nodiscard]] bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<T> queue_;
    size_t capacity_;
    bool closed_;
    
    mutable std::mutex mutex_;
    std::condition_variable recv_cv_;
    std::condition_variable send_cv_;
};

// ============================================================================
// Blocking queue (unbounded thread-safe queue)
// ============================================================================

export template<typename T>
class blocking_queue {
public:
    blocking_queue() = default;
    
    void push(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(value);
        cv_.notify_one();
    }
    
    void push(T&& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
        cv_.notify_one();
    }
    
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }
    
    bool try_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (queue_.empty()) {
            return false;
        }
        
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    [[nodiscard]] bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    [[nodiscard]] size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

// ============================================================================
// Async utilities
// ============================================================================

// Run function asynchronously and return future
export template<typename F, typename... Args>
auto async(F&& f, Args&&... args) {
    return std::async(std::launch::async, 
                     std::forward<F>(f), 
                     std::forward<Args>(args)...);
}

// Run multiple functions in parallel and wait for all to complete
export template<typename... Functions>
void parallel_invoke(Functions&&... functions) {
    std::vector<std::thread> threads;
    threads.reserve(sizeof...(functions));
    
    (threads.emplace_back(std::forward<Functions>(functions)), ...);
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// ============================================================================
// Atomic counter
// ============================================================================

export class atomic_counter {
public:
    explicit atomic_counter(long long initial = 0)
        : value_(initial)
    {}
    
    long long increment() {
        return value_.fetch_add(1) + 1;
    }
    
    long long decrement() {
        return value_.fetch_sub(1) - 1;
    }
    
    long long add(long long delta) {
        return value_.fetch_add(delta) + delta;
    }
    
    long long get() const {
        return value_.load();
    }
    
    void set(long long value) {
        value_.store(value);
    }
    
    long long exchange(long long new_value) {
        return value_.exchange(new_value);
    }
    
    bool compare_exchange(long long expected, long long desired) {
        return value_.compare_exchange_strong(expected, desired);
    }

private:
    std::atomic<long long> value_;
};

// ============================================================================
// Spinlock (for very short critical sections)
// ============================================================================

export class spinlock {
public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // Busy wait
        }
    }
    
    void unlock() {
        flag_.clear(std::memory_order_release);
    }
    
    bool try_lock() {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

private:
    std::atomic_flag flag_;
};

// ============================================================================
// Once flag wrapper
// ============================================================================

export class call_once {
public:
    template<typename F, typename... Args>
    void operator()(F&& f, Args&&... args) {
        std::call_once(flag_, std::forward<F>(f), std::forward<Args>(args)...);
    }

private:
    std::once_flag flag_;
};

} // namespace cppx

